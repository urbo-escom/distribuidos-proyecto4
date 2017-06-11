#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	typedef SOCKET socket_t;
	#define preserving_error(s) do { \
		int   err_; \
		DWORD errw32_; \
		errw32_ = GetLastError(); \
		err_    = errno; \
		s; \
		SetLastError(errw32_); \
		errno = err_; \
	} while (0)
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <fcntl.h>
	#include <unistd.h>
	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR   (-1)
	#define closesocket    close
	typedef int socket_t;
	#define preserving_error(s) do { \
		int   err_; \
		err_  = errno; \
		s; \
		errno = err_; \
	} while (0)
#endif


#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "socket.h"


struct socket_fd {
#ifdef _WIN32
	SOCKET sock;
#else
	int sock;
#endif
};


struct socket_addr {
	struct sockaddr_storage addr;
	socklen_t               addrlen;
};
