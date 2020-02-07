#ifndef SOCKETTCP_H
#define SOCKETTCP_H

#include "socketServer.h"

return_t ssSendDataTCP(const socketHandle *const __RESTRICT__ clientHandle, const char *const __RESTRICT__ msg);
return_t ssDisconnectSocketTCP(socketConnectionHandler *const __RESTRICT__ connectionHandler, socketDetails *clientDetails);
#ifdef SOCKET_MANAGE_TIMEOUTS
return_t ssHandleConnectionsTCP(socketConnectionHandler *const __RESTRICT__ connectionHandler, const flags_t flags, const uint32_t currentTick);
#else
return_t ssHandleConnectionsTCP(socketConnectionHandler *const __RESTRICT__ connectionHandler, const flags_t flags);
#endif
void ssShutdownTCP(socketConnectionHandler *const __RESTRICT__ connectionHandler);

#endif
