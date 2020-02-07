#ifndef SOCKETCONNECTIONHANDLER_H
#define SOCKETCONNECTIONHANDLER_H

#include "socketShared.h"
#include "../shared/flags.h"
#include "../shared/return.h"
#include "../shared/qualifiers.h"
#include <stdint.h>
#include <stdlib.h>

// Various socketDetails status flags.
#define SOCKET_DETAILS_NEW_DATA     0x01  // The socket has sent data between the previous and current updates.
#define SOCKET_DETAILS_CONNECTED    0x02  // The socket has connected between the previous and current updates.
#define SOCKET_DETAILS_DISCONNECTED 0x04  // The socket has disconnected between the previous and current updates.
#define SOCKET_DETAILS_ERROR        0x08  // There has been an error with the socket between the previous and current updates.
#define SOCKET_DETAILS_TIMED_OUT    0x10  // The socket has timed out between the previous and current updates.

typedef struct {
	socketHandle *handle;                     // Pointer to the corresponding handle. NULL if inactive.
	uintptr_t id;                             // Index in the details array.
	socketAddrLength addressSize;             // The size of the socket's address, in bytes.
	struct sockaddr_storage address;          // Socket address.
	#ifdef SOCKET_MANAGE_TIMEOUTS
	uint32_t lastTick;                        // The tick that data was last received from the socket on.
	#endif
	// Remove the following eventually.
	int bufferSize;                       // The size of the last buffer received from the socket, in bytes.
	char buffer[SOCKET_MAX_BUFFER_SIZE];  // The last buffer received from the socket.
	flags_t flags;
} socketDetails;

typedef struct {
	socketDetails *details;  // Holds an array of socketDetails for both TCP and UDP.
	socketHandle *handles;   // Holds an array of struct pollfds, separate from details for TCP socket polling.
	socketHandle *handleLast;    // The handle at the very end of the array.
	socketDetails *detailsLast;  // The socket details corresponding to handleLast.
	size_t capacity;  // Total number of file descriptors allocated.
	size_t nfds;      // Number of file descriptors, specifically for TCP socket polling.
} socketConnectionHandler;

#define sdValid(details) ((details)->handle != NULL)
#ifdef SOCKET_MANAGE_TIMEOUTS
return_t sdTimedOut(const socketDetails *const __RESTRICT__ details, const uint32_t currentTick);
#else
#define sdTimedOut(details, currentTick) 0
#endif

#define scMasterHandle(sc) ((sc)->handles)
#define scMasterDetails(sc) ((sc)->details)
return_t scInit(socketConnectionHandler *const __RESTRICT__ sc, const size_t capacity, const socketHandle *const __RESTRICT__ masterHandle, const socketDetails *const __RESTRICT__ masterDetails);
return_t scAddSocket(socketConnectionHandler *const __RESTRICT__ sc, const socketHandle *const __RESTRICT__ handle, const socketDetails *const __RESTRICT__ details);
return_t scRemoveSocket(socketConnectionHandler *const __RESTRICT__ sc, socketDetails *details);
socketDetails *scSocket(socketConnectionHandler *const __RESTRICT__ sc, const uintptr_t id);
void scDelete(socketConnectionHandler *const __RESTRICT__ sc);

#endif
