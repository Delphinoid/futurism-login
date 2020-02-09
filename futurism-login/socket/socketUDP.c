#include "socketUDP.h"
#include <string.h>

#ifdef _WIN32
int ssSendUDP(const socketServer *const __RESTRICT__ server, const socketDetails *const __RESTRICT__ details, const char *const __RESTRICT__ buffer, const int length){
#else
int ssSendUDP(const socketServer *const __RESTRICT__ server, const socketDetails *const __RESTRICT__ details, const char *const __RESTRICT__ buffer, const size_t length){
#endif
	#ifdef SOCKET_DEBUG
	if(sendto(scMasterHandle(server)->fd, buffer, length, 0, (struct sockaddr *)&details->address, details->addressSize) < 0){
		ssReportError("sendto()", ssError);
		return 0;
	}
	return 1;
	#else
	return (sendto(scMasterHandle(server)->fd, buffer, length, 0, (struct sockaddr *)&details->address, details->addressSize) >= 0);
	#endif
}

int ssReceiveUDP(socketServer *const __RESTRICT__ server, socketDetails **details, char *const __RESTRICT__ buffer){

	int bufferSize;

	// Create socketDetails struct for the socket we are receiving data from.
	socketDetails clientDetails;
	clientDetails.addressSize = sizeof(struct sockaddr);

	// Receives up to MAX_BUFFER_SIZE bytes of data from a client socket and stores it in buffer.
	bufferSize = recvfrom(
		scMasterHandle(server)->fd, buffer, SOCKET_MAX_BUFFER_SIZE, 0,
		(struct sockaddr *)&clientDetails.address, &clientDetails.addressSize
	);

	// Check if anything was received.
	if(bufferSize < 0){
		// Error was encountered, abort the loop.
		const int error = ssError;
		// Don't bother reporting the error if it's EWOULDBLOCK or ECONNRESET, as it can be ignored here.
		if(error != EWOULDBLOCK && error != ECONNRESET){
			#ifdef SOCKET_DEBUG
			ssReportError("recvfrom()", error);
			#endif
		}
		*details = NULL;
	}else{

		socketDetails *i = server->details;
		size_t j = server->nfds-1;

		// Check if the socket exists, and while we're at it disconnect any sockets that have timed out.
		while(j > 0){
			if(sdValid(i)){
				if(
					clientDetails.addressSize == i->addressSize &&
					memcmp(&clientDetails.address, &i->address, clientDetails.addressSize)
				){
					// We found the sender.
					*details = i;
					return bufferSize;
				}
				++i;
			}
			--j;
		}


		// If the socket was not found (we currently do not have a session
		// with it) and we have enough room, add it to the connection handler.
		#ifdef SOCKET_REALLOCATE
		int r;
		#endif
		socketHandle clientHandle;
		#ifdef SOCKET_REALLOCATE
		r = scAddSocket(server, &clientHandle, &clientDetails);
		if(r < 0){
			// Memory allocation failure.
			return -1;
		}else if(r != 0){
			// Connection handler is not full.
			*details = server->detailsLast;
		}
		#else
		if(scAddSocket(server, &clientHandle, &clientDetails) != 0){
			// Connection handler is not full.
			*details = server->detailsLast;
		}
		#endif

	}

	return bufferSize;

}
