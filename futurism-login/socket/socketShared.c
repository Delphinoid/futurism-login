#include "socketShared.h"

#ifndef SOCKET_USE_SELECT
	#ifdef _WIN32
		WINSOCK_API_LINKAGE int WSAAPI WSAPoll(struct pollfd *ufds, ULONG fds, INT timeout);
		int pollFunc(socketHandle *ufds, size_t nfds, int timeout){ return WSAPoll(ufds, nfds, timeout); }
	#else
		int pollFunc(socketHandle *ufds, size_t nfds, int timeout){ return poll(ufds, nfds, timeout); }
	#endif
#else
	int pollFunc(socketHandle *ufds, size_t nfds, int timeout){
		int changedSockets;
		struct timeval timeoutConversion;
		struct timeval *timeoutVar = NULL;
		fd_set socketSet;
		size_t socketNum = nfds < SOCKET_MAX_SOCKETS ? nfds : SOCKET_MAX_SOCKETS;
		int *ssfd = socketSet.fd_array;
		socketHandle *ufd = ufds;
		const int *ssfdLast = &ssfd[socketNum];
		const socketHandle *const ufdLast = &ufd[socketNum];
		while(ssfd < ssfdLast){
			*ssfd = ufd->fd;
			++ssfd; ++ufd;
		}
		socketSet.fd_count = socketNum;
		if(timeout >= 0){
			if(timeout > 0){
				timeoutConversion.tv_sec = timeout / 1000;
				timeoutConversion.tv_usec = (timeout - (timeoutConversion.tv_sec * 1000)) * 1000;
			}else{
				timeoutConversion.tv_sec = 0;
				timeoutConversion.tv_usec = 0;
			}
			timeoutVar = &timeoutConversion;
		}
		changedSockets = select(0, &socketSet, NULL, NULL, timeoutVar);
		ssfd = socketSet.fd_array;
		ssfdLast = &ssfd[changedSockets];
		while(ssfd < ssfdLast){
			ufd = ufds;
			for(ufd < ufdLast){
				if(*ssfd == ufd->fd){
					ufd->revents = POLLIN;
					break;
				}
				++ufd;
			}
			++ssfd;
		}
		return changedSockets;
	}
#endif
