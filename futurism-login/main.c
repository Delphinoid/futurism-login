#define WIN32_LEAN_AND_MEAN
#include "memory/memoryManager.h"
#include "server/server.h"
#include "shared/flags.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

server s;

void cleanup(){
	serverDelete(&s);
	ssCleanup();
	memMngrDelete();
}

int main(int argc, char **argv){

	char buffer[SOCKET_MAX_BUFFER_SIZE];
	int buffer_size;

	atexit(cleanup);
	srand(time(NULL));

	// Initialize the memory manager and the server.
	if(memMngrInit(MEMORY_MANAGER_DEFAULT_VIRTUAL_HEAP_SIZE, 1) < 0 || !ssStartup() || !serverInit(&s)){
		return 1;
	}

	// Main loop.
	for(;;){

		socketDetails *i;
		size_t j;

		// Handle TCP connections.
		i = s.ss.detailsLast;
		// Poll TCP connections.
		if((j = ssPollTCP(&s.ss)) < 0){
			// Fatal error.
			break;
		}
		// Check if a new client connected.
		if(s.ss.detailsLast != i){
			#ifdef SOCKET_DEBUG
			// Incoming request.
			// Socket has connected.
			char ip[46];
			inet_ntop(
				i->address.ss_family,
				(i->address.ss_family == AF_INET ?
				(void *)(&((struct sockaddr_in *)&i->address)->sin_addr) :
				(void *)(&((struct sockaddr_in6 *)&i->address)->sin6_addr)),
				ip, sizeof(ip)
			);
			printf("Opening TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&i->address)->sin_port, (unsigned long)i->id);
			#endif
		}
		// Receive data from clients.
		i = s.ss.details+1;
		while(j > 0){
			if(sdValid(i)){

				if(flagsAreSet(i->handle->revents, POLLIN)){

					// Receives up to MAX_BUFFER_SIZE bytes of data from a client socket and stores it in buffer.
					buffer_size = recv(i->handle->fd, buffer, SOCKET_MAX_BUFFER_SIZE, 0);

					if(buffer_size == -1){
						// Error encountered, disconnect problematic socket.
						#ifdef SOCKET_DEBUG
						ssReportError("recv()", ssError);
						#endif
						ssDisconnect(&s.ss, i);
					}else if(buffer_size == 0){
						#ifdef SOCKET_DEBUG
						// Socket has disconnected.
						char ip[46];
						inet_ntop(
							i->address.ss_family,
							(i->address.ss_family == AF_INET ?
							(void *)(&((struct sockaddr_in *)&i->address)->sin_addr) :
							(void *)(&((struct sockaddr_in6 *)&i->address)->sin6_addr)),
							ip, sizeof(ip)
						);
						printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&i->address)->sin_port, (unsigned long)i->id);
						ssDisconnect(&s.ss, i);
						#endif
					}else{
						// Handle request and reset state flags.
						serverHandleRequest(&s, i, buffer, buffer_size);
					}

					--j;

				}else if(flagsAreSet(i->handle->revents, POLLHUP)){
					#ifdef SOCKET_DEBUG
					// Socket has disconnected.
					char ip[46];
					inet_ntop(
						i->address.ss_family,
						(i->address.ss_family == AF_INET ?
						(void *)(&((struct sockaddr_in *)&i->address)->sin_addr) :
						(void *)(&((struct sockaddr_in6 *)&i->address)->sin6_addr)),
						ip, sizeof(ip)
					);
					printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&i->address)->sin_port, (unsigned long)i->id);
					ssDisconnect(&s.ss, i);
					#endif
					--j;
				}

			}
			++i;
		}

	}

	// Clean up.
	cleanup();

	return 0;

}
