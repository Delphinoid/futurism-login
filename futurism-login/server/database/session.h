#ifndef SESSION_H
#define SESSION_H

#include "user.h"
#include "../../shared/return.h"
#include "../../shared/qualifiers.h"
#include <time.h>

#ifndef SESSION_LIFETIME
	// Time in seconds before a session will time out.
	#define SESSION_LIFETIME 3600.0
#endif

#define SESSION_TOKEN_BYTES 32

typedef struct {

	// Last activity.
	time_t last_active;

	// Encoded username.
	char id[USER_NAME_BYTES<<1];

} session;

void seshGenerate(session *const __RESTRICT__ sesh, const char *const id, char *const __RESTRICT__ path, const size_t path_length);
return_t seshInvalid(const session *const __RESTRICT__ sesh, const time_t t);

#endif
