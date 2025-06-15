// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#pragma once

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef POLLIN
#define POLLIN 0x0001
#endif
#ifndef POLLOUT
#define POLLOUT 0x0004
#endif
#ifndef POLLERR
#define POLLERR 0x0008
#endif

// Replace poll with WSAPoll using Winsock2
static inline int poll(struct pollfd *fds, unsigned int nfds, int timeout)
{
    return WSAPoll(fds, nfds, timeout);
}

#endif // _WIN32


