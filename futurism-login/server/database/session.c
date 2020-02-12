#include "session.h"
#include "../../shared/helpers.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static __FORCE_INLINE__ void seshGenerateToken(char *token){
	const char *const tokenLast = &token[SESSION_TOKEN_BYTES];
	int *t = (int *)token;
	const int *const tLast = (int *)tokenLast;
	while(t < tLast){
		*t = rand();
		++t;
	}
	// Make sure the characters are alphanumeric.
	while(token < tokenLast){
		char c = *token;
		c %= 60;
		if(c < 0){
			c = -c;
		}
		if(c < 10){
			// Numbers.
			c += '0';
		}else if(c < 35){
			// Capital letters.
			c += 'A' - 10;
		}else{
			// Lowercase letters.
			c += 'a' - 35;
		}
		*token = c;
		++token;
	}
}

void seshGenerate(session *const __RESTRICT__ sesh, const char *const id, const size_t id_length, char *const __RESTRICT__ path, const size_t path_length){

	FILE *f;
	char token[SESSION_TOKEN_BYTES];

	sesh->last_active = time(NULL);
	if(id != sesh->id){
		// The id should already be set, so this
		// will really never be invoked.
		sesh->id_length = strnlen(sesh->id, USER_NAME_BYTES<<1);
		memcpy(sesh->id, id, sesh->id_length);
	}

	// Generate a token.
	seshGenerateToken(token);

	// Build a path to a new session file.
	memcpy(&path[path_length-SESSION_TOKEN_BYTES-10], FILE_PATH_DELIMITER_STRING"sessions"FILE_PATH_DELIMITER_STRING, 10);
	memcpy(&path[path_length-SESSION_TOKEN_BYTES], token, SESSION_TOKEN_BYTES);
	path[path_length] = '\0';

	// Create a new session file.
	while(access(path, 0) == 0){
		// It's possible that our randomly generated token already exists.
		seshGenerateToken(token);
		memcpy(&path[path_length-SESSION_TOKEN_BYTES], token, SESSION_TOKEN_BYTES);
	}
	f = fopen(path, "w");
	fwrite(sesh, 1, sizeof(session)-(USER_NAME_BYTES<<1)+sesh->id_length, f);
	fclose(f);
	// This file is not particularly sensitive.
	chmod(path, 0744);

}

return_t seshInvalid(const session *const __RESTRICT__ sesh, const time_t t){
	//return difftime(t, sesh->last_active) > SESSION_LIFETIME;
	return 0;
}
