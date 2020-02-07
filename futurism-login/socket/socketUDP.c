#include "socketUDP.h"
#include <string.h>

return_t ssSendDataUDP(const socketConnectionHandler *const __RESTRICT__ sc, const socketDetails *const __RESTRICT__ details, const char *const __RESTRICT__ msg){
	if(sendto(scMasterHandle(sc)->fd, msg, strlen(msg), 0, (struct sockaddr *)&details->address, details->addressSize) < 0){
		#ifdef SOCKET_DEBUG
		ssReportError("sendto()", lastErrorID);
		#endif
		return 0;
	}
	return 1;
}

#ifdef SOCKET_MANAGE_TIMEOUTS
return_t ssHandleConnectionsUDP(socketConnectionHandler *const __RESTRICT__ sc, const flags_t flags, const uint32_t currentTick){
#else
return_t ssHandleConnectionsUDP(socketConnectionHandler *const __RESTRICT__ sc, const flags_t flags){
#endif

	// Keep receiving data while the buffer is not empty.
	do {

		// Create socketDetails struct for the socket we are receiving data from.
		socketDetails clientDetails;
		clientDetails.addressSize = sizeof(struct sockaddr);

		// Receives up to MAX_BUFFER_SIZE bytes of data from a client socket and stores it in buffer.
		clientDetails.bufferSize = recvfrom(
			scMasterHandle(sc)->fd, clientDetails.buffer, SOCKET_MAX_BUFFER_SIZE, 0,
			(struct sockaddr *)&clientDetails.address, &clientDetails.addressSize
		);

		// Check if anything was received.
		if(clientDetails.bufferSize > 0){

			unsigned int found = 0;
			socketDetails *i = sc->details+1;
			size_t j = sc->nfds-1;

			// Check if the socket exists, and while we're at it disconnect any sockets that have timed out.
			while(j > 0){

				if(sdValid(i)){

					// Check if the addresses are the same for the two sockets (includes port).
					if(!found && memcmp(&clientDetails.address, &i->address, clientDetails.addressSize)){
						found = 1;
					#ifdef SOCKET_MANAGE_TIMEOUTS
						if(flagsAreUnset(flags, SOCKET_FLAGS_MANAGE_TIMEOUTS)){
							break;
						}

					// Disconnect the current socket if it has timed out.
					}else if(flagsAreSet(flags, SOCKET_FLAGS_MANAGE_TIMEOUTS) && sdTimedOut(i, currentTick)){
						flagsSet(i->flags, SOCKET_DETAILS_TIMED_OUT);
					#endif
					}

					--j;

				}

				++i;

			}


			// If the socket was not found (we currently do not have a session
			// with it) and we have enough room, add it to the connection handler.
			if(!found){
				#ifdef SOCKET_REALLOCATE
				return_t r;
				#endif
				socketHandle clientHandle;
				clientHandle.fd = scMasterHandle(sc)->fd;
				clientDetails.flags = SOCKET_DETAILS_CONNECTED;
				#ifdef SOCKET_REALLOCATE
				r = scAddSocket(sc, &clientHandle, &clientDetails);
				if(r < 0){
					// Memory allocation failure.
					return -1;
				}else if(r == 0){
					// Server is full.
					continue;
				}
				// Set i to the newly added socket.
				i = sc->detailsLast;
				#else
				if(scAddSocket(sc, &clientHandle, &clientDetails) == 0){
					// Server is full.
					continue;
				}
				#endif
			}

			// Copy over the last buffer.
			i->bufferSize = clientDetails.bufferSize;
			memcpy(i->buffer, clientDetails.buffer, clientDetails.bufferSize);

			// Do something with the received data.
			flagsSet(i->flags, SOCKET_DETAILS_NEW_DATA);

		}else{
			// Error was encountered, abort the loop.
			const int tempLastErrorID = lastErrorID;
			// Don't bother reporting the error if it's EWOULDBLOCK or ECONNRESET, as it can be ignored here.
			if(tempLastErrorID != EWOULDBLOCK && tempLastErrorID != ECONNRESET){
				#ifdef SOCKET_DEBUG
				ssReportError("recvfrom()", tempLastErrorID);
				#endif
				return 0;
			}
			break;
		}

	} while(flagsAreSet(flags, SOCKET_FLAGS_READ_FULL_QUEUE));

	return 1;

}

void ssShutdownUDP(socketConnectionHandler *const __RESTRICT__ sc){
	scDelete(sc);
}
