#ifndef DATABASE_H
#define DATABASE_H

#include "session.h"
#include <stddef.h>

typedef struct {

	// Length of the path.
	size_t path_length;

	// Path to the database.
	// This is the first character of the path;
	// the next path_length bytes in memory are
	// the remaining characters of the path.
	char path;

} database;

return_t dbRegister(database *const __RESTRICT__ db, const char *const __RESTRICT__ username, const char *const __RESTRICT__ password, const char *const __RESTRICT__ avatar);
return_t dbLogin(database *const __RESTRICT__ db, const char *const __RESTRICT__ username, const char *const __RESTRICT__ password, char *const __RESTRICT__ token);
return_t dbAuthenticate(database *const __RESTRICT__ db, const char *const __RESTRICT__ token, session *const __RESTRICT__ sesh);
return_t dbUserData(database *const __RESTRICT__ db, const char *const __RESTRICT__ id, char *const __RESTRICT__ data, size_t *const __RESTRICT__ data_length);
return_t dbDestroy(database *const __RESTRICT__ db, const char *const __RESTRICT__ token);

#endif
