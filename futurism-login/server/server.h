#ifndef SERVER_H
#define SERVER_H

#include "database/database.h"
#include "../socket/socketTCP.h"

typedef struct {

	// Database settings.
	database db;

	// Socket server settings.
	socketServer ss;

} server;

return_t serverInit(server *const __RESTRICT__ s);
void serverHandleRequest(server *const __RESTRICT__ s, socketDetails *const __RESTRICT__ client);
void serverDelete(server *const __RESTRICT__ s);

#endif
