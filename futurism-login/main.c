#define WIN32_LEAN_AND_MEAN
#include "memory/memoryManager.h"
#include "server/server.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

server s;

void cleanup(){
	serverDelete(&s);
	ssCleanup();
	memMngrDelete();
}

int main(int argc, char **argv){

	atexit(cleanup);
	srand(time(NULL));

	// Initialize the memory manager and the server.
	if(memMngrInit(MEMORY_MANAGER_DEFAULT_VIRTUAL_HEAP_SIZE, 1) < 0 || !ssStartup() || !serverInit(&s)){
		return 1;
	}

	// Main loop.
	while(1){

		socketDetails *i;
		size_t j;

		// Check state changes.
		ssHandleConnectionsTCP(&s.ss.connectionHandler, 0x00);

		// Handle incoming requests.
		i = s.ss.connectionHandler.details;
		j = s.ss.connectionHandler.nfds;
		while(j > 0){
			if(sdValid(i)){

				if(flagsAreSet(i->flags, SOCKET_DETAILS_DISCONNECTED | SOCKET_DETAILS_TIMED_OUT | SOCKET_DETAILS_ERROR)){

					#ifdef SOCKET_DEBUG
					// Socket has disconnected.
					char ip[46];
					inet_ntop(
						i->address.ss_family,
						(i->address.ss_family == AF_INET ?
						(void *)(&((struct sockaddr_in *)&i->address)->sin_addr) :
						(void *)(&((struct sockaddr_in6 *)&i->address)->sin6_addr)),
						ip, sizeof(ip)
					);
					printf("Closing TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&i->address)->sin_port, (unsigned long)i->id);
					scRemoveSocket(&s.ss.connectionHandler, i);
					#endif

				}else{

					#ifdef SOCKET_DEBUG
					// Incoming request.
					if(flagsAreSet(i->flags, SOCKET_DETAILS_CONNECTED)){
						// Socket has connected.
						char ip[46];
						inet_ntop(
							i->address.ss_family,
							(i->address.ss_family == AF_INET ?
							(void *)(&((struct sockaddr_in *)&i->address)->sin_addr) :
							(void *)(&((struct sockaddr_in6 *)&i->address)->sin6_addr)),
							ip, sizeof(ip)
						);
						printf("Opening TCP connection with %s:%u (socket #%lu).\n", ip, ((struct sockaddr_in *)&i->address)->sin_port, (unsigned long)i->id);
					}
					#endif

					if(flagsAreSet(i->flags, SOCKET_DETAILS_NEW_DATA)){
						// Handle request and reset state flags.
						serverHandleRequest(&s, i);
					}

					i->flags = 0x00;

				}

				--j;

			}
			++i;
		}

	}

	// Clean up.
	cleanup();

	return 0;

}
