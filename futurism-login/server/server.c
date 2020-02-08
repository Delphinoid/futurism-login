#include "server.h"
#include "../memory/memoryManager.h"
#include "../http/httpRequest.h"
#include "../http/httpResponse.h"
#include "../shared/helpers.h"
#include <string.h>
#include <stdio.h>

return_t serverInit(server *const __RESTRICT__ s){

	// Initialize defaults.
	// We don't load any configuration files at the moment.
	// Once we do, the server or database structs may have to be modified.
	char *path;
	ssConfig config = {
		.type = SOCK_STREAM,
		.protocol = IPPROTO_TCP,
		.port = 9000,
		.backlog = SOMAXCONN,
		.connections = SOCKET_MAX_SOCKETS
	};
	config.ip[0] = '\0';
	s->db.path_length = 1;
	s->db.path = '.';

	// Create folders if they don't already exist.
	path = memAllocate((s->db.path_length + 9)*sizeof(char));
	memcpy(path, &s->db.path, s->db.path_length);
	memcpy(&path[s->db.path_length], FILE_PATH_DELIMITER_STRING"users", 6);
	mkdir(path, 0744);
	memcpy(&path[s->db.path_length], FILE_PATH_DELIMITER_STRING"sessions", 9);
	mkdir(path, 0744);
	memFree(path);

	return ssInit(&s->ss, config);

}

void serverHandleRequest(server *const __RESTRICT__ s, socketDetails *const __RESTRICT__ client){

	httpRequest request;

	#ifdef SOCKET_DEBUG
	client->buffer[client->bufferSize] = '\0';
	printf("Received:\n%s\n", client->buffer);
	#endif

	// Check if the request is a HTTP request.
	if(httpRequestValid(&request, client->buffer, client->bufferSize)){

		// GET request.
		if(request.methodLength == 3){

			// GET request received from Futurism.
			if(memcmp(request.targetStart, "/?callback=jQuery", 17) == 0){

				char response[106 + SESSION_TOKEN_BYTES];
				size_t response_length;

				// Get the length of the JSONP function name and find any attached cookies.
				const char *const cookiesStart = httpRequestFindHeader(&request, NULL, "cookie", 6, NULL);
				const char *const targetStart = request.targetStart + 11;
				response_length = strchr(targetStart, '&') - targetStart;
				memcpy(response, targetStart, response_length);
				response[response_length] = '\0';

				// Check if a valid session exists.
				// For this we need to use cookies.
				if(
					cookiesStart != NULL &&
					memcmp(cookiesStart, "token=", 6) == 0 &&
					dbAuthenticate(&s->db, cookiesStart+6, NULL) == 0
				){
					memcpy(response+response_length, "({\"logged_in\": true, \"token\": \"", 31);
					response_length += 31;
					memcpy(response+response_length, cookiesStart+6, SESSION_TOKEN_BYTES);
					response_length += SESSION_TOKEN_BYTES;
					memcpy(response+response_length, "\"})", 3);
					response_length += 3;
				}else{
					memcpy(response+response_length, "({\"logged_in\": false})", 22);
					response_length += 22;
				}

				// Package and send response.
				httpResponse(response, response_length, NULL, 0, "application/javascript", 22);
				ssSendDataTCP(client->handle, response);

				#ifdef SOCKET_DEBUG
				printf("Sent:\n%s\n", response);
				{
					char ip[46];
					inet_ntop(
						client->address.ss_family,
						(client->address.ss_family == AF_INET ?
						(void *)(&((struct sockaddr_in *)&client->address)->sin_addr) :
						(void *)(&((struct sockaddr_in6 *)&client->address)->sin6_addr)),
						ip, sizeof(ip)
					);
					printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&client->address)->sin_port, (unsigned long)client->id);
				}
				#endif

				// Close the connection.
				socketclose(client->handle->fd);
				scRemoveSocket(&s->ss.connectionHandler, client);
				return;

			// GET request received from login client.
			}else if(memcmp(request.targetStart, "/?operation=login", 17) == 0){

				char response[178 + SESSION_TOKEN_BYTES];

				// First, if the user has a valid session, destroy it.
				const char *const cookiesStart = httpRequestFindHeader(&request, NULL, "cookie", 6, NULL);
				if(cookiesStart != NULL && memcmp(cookiesStart, "token=", 6) == 0){
					// Destroy the session.
					dbDestroy(&s->db, cookiesStart+6);
				}

				// Now validate the login request.
				if(memcmp(request.targetStart+17, "&username=", 10) == 0){

					char username[USER_NAME_BYTES];

					const char *read = request.targetStart+27;
					const char *read_end = strchr(read, '&');
					char *write = username;
					char *write_end = &username[USER_NAME_BYTES];

					memset(username, 0, USER_NAME_BYTES);

					// Parse the username.
					userParseURIString(read, read_end, write, write_end);

					read = read_end;
					if(memcmp(read, "&password=", 10) == 0){

						char password[USER_PASSWORD_BYTES];
						char token[SESSION_TOKEN_BYTES];

						read += 10;
						read_end = strpbrk(read, " \r\n&");
						write = password;
						write_end = &password[USER_PASSWORD_BYTES];

						memset(password, 0, USER_PASSWORD_BYTES);
						memset(token, 0, SESSION_TOKEN_BYTES);

						// Parse the password.
						userParseURIString(read, read_end, write, write_end);

						if(dbLogin(&s->db, username, password, token) == 0){

							// Build the body and cookie header.
							char cookie[21 + SESSION_TOKEN_BYTES];
							memcpy(response, "Login successful.", 17);
							memcpy(cookie, "Set-Cookie: token=", 18);
							memcpy(cookie + 18, token, SESSION_TOKEN_BYTES);
							memcpy(cookie + 18 + SESSION_TOKEN_BYTES, ";\r\n", 3);

							// Login successful. Send the user back the token as a cookie.
							httpResponse(
								response, 17,
								cookie, 21 + SESSION_TOKEN_BYTES,
								"text/html; charset=UTF-8", 24
							);
							ssSendDataTCP(client->handle, response);

							#ifdef SOCKET_DEBUG
							printf("Sent:\n%s\n", response);
							{
								char ip[46];
								inet_ntop(
									client->address.ss_family,
									(client->address.ss_family == AF_INET ?
									(void *)(&((struct sockaddr_in *)&client->address)->sin_addr) :
									(void *)(&((struct sockaddr_in6 *)&client->address)->sin6_addr)),
									ip, sizeof(ip)
								);
								printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&client->address)->sin_port, (unsigned long)client->id);
							}
							#endif

							// Close the connection.
							socketclose(client->handle->fd);
							scRemoveSocket(&s->ss.connectionHandler, client);
							return;

						}

					}

				}

				// Login failed.
				memcpy(response, "Login failed. This is either a problem with your form or an internal server error.", 82);
				httpResponse(response, 82, NULL, 0, "text/html; charset=UTF-8", 24);
				ssSendDataTCP(client->handle, response);

				#ifdef SOCKET_DEBUG
				printf("Sent:\n%s\n", response);
				{
					char ip[46];
					inet_ntop(
						client->address.ss_family,
						(client->address.ss_family == AF_INET ?
						(void *)(&((struct sockaddr_in *)&client->address)->sin_addr) :
						(void *)(&((struct sockaddr_in6 *)&client->address)->sin6_addr)),
						ip, sizeof(ip)
					);
					printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&client->address)->sin_port, (unsigned long)client->id);
				}
				#endif

				// Close the connection.
				socketclose(client->handle->fd);
				scRemoveSocket(&s->ss.connectionHandler, client);
				return;

			}

		// POST request.
		}else if(request.methodLength == 4){

			// POST request received from registration client.
			if(memcmp(request.contentStart, "operation=register", 18) == 0){

				char response[164];

				// Validate the registration request.
				if(memcmp(request.contentStart+18, "&username=", 10) == 0){

					char username[USER_NAME_BYTES];

					const char *read = request.contentStart+28;
					const char *read_end = strchr(read, '&');
					char *write = username;
					char *write_end = &username[USER_NAME_BYTES];

					memset(username, 0, USER_NAME_BYTES);

					// Parse the username.
					userParseURIString(read, read_end, write, write_end);

					read = read_end;
					if(memcmp(read, "&password=", 10) == 0){

						char avatar[FILE_MAX_PATH_LENGTH];
						char password[USER_PASSWORD_BYTES];
						char token[SESSION_TOKEN_BYTES];

						read += 10;
						read_end = strpbrk(read, " \r\n&");
						write = password;
						write_end = &password[USER_PASSWORD_BYTES];

						memset(avatar, 0, FILE_MAX_PATH_LENGTH);
						memset(password, 0, USER_PASSWORD_BYTES);
						memset(token, 0, SESSION_TOKEN_BYTES);

						// Parse the password.
						userParseURIString(read, read_end, write, write_end);

						// User may have specified an avatar URL.
						read = read_end;
						read_end = strpbrk(read, " \r\n&");
						if(read_end > read && memcmp(read, "&avatar=", 8) == 0){
							memcpy(avatar, read+8, read_end-read-8);
						}

						if(dbRegister(&s->db, username, password, avatar) == 0){

							// Build the body and cookie header.
							memcpy(response, "Registration successful.", 24);

							// Login successful. Send the user back the token as a cookie.
							httpResponse(response, 24, NULL, 0, "text/html; charset=UTF-8", 24);
							ssSendDataTCP(client->handle, response);

							#ifdef SOCKET_DEBUG
							printf("Sent:\n%s\n", response);
							{
								char ip[46];
								inet_ntop(
									client->address.ss_family,
									(client->address.ss_family == AF_INET ?
									(void *)(&((struct sockaddr_in *)&client->address)->sin_addr) :
									(void *)(&((struct sockaddr_in6 *)&client->address)->sin6_addr)),
									ip, sizeof(ip)
								);
								printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&client->address)->sin_port, (unsigned long)client->id);
							}
							#endif

							// Close the connection.
							socketclose(client->handle->fd);
							scRemoveSocket(&s->ss.connectionHandler, client);
							return;

						}

					}

				}

				// Registration failed.
				memcpy(response, "Registration failed. This is either a problem with your form or an internal server error.", 89);
				httpResponse(response, 89, NULL, 0, "text/html; charset=UTF-8", 24);
				ssSendDataTCP(client->handle, response);

				#ifdef SOCKET_DEBUG
				printf("Sent:\n%s\n", response);
				{
					char ip[46];
					inet_ntop(
						client->address.ss_family,
						(client->address.ss_family == AF_INET ?
						(void *)(&((struct sockaddr_in *)&client->address)->sin_addr) :
						(void *)(&((struct sockaddr_in6 *)&client->address)->sin6_addr)),
						ip, sizeof(ip)
					);
					printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&client->address)->sin_port, (unsigned long)client->id);
				}
				#endif

				// Close the connection.
				socketclose(client->handle->fd);
				scRemoveSocket(&s->ss.connectionHandler, client);
				return;

			// POST request received from Futurism.
			}else if(memcmp(request.contentStart, "token=", 6) == 0){

				session sesh;
				char response[SOCKET_MAX_BUFFER_SIZE];
				size_t response_length = SOCKET_MAX_BUFFER_SIZE;

				if(
					dbAuthenticate(&s->db, request.contentStart+6, &sesh) == 0 &&
					dbUserData(&s->db, sesh.id, response, &response_length) == 0
				){

					// Token from POST request sent and authenticated.
					// Package and send response.
					httpResponse(response, response_length, NULL, 0, "application/json", 16);
					ssSendDataTCP(client->handle, response);

					#ifdef SOCKET_DEBUG
					printf("Sent:\n%s\n", response);
					{
						char ip[46];
						inet_ntop(
							client->address.ss_family,
							(client->address.ss_family == AF_INET ?
							(void *)(&((struct sockaddr_in *)&client->address)->sin_addr) :
							(void *)(&((struct sockaddr_in6 *)&client->address)->sin6_addr)),
							ip, sizeof(ip)
						);
						printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&client->address)->sin_port, (unsigned long)client->id);
					}
					#endif

					// Close the connection.
					socketclose(client->handle->fd);
					scRemoveSocket(&s->ss.connectionHandler, client);
					return;

				}

			}

		}

	}

	#ifdef SOCKET_DEBUG
	printf("Nothing sent.\n");
	{
		char ip[46];
		inet_ntop(
			client->address.ss_family,
			(client->address.ss_family == AF_INET ?
			(void *)(&((struct sockaddr_in *)&client->address)->sin_addr) :
			(void *)(&((struct sockaddr_in6 *)&client->address)->sin6_addr)),
			ip, sizeof(ip)
		);
		printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&client->address)->sin_port, (unsigned long)client->id);
	}
	#endif

	// Close the connection.
	socketclose(client->handle->fd);
	scRemoveSocket(&s->ss.connectionHandler, client);
	return;

}

void serverDelete(server *const __RESTRICT__ s){
	ssShutdownTCP(&s->ss.connectionHandler);
}
