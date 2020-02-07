#ifndef USER_H
#define USER_H

#ifndef USER_GROUP_DEFAULT
	#define USER_GROUP_DEFAULT 1
#endif

#ifndef USER_BETA_DEFAULT
	#define USER_BETA_DEFAULT 0
#endif

#define USER_GROUP_GUEST         0
#define USER_GROUP_USER          1
#define USER_GROUP_MODERATOR     2
#define USER_GROUP_ADMINISTRATOR 3

#define USER_NAME_BYTES     32
#define USER_PASSWORD_BYTES 64

void userParseURIString(const char *read, const char *read_end, char *write, const char *const write_end);
void userEncodeName(const char *name, const char *const end, char *encoded);
void userHashPassword(const char *const password, char *const hash);

#endif
