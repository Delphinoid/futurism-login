#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include "socketShared.h"
#include "../shared/qualifiers.h"
#include <stdint.h>
#include <stdlib.h>

// If initializing with SOCKET_SERVER_ALLOCATE_NOTHING,
// there is no need to call ssDelete().
#define SOCKET_SERVER_ALLOCATE_NOTHING    0x00
#define SOCKET_SERVER_ALLOCATE_EVERYTHING  0x01
#define SOCKET_SERVER_ALLOCATE_LIGHTWEIGHT 0x02

typedef struct {
	int type;
	int protocol;
	int allocate;
	char ip[45];
	uint16_t port;
	int backlog;  // Maximum number of queued connections for listen().
	size_t connections;  // Initial maximum number of connected sockets. May be subject to resizes if SOCKET_REALLOCATE is specified.
} ssConfig;

typedef struct {
	socketHandle *handle;             // Pointer to the corresponding handle. NULL if inactive.
	uintptr_t id;                     // Index in the details array.
	struct sockaddr_storage address;  // Socket address.
	socketAddrLength addressSize;     // The size of the socket's address, in bytes.
} socketDetails;

typedef struct {
	socketDetails *details;  // Holds an array of socketDetails for both TCP and UDP.
	socketHandle *handles;   // Holds an array of struct pollfds, separate from details for TCP socket polling.
	socketHandle *handleLast;    // The handle at the very end of the array.
	socketDetails *detailsLast;  // The socket details corresponding to handleLast.
	size_t capacity;  // Total number of file descriptors allocated.
	size_t nfds;      // Number of file descriptors, specifically for TCP socket polling.
} socketServer;

typedef struct {
	socketHandle handle;
	struct sockaddr_storage address;
	socketAddrLength addressSize;
} socketMaster;

// Socket functions shared by TCP and UDP sockets.
#ifdef SOCKET_DEBUG
void ssReportError(const char *const __RESTRICT__ failedFunction, const int errorCode);
#endif
int ssInit(void *const __RESTRICT__ server, ssConfig config);
int ssDisconnect(socketServer *const __RESTRICT__ server, socketDetails *const clientDetails);
#define ssDelete(server) scDelete(server)

#ifdef _WIN32
	int ssStartup();
	void ssCleanup();
#else
	#define ssStartup() 1
	#define ssCleanup() ;
#endif

#define sdValid(details) ((details)->handle != NULL)
#ifdef SOCKET_MANAGE_TIMEOUTS
int sdTimedOut(const socketDetails *const __RESTRICT__ details, const uint32_t currentTick);
#else
#define sdTimedOut(details, currentTick) 0
#endif

#define scMasterHandle(sc) ((sc)->handles)
#define scMasterDetails(sc) ((sc)->details)
int scInit(socketServer *const __RESTRICT__ sc, const size_t capacity, const socketHandle *const __RESTRICT__ masterHandle, const socketDetails *const __RESTRICT__ masterDetails);
int scAddSocket(socketServer *const __RESTRICT__ sc, const socketHandle *const __RESTRICT__ handle, const socketDetails *const __RESTRICT__ details);
int scRemoveSocket(socketServer *const __RESTRICT__ sc, socketDetails *const details);
socketDetails *scSocket(socketServer *const __RESTRICT__ sc, const uintptr_t id);
void scDelete(socketServer *const __RESTRICT__ sc);

#endif
