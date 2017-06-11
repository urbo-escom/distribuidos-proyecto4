#include "_common.h"


int socket_init()
{
#ifdef _WIN32
	WSADATA wsa;
	int status;
	if (0 != (status = WSAStartup(MAKEWORD(2, 2), &wsa)))
		return -1;
#endif
	return 0;
}


int socket_finalize()
{
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
