#include "socketTCP.h"
#include "../shared/flags.h"

#ifdef _WIN32
int ssSendTCP(const socketHandle *const __RESTRICT__ handle, const char *const __RESTRICT__ buffer, const int length){
#else
int ssSendTCP(const socketHandle *const __RESTRICT__ handle, const char *const __RESTRICT__ buffer, const size_t length){
#endif
	#ifdef SOCKET_DEBUG
	if(send(handle->fd, buffer, length, 0) < 0){
		ssReportError("send()", ssError);
		return 0;
	}
	return 1;
	#else
	return (send(handle->fd, buffer, length, 0) >= 0);
	#endif
}

int ssPollTCP(socketServer *const __RESTRICT__ server){

	socketHandle *const master = scMasterHandle(server);
	int changedSockets = pollFunc(master, server->nfds, SOCKET_POLL_TIMEOUT);

	// If pollFunc() returned SOCKET_ERROR (-1), return.
	if(changedSockets == SOCKET_ERROR){
		#ifdef SOCKET_DEBUG
		ssReportError(SOCKET_POLL_FUNC, ssError);
		#endif

	// The master socket changes state when it receives a new connection.
	}else if(flagsAreSet(master->revents, POLLIN)){

		socketHandle clientHandle;
		socketDetails clientDetails;

		// Clear the master socket's revents flags.
		master->revents = 0;
		--changedSockets;

		// Accept the new connection.
		clientDetails.addressSize = sizeof(struct sockaddr);
		clientHandle.fd = accept(master->fd, (struct sockaddr *)&clientDetails.address, &clientDetails.addressSize);

		// Check if accept() was successful.
		if(clientHandle.fd != INVALID_SOCKET){
			#ifdef SOCKET_REALLOCATE
			return_t r;
			#endif
			clientHandle.events = POLLIN;
			clientHandle.revents = 0;
			#ifdef SOCKET_REALLOCATE
			r = scAddSocket(sc, &clientHandle, &clientDetails);
			if(r < 0){
				// Memory allocation failure.
				return -1;
			}else if(r == 0){
				// Server is full.
				socketclose(clientHandle.fd);
			}
			#else
			if(scAddSocket(server, &clientHandle, &clientDetails) == 0){
				// Server is full.
				socketclose(clientHandle.fd);
			}
			#endif
		}else{
			#ifdef SOCKET_DEBUG
			ssReportError("accept()", ssError);
			#endif
		}

	}

	return changedSockets;

}
