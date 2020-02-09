#ifndef SOCKETUDP_H
#define SOCKETUDP_H

#include "socketServer.h"

#ifdef _WIN32
int ssSendUDP(const socketServer *const __RESTRICT__ server, const socketDetails *const __RESTRICT__ details, const char *const __RESTRICT__ buffer, const int length);
#else
int ssSendUDP(const socketServer *const __RESTRICT__ server, const socketDetails *const __RESTRICT__ details, const char *const __RESTRICT__ buffer, const size_t length);
#endif
int ssReceiveUDP(socketServer *const __RESTRICT__ server, socketDetails **details, char *const __RESTRICT__ buffer);

#endif
