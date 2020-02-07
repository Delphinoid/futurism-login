#include "socketConnectionHandler.h"

#ifndef SOCKET_USE_MALLOC
#include "../memory/memoryManager.h"
#endif

#ifdef SOCKET_MANAGE_TIMEOUTS
__FORCE_INLINE__ return_t sdTimedOut(const socketDetails *const __RESTRICT__ details, const uint32_t currentTick){
	return currentTick - details->lastTick >= SOCKET_CONNECTION_TIMEOUT;
}
#endif

static __FORCE_INLINE__ uintptr_t scSocketID(const socketConnectionHandler *const __RESTRICT__ sc, const socketDetails *const details){
	return (((uintptr_t)details) - ((uintptr_t)sc->details)) / sizeof(socketDetails);
}

socketDetails *scSocket(socketConnectionHandler *const __RESTRICT__ sc, const uintptr_t id){
	return &sc->details[id];
}

return_t scInit(socketConnectionHandler *const __RESTRICT__ sc, const size_t capacity, const socketHandle *const __RESTRICT__ masterHandle, const socketDetails *const __RESTRICT__ masterDetails){

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

static __FORCE_INLINE__ return_t scResize(socketConnectionHandler *const __RESTRICT__ sc){

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

return_t scAddSocket(socketConnectionHandler *const __RESTRICT__ sc, const socketHandle *const __RESTRICT__ handle, const socketDetails *const __RESTRICT__ details){

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
		newDetails->bufferSize = 0;
		newDetails->buffer[0] = '\0';
		newDetails->flags = details->flags;

		sc->detailsLast = newDetails;
		++sc->nfds;

	}

	return 1;

}

return_t scRemoveSocket(socketConnectionHandler *const __RESTRICT__ sc, socketDetails *details){

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

void scDelete(socketConnectionHandler *sc){

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
