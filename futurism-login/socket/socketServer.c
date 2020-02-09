#include "socketServer.h"
#include <string.h>

#ifndef _WIN32
#include <fcntl.h>
#endif

#ifndef SOCKET_USE_MALLOC
#include "../memory/memoryManager.h"
#endif

#ifdef SOCKET_DEBUG
#include <stdio.h>
void ssReportError(const char *const __RESTRICT__ failedFunction, const int errorCode){
	printf("\nSocket function %s has failed: %i\nFor troubleshooting, please refer to the manual for your platform.\n\n", failedFunction, errorCode);
}
#endif

static __FORCE_INLINE__ int ssSetNonBlockMode(const int fd, unsigned long mode){
	#ifdef _WIN32
		return !ioctlsocket(fd, FIONBIO, &mode);
	#else
		int flags = fcntl(fd, F_GETFL, 0);
		if(flags < 0){
			return 0;
		}
		flags = mode ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
		return !fcntl(fd, F_SETFL, flags);
	#endif
}

static __FORCE_INLINE__ int ssGetAddressFamily(const char *const __RESTRICT__ ip){
	char buffer[16];
	if(inet_pton(AF_INET, ip, buffer)){
		return AF_INET;
	}else if(inet_pton(AF_INET6, ip, buffer)) {
		return AF_INET6;
	}
	return -1;
}

int ssInit(void *const __RESTRICT__ server, ssConfig config){

	// Initialize server IP, port, type and protocol, then load the server config.
	struct pollfd masterHandle;
	socketDetails masterDetails;
	int af;

	#ifdef SOCKET_DEBUG
	puts("Initializing server...");
	#endif

	// Create a socket prototype for the master socket.
	//
	// socket(address family, type, protocol)
	// address family = AF_UNSPEC, which can be either IPv4 or IPv6, AF_INET, which is IPv4 or AF_INET6, which is IPv6.
	// type = SOCKET_STREAM, which uses TCP or SOCKET_DGRAM, which uses UDP.
	// protocol = IPPROTO_TCP, which specifies to use TCP or IPPROTO_UDP, which specifies to use UDP.
	af = ssGetAddressFamily(config.ip);
	if(af == -1){
		af = SOCKET_DEFAULT_ADDRESS_FAMILY;
	}
	masterHandle.fd = socket(af, config.type, config.protocol);
	if(masterHandle.fd == INVALID_SOCKET){
		// If socket() failed, abort.
		#ifdef SOCKET_DEBUG
		ssReportError("socket()", ssError);
		#endif
		return 0;
	}
	masterHandle.events = POLLIN;
	masterHandle.revents = 0;

	// If SOCKET_POLL_TIMEOUT isn't negative, we want a timeout for recfrom().
	if(SOCKET_POLL_TIMEOUT > 0){
		// Set SO_RCVTIMEO.
		const unsigned long timeout = SOCKET_POLL_TIMEOUT;
		if(setsockopt(masterHandle.fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(unsigned long)) == SOCKET_ERROR){
			#ifdef SOCKET_DEBUG
			ssReportError("setsockopt()", ssError);
			#endif
			return 0;
		}
	}else if(SOCKET_POLL_TIMEOUT == 0){
		ssSetNonBlockMode(masterHandle.fd, 1);
	}

	// Bind the master socket to the host address.
	// Address is IPv4.
	if(af == AF_INET){

		// Create the sockaddr_in structure using the master socket's address family and the supplied IP / port.
		struct sockaddr_in serverAddress4;
		if(!inet_pton(af, config.ip, (char*)&(serverAddress4.sin_addr))){
			serverAddress4.sin_addr.s_addr = INADDR_ANY;
			memcpy(config.ip, "INADDR_ANY\0", 11);
		}
		serverAddress4.sin_family = af;
		// htons() converts the port from big-endian to little-endian for sockaddr_in.
		serverAddress4.sin_port = htons(config.port);
		masterDetails.address = *((struct sockaddr_storage *)&serverAddress4);

	// Address is IPv6.
	}else if(af == AF_INET6){

		// Create the sockaddr_in6 structure using the master socket's address family and the supplied IP / port.
		struct sockaddr_in6 serverAddress6;
		// Set everything in sockaddr_in6 to 0, as there are a number of fields we don't otherwise set.
		memset(&serverAddress6, 0, sizeof(struct sockaddr_in6));
		if(!inet_pton(af, config.ip, (char*)&(serverAddress6.sin6_addr))){
			serverAddress6.sin6_addr = in6addr_any;
			memcpy(config.ip, "in6addr_any\0", 12);
		}
		serverAddress6.sin6_family = af;
		// htons() converts the port from big-endian to little-endian for sockaddr_in6.
		serverAddress6.sin6_port = htons(config.port);
		masterDetails.address = *((struct sockaddr_storage *)&serverAddress6);

	}

	// Check result of bind()
	if(bind(masterHandle.fd, (struct sockaddr *)&masterDetails.address, sizeof(struct sockaddr_storage)) == SOCKET_ERROR){  // If bind() failed, abort.
		#ifdef SOCKET_DEBUG
		ssReportError("bind()", ssError);
		#endif
		return 0;
	}

	// If the server is operating over TCP, set the master socket's state to "listen" so it will start listening for incoming connections from sockets.
	if(config.protocol == IPPROTO_TCP){
		// SOMAXCONN = automatically choose maximum number of pending connections, different across systems.
		if(listen(masterHandle.fd, config.backlog) == SOCKET_ERROR){
			#ifdef SOCKET_DEBUG
			ssReportError("listen()", ssError);
			#endif
			return 0;
		}
	}

	masterDetails.id = 0;
	if(config.allocate == SOCKET_SERVER_ALLOCATE_EVERYTHING){

		// Initialize the connection handler.
		((socketServer *const)server)->details = NULL;
		if(!scInit(server, config.connections, &masterHandle, &masterDetails)){
			#ifdef SOCKET_DEBUG
			puts("Error: the socket connection handler could not be initialized.\n");
			#endif
			return 0;
		}

	}else if(config.allocate == SOCKET_SERVER_ALLOCATE_LIGHTWEIGHT){

		// This should use a free-list or something.

	}else{

		// Don't allocate a connection handler. Just return the master socket.
		// The programmer can keep track of connections themselves if they wish.
		((socketMaster *const)server)->handle = masterHandle;
		((socketMaster *const)server)->address = masterDetails.address;
		((socketMaster *const)server)->addressSize = masterDetails.addressSize;

	}

	#ifdef SOCKET_DEBUG
	printf("Socket successfully initialized on %s:%u.\n\n", config.ip, config.port);
	#endif
	return 1;

}

int ssDisconnect(socketServer *const __RESTRICT__ server, socketDetails *const clientDetails){
	socketclose(clientDetails->handle->fd);
	return scRemoveSocket(server, clientDetails);
}

#ifdef _WIN32
int ssStartup(){
	// Initialize Winsock.
	WSADATA wsaData;
	int initError = WSAStartup(WINSOCK_VERSION, &wsaData);

	if(initError != 0){  // If Winsock did not initialize correctly, abort.
		#ifdef SOCKET_DEBUG
		ssReportError("WSAStartup()", initError);
		#endif
		return 0;
	}
	return 1;
}
void ssCleanup(){
	WSACleanup();
}
#endif

static __FORCE_INLINE__ uintptr_t scSocketID(const socketServer *const __RESTRICT__ sc, const socketDetails *const details){
	return (((uintptr_t)details) - ((uintptr_t)sc->details)) / sizeof(socketDetails);
}

socketDetails *scSocket(socketServer *const __RESTRICT__ sc, const uintptr_t id){
	return &sc->details[id];
}

int scInit(socketServer *const __RESTRICT__ sc, const size_t capacity, const socketHandle *const __RESTRICT__ masterHandle, const socketDetails *const __RESTRICT__ masterDetails){

	socketDetails *details;
	socketHandle *handle;
	const socketDetails *detailsLast;

	// Initialize socketDetails.
	details =
	#ifdef SOCKET_USE_MALLOC
		malloc(capacity * (sizeof(socketDetails) + sizeof(socketHandle)));
	#else
		memAllocate(capacity * (sizeof(socketDetails) + sizeof(socketHandle)));
	#endif
	if(details == NULL){
		// Memory allocation failure.
		return -1;
	}
	sc->details = details;
	sc->detailsLast = details-1;

	// Initialize socketHandles.
	handle = (socketHandle *)&details[capacity];
	sc->handles = handle;
	sc->handleLast = handle-1;

	// Initialize the fd array.
	detailsLast = (socketDetails *)handle;
	while(details < detailsLast){
		*((socketDetails **)handle) = details;
		details->handle = NULL;
		details->id = scSocketID(sc, details);
		++handle; ++details;
	}

	sc->capacity = capacity;
	sc->nfds = 0;

	// Add the master socket.
	return scAddSocket(sc, masterHandle, masterDetails);

}

static __FORCE_INLINE__ int scResize(socketServer *const __RESTRICT__ sc){

	uintptr_t offset;
	socketDetails *details;
	socketHandle *handle;
	socketHandle *handleOld;
	const socketDetails *detailsLast;

	size_t detailsLeft = sc->nfds;
	size_t handlesLeft = detailsLeft;

	// Resize the buffer.
	const size_t capacity = sc->capacity << 1;
	void *const buffer =
	#ifdef SOCKET_USE_MALLOC
		realloc(sc->details, capacity * (sizeof(socketDetails) + sizeof(socketHandle)));
	#else
		memReallocate(sc->details, capacity * (sizeof(socketDetails) + sizeof(socketHandle)));
	#endif
	if(buffer == NULL){
		// Memory allocation failure.
		return -1;
	}
	details = buffer;
	handle = (socketHandle *)&details[capacity];
	handleOld = (socketHandle *)&details[sc->capacity];
	sc->capacity = capacity;

	// Get the offset to add to each details' handle pointer.
	// This should also work if the allocated space is before
	// the old space in memory.
	offset = ((uintptr_t)handle) - ((uintptr_t)sc->handles);

	// Shift member pointers.
	sc->details = details;
	sc->detailsLast = (socketDetails *)(((uintptr_t)sc->detailsLast) + ((uintptr_t)details) - ((uintptr_t)sc->details));
	sc->handles = handle;
	sc->handleLast = (socketHandle *)(((uintptr_t)sc->handleLast) + offset);

	// Fix element pointers.
	detailsLast = (socketDetails *)handle;
	while(details < detailsLast){
		if(handlesLeft > 0){
			*handle = *handleOld;
			++handleOld;
			--handlesLeft;
		}else{
			*((socketDetails **)handle) = details;
		}
		if(detailsLeft > 0){
			if(details->handle != NULL){
				details->handle = (socketHandle *)(((uintptr_t)details->handle) + offset);
				--detailsLeft;
			}
		}else{
			details->handle = NULL;
			details->id = scSocketID(sc, details);
		}
		++details; ++handle;
	}

	return 1;

}

int scAddSocket(socketServer *const __RESTRICT__ sc, const socketHandle *const __RESTRICT__ handle, const socketDetails *const __RESTRICT__ details){

	if(sc->nfds >= sc->capacity){
		#ifdef SOCKET_REALLOCATE
			if(scResize(sc) < 0){
				// Memory allocation failure.
				return -1;
			}
		#else
			return 0;
		#endif
	}

	{

		// The file descriptor stores a pointer
		// to its corresponding details buffer.
		socketHandle *const newHandle = ++sc->handleLast;
		socketDetails *const newDetails = *((socketDetails **)newHandle);

		*newHandle = *handle;
		newDetails->handle = newHandle;
		newDetails->addressSize = details->addressSize;
		newDetails->address = details->address;
		#ifdef SOCKET_MANAGE_TIMEOUTS
		newDetails->lastTick = details->lastTick;
		#endif

		sc->detailsLast = newDetails;
		++sc->nfds;

	}

	return 1;

}

int scRemoveSocket(socketServer *const __RESTRICT__ sc, socketDetails *const details){

	// Don't touch the master socket.
	if(details == scMasterDetails(sc)){
		return 0;
	}

	// Move the last handle to fill in the gap.
	*details->handle = *sc->handleLast;
	// Make the now-empty last handle point to
	// its new (empty) corresponding details.
	*((socketDetails **)sc->handleLast) = details;
	sc->detailsLast->handle = details->handle;
	// Shift the last handle back.
	--sc->handleLast;

	// These details are no longer in use.
	details->handle = NULL;

	--sc->nfds;
	return 1;

}

void scDelete(socketServer *sc){

	if(sc->details != NULL){

		socketHandle *handle = sc->handles;
		const socketHandle *const handleLast = &handle[sc->nfds];

		while(handle < handleLast){
			socketclose(handle->fd);
			++handle;
		}

		#ifdef SOCKET_USE_MALLOC
		free(sc->details);
		#else
		memFree(sc->details);
		#endif

	}

}
