#ifndef SOCKETUDP_H
#define SOCKETUDP_H

#include "socketServer.h"

return_t ssSendDataUDP(const socketConnectionHandler *const __RESTRICT__ connectionHandler, const socketDetails *const __RESTRICT__ details, const char *const __RESTRICT__ msg);
#ifdef SOCKET_MANAGE_TIMEOUTS
return_t ssHandleConnectionsUDP(socketConnectionHandler *const __RESTRICT__ connectionHandler, const flags_t flags, const uint32_t currentTick);
#else
return_t ssHandleConnectionsUDP(socketConnectionHandler *const __RESTRICT__ connectionHandler, const flags_t flags);
#endif
void ssShutdownUDP(socketConnectionHandler *const __RESTRICT__ connectionHandler);

#endif
