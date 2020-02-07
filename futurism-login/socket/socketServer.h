#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include "socketConnectionHandler.h"

typedef struct {
	int type;
	int protocol;
	char ip[45];
	uint16_t port;
	int backlog;  // Maximum number of queued connections for listen().
	size_t connections;  // Initial maximum number of connected sockets. May be subject to resizes if SOCKET_REALLOCATE is specified.
} ssConfig;

typedef struct {
	int type;
	int protocol;
	socketConnectionHandler connectionHandler;
} socketServer;

// Socket functions shared by TCP and UDP sockets.
#ifdef SOCKET_DEBUG
void ssReportError(const char *const __RESTRICT__ failedFunction, const int errorCode);
#endif
return_t ssInit(socketServer *const __RESTRICT__ server, ssConfig config);
#ifdef SOCKET_MANAGE_TIMEOUTS
void ssCheckTimeouts(socketConnectionHandler *const __RESTRICT__ sc, const uint32_t currentTick);
#else
#define ssCheckTimeouts(sc, currentTick) ;
#endif

#ifdef _WIN32
	return_t ssStartup();
	void ssCleanup();
#else
	#define ssStartup() 1
	#define ssCleanup() ;
#endif

#endif
