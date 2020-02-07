#include "database.h"
#include "../../memory/memoryManager.h"
#include "../../shared/helpers.h"
#include <string.h>
#include <stdio.h>

return_t dbRegister(database *const __RESTRICT__ db, const char *const __RESTRICT__ username, const char *const __RESTRICT__ password, const char *const __RESTRICT__ avatar){

	FILE *f;
	int error;

	char hash[USER_PASSWORD_BYTES];

	// Length of the path to the user directory.
	const size_t user_name_length = strlen(username);
	const size_t user_directory_length = db->path_length + 7 + (user_name_length << 1);

	// Allocate one big string that holds each required path.
	// We only really care about the users subdirectory.
	// The name of the longest file in the user directory is the
	// password file at 8 characters, plus an extra character for
	// the delimiter. Allocate these extra characters so that we
	// may reuse this allocation later on. Also allocate an extra
	// byte for the null terminator.
	char *path = memAllocate((user_directory_length + 9 + 1) * sizeof(char));

	// Build user directory path.
	memcpy(path, &db->path, db->path_length);
	memcpy(&path[db->path_length], FILE_PATH_DELIMITER_STRING"users"FILE_PATH_DELIMITER_STRING, 7);
	userEncodeName(username, &username[user_name_length], &path[db->path_length+7]);
	path[user_directory_length] = '\0';

	// Create a directory. Only the owner can enter and read / modify
	// the contents of the user directory, with everyone else only having
	// read access. If this fails, the user probably already exists.
	if((error = mkdir(path, 0744))){
		// The user probably already exists.
		memFree(path);
		return 1;
	}

	// Create the user data document.
	// First modify the directory string to include the file name.
	memcpy(&path[user_directory_length], FILE_PATH_DELIMITER_STRING"data\0", 6);

	f = fopen(path, "w");
	// Add the null terminator back for printing the user ID in fprintf.
	path[user_directory_length] = '\0';
	// Generate a simple JSON file for the game server.
	if(f == NULL || (error = fprintf(f,
		"{\n"
		"\t\"user_name\": \"%s\",\n"
		"\t\"user_id\": \"%s\",\n"
		"\t\"avatar\": \"%s\",\n"
		"\t\"group\": %u,\n"
		"\t\"beta\": %s\n"
		"}",
		username, &path[db->path_length+7], avatar, USER_GROUP_DEFAULT, USER_BETA_DEFAULT ? "true" : "false"
	)) <= 0){
		fclose(f);
		memFree(path);
		return 2;
	}
	fclose(f);
	// This file is not particularly sensitive.
	chmod(path, 0744);

	// Create the encrypted user password document.
	// First modify the directory string to include the file name.
	memcpy(&path[user_directory_length], FILE_PATH_DELIMITER_STRING"password\0", 10);

	f = fopen(path, "w");
	if(f == NULL || (userHashPassword(password, hash), error = fputs(hash, f)) <= 0){
		fclose(f);
		memFree(path);
		return 3;
	}
	fclose(f);
	// This file is very sensitive. Only the owner can modify it.
	chmod(path, 0700);

	// Clean up and return.
	memFree(path);
	return 0;

}

return_t dbLogin(database *const __RESTRICT__ db, const char *const __RESTRICT__ username, const char *const __RESTRICT__ password, char *const __RESTRICT__ token){

	FILE *f;
	session s;

	// Length of the path to the user directory.
	const size_t user_name_length = strlen(username);
	const size_t user_directory_length = db->path_length + 7 + (user_name_length << 1);
	const size_t password_path_length = user_directory_length + 10;

	// Length of the path to the session directory.
	const size_t session_path_length = db->path_length + 10 + SESSION_TOKEN_BYTES;

	// Allocate a buffer for the password and session paths.
	char *path = memAllocate(
		(password_path_length > session_path_length ? password_path_length : session_path_length) * sizeof(char)
	);

	// Build password path.
	memcpy(path, &db->path, db->path_length);
	memcpy(&path[db->path_length], FILE_PATH_DELIMITER_STRING"users"FILE_PATH_DELIMITER_STRING, 7);
	// Store the encoded username for later on.
	userEncodeName(username, &username[user_name_length], s.id);
	memcpy(&path[db->path_length+7], s.id, user_name_length << 1);
	memcpy(&path[user_directory_length], FILE_PATH_DELIMITER_STRING"password\0", 10);

	// Check if the user exists. If we couldn't open the
	// password file, this is as good as true.
	f = fopen(path, "r");
	if(f == NULL){
		// The user probably does not exist.
		memFree(path);
		return 1;
	}else{

		char hash[USER_PASSWORD_BYTES];
		char password_buffer[USER_PASSWORD_BYTES];
		size_t password_bytes;
		if(
			!(password_bytes = fread(&password_buffer, sizeof(char), USER_PASSWORD_BYTES, f)) ||
			(userHashPassword(password, hash), strncmp(hash, password_buffer, password_bytes))
		){
			// The password is probably incorrect.
			fclose(f);
			memFree(path);
			return 2;
		}else{

			fclose(f);

			// Build session path.
			memcpy(path, &db->path, db->path_length);
			// Generate a new session.
			seshGenerate(&s, s.id, path, session_path_length);
			// Copy over the new token.
			memcpy(token, &path[session_path_length-SESSION_TOKEN_BYTES], SESSION_TOKEN_BYTES);

			memFree(path);

		}

	}

	return 0;

}

return_t dbAuthenticate(database *const __RESTRICT__ db, const char *const __RESTRICT__ token, session *const __RESTRICT__ sesh){

	FILE *f;

	// Length of the path to the session directory.
	const size_t session_path_length = db->path_length + 10 + SESSION_TOKEN_BYTES;

	// Allocate a buffer for the session directory.
	char *path = memAllocate((session_path_length + 1) * sizeof(char));

	// Build session path.
	memcpy(path, &db->path, db->path_length);
	memcpy(&path[db->path_length], FILE_PATH_DELIMITER_STRING"sessions"FILE_PATH_DELIMITER_STRING, 10);
	memcpy(&path[db->path_length+10], token, SESSION_TOKEN_BYTES);
	path[session_path_length] = '\0';

	// Check if the session exists. If we couldn't open the
	// session file, this is as good as true.
	f = fopen(path, "r+");
	memFree(path);
	if(f == NULL){
		// The session probably does not exist.
		return 1;
	}else{

		// Check if the session is still valid.
		session s;
		const time_t t = time(NULL);

		if(!fread(&s, 1, sizeof(session), f) || seshInvalid(&s, t)){
			// The session has probably timed out.
			fclose(f);
			remove(path);
			return 2;
		}

		// If it is, update it to reflect the current time.
		s.last_active = t;
		fseek(f, 0, SEEK_SET);
		fwrite(&s, 1, sizeof(session), f);
		fclose(f);

		// Return the valid session.
		if(sesh != NULL){
			*sesh = s;
		}

	}

	return 0;

}
#include <errno.h>
return_t dbUserData(database *const __RESTRICT__ db, const char *const __RESTRICT__ id, char *const __RESTRICT__ data, size_t *const __RESTRICT__ data_length){

	FILE *f;

	// Length of the path to the user directory.
	const size_t user_id_length = strnlen(id, USER_NAME_BYTES<<1);
	const size_t user_directory_length = db->path_length + 7 + user_id_length;
	const size_t user_data_path_length = user_directory_length + 6;

	// Allocate a buffer for the password and session paths.
	char *const path = memAllocate(user_data_path_length * sizeof(char));

	// Build data path.
	memcpy(path, &db->path, db->path_length);
	memcpy(&path[db->path_length], FILE_PATH_DELIMITER_STRING"users"FILE_PATH_DELIMITER_STRING, 7);
	memcpy(&path[db->path_length+7], id, user_id_length);
	memcpy(&path[user_directory_length], FILE_PATH_DELIMITER_STRING"data\0", 6);

	f = fopen(path, "r");
	memFree(path);
	if(f == NULL){
		// User probably doesn't exist.
		return 1;
	}else{
		// Read the user data.
		*data_length = fread(data, sizeof(char), *data_length, f);
	}

	return 0;

}

return_t dbDestroy(database *const __RESTRICT__ db, const char *const __RESTRICT__ token){

	// Length of the path to the session directory.
	const size_t session_path_length = db->path_length + 10 + SESSION_TOKEN_BYTES;

	// Allocate a buffer for the session directory.
	char *path = memAllocate((session_path_length + 1) * sizeof(char));

	// Build session path.
	memcpy(path, &db->path, db->path_length);
	memcpy(&path[db->path_length], FILE_PATH_DELIMITER_STRING"sessions"FILE_PATH_DELIMITER_STRING, 10);
	memcpy(&path[db->path_length+10], token, SESSION_TOKEN_BYTES);
	path[session_path_length] = '\0';

	// Destroy the session.
	if(remove(path) == -1){
		// The session probably does not exist.
		memFree(path);
		return 1;
	}
	memFree(path);
	return 0;

}
