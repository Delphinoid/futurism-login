#ifndef SOCKETTCP_H
#define SOCKETTCP_H

#include "socketServer.h"

#ifdef _WIN32
int ssSendTCP(const socketHandle *const __RESTRICT__ handle, const char *const __RESTRICT__ buffer, const int length);
#else
int ssSendTCP(const socketHandle *const __RESTRICT__ handle, const char *const __RESTRICT__ buffer, const size_t length);
#endif
int ssPollTCP(socketServer *const __RESTRICT__ server);

#endif
