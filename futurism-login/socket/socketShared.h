#ifndef SOCKETSHARED_H
#define SOCKETSHARED_H

#include "../settings/socketSettings.h"

#ifndef SOCKET_DEFAULT_ADDRESS_FAMILY
	#define SOCKET_DEFAULT_ADDRESS_FAMILY AF_INET6
#endif
#ifndef SOCKET_DEFAULT_PORT
	#define SOCKET_DEFAULT_PORT 7249
#endif
#ifndef SOCKET_UDP_MAX_BUFFER_SIZE
	#define SOCKET_UDP_MAX_BUFFER_SIZE 4096
#endif
#ifndef SOCKET_TCP_MAX_BUFFER_SIZE
	#define SOCKET_TCP_MAX_BUFFER_SIZE 4096
#endif
#ifndef SOCKET_POLL_TIMEOUT
	#define SOCKET_POLL_TIMEOUT 0
#endif
#ifndef SOCKET_CONNECTION_TIMEOUT
	#define SOCKET_CONNECTION_TIMEOUT 60000
#endif
#ifndef SOCKET_MAX_SOCKETS
	#define SOCKET_MAX_SOCKETS 257
#endif
#ifndef SOCKET_USE_SELECT
	#define SOCKET_POLL_FUNC "poll()"
#else
	#define SOCKET_POLL_FUNC "select()"
#endif

#undef FD_SETSIZE
#undef __FD_SETSIZE
#define FD_SETSIZE SOCKET_MAX_SOCKETS
#define __FD_SETSIZE SOCKET_MAX_SOCKETS

// Flags passed into ssHandleConnections functions.
#define SOCKET_FLAGS_UDP             0x01  // Currently no longer used.
#define SOCKET_FLAGS_TCP             0x02  // Currently no longer used.
#define SOCKET_FLAGS_VERBOSE         0x04  // Currently no longer used.
#define SOCKET_FLAGS_MANAGE_TIMEOUTS 0x08
#define SOCKET_FLAGS_ABSTRACT_HANDLE 0x10  // Currently no longer used.
#define SOCKET_FLAGS_READ_FULL_QUEUE 0x20
#define SOCKET_FLAGS_ALLOCATE_SOCKET 0x40  // Currently no longer used.

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define lastErrorID WSAGetLastError()
	#define WINSOCK_VERSION MAKEWORD(2, 2)
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#define ECONNRESET WSAECONNRESET
	#define POLLHUP    0x002
	#define POLLOUT    0x010
	#define POLLIN     0x100
	#define POLLPRI    0x200
	#define socketclose(x) closesocket(x)
	struct pollfd {
		int fd;
		short events;
		short revents;
	} pollfd;
	#define socketHandle struct pollfd
	#define socketAddrLength int
	int inet_pton(int af, const char *src, char *dst);
	const char *inet_ntop(int af, const void *src, char *dst, size_t size);
#else
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>
	#define lastErrorID errno
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define socketclose(x) close(x)
	#ifndef SOCKET_USE_SELECT
		#include <sys/poll.h>
	#else
		#define POLLIN     0x0001
		#define POLLPRI    0x0002
		#define POLLOUT    0x0004
		#define POLLERR    0x0008
		#define POLLHUP    0x0010
		#define POLLNVAL   0x0020
		#define POLLRDNORM 0x0040
		#define POLLRDBAND 0x0080
		#define POLLWRNORM 0x0100
		#define POLLWRBAND 0x0200
		struct pollfd {
			int fd;
			short events;
			short revents;
		} pollfd;
	#endif
	#define socketHandle struct pollfd
	#define socketAddrLength socklen_t
#endif

int pollFunc(socketHandle *ufds, size_t nfds, int timeout);

#endif
