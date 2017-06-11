/*
 * cross-platform socket library (IPv4-only)
 */
#ifndef SOCKET_H
#define SOCKET_H

#include <stddef.h>


typedef struct socket_fd   socket_fd;   /* socket handle */
typedef struct socket_addr socket_addr; /* an IPv4 addr */


/* Must be called before any other function */
int socket_init();

/* Should be called at the end of the program */
int socket_finalize();


/*
 * addr functions
 */

int  socket_addr_create(socket_addr **addr);
void socket_addr_free(socket_addr *addr);

void socket_addr_get_handle(const socket_addr *addr, void *handle, int *len);
void socket_addr_set_handle(socket_addr *addr, const void *handle, int len);

int  socket_addr_get_ip(const socket_addr *addr, char *ip, size_t n);
int  socket_addr_set_ip(socket_addr *addr, const char *ip);

int  socket_addr_get_port(const socket_addr *addr, int *port);
int  socket_addr_set_port(socket_addr *addr, int port);

socket_addr* socket_addr_cpy(socket_addr *dst, const socket_addr *src);
int          socket_addr_cmp(const socket_addr *a, const socket_addr *b);


/*
 * socket functions
 */

int socket_udp_create(socket_fd **fd);
int socket_close(socket_fd *fd);

void socket_get_handle(const socket_fd *fd, void *handle);
void socket_set_handle(socket_fd *fd, const void *handle);

int socket_connect(socket_fd *fd, const socket_addr *addr);
int socket_bind(   socket_fd *fd, const socket_addr *addr);
int socket_getpeeraddr(const socket_fd *fd, socket_addr *addr);
int socket_getselfaddr(const socket_fd *fd, socket_addr *addr);


/* create, bind and fill an udp socket fd and addr */
int socket_udp_bind(socket_fd **fd, socket_addr *addr,
		const char *ip, int port);

/* create, connect and fill an udp socket fd and addr */
int socket_udp_connect(socket_fd **fd, socket_addr *addr,
		const char *ip, int port);


int socket_recv(    socket_fd* fd, void *buf, size_t len);
int socket_recvfrom(socket_fd* fd, void *buf, size_t len,
		socket_addr *addr);

int socket_send(  socket_fd* fd, const void *buf, size_t len);
int socket_sendto(socket_fd* fd, const void *buf, size_t len,
		const socket_addr *addr);


/*
 * socket options
 */
int socket_setnonblocking(socket_fd *fd);
int socket_setbroadcast(socket_fd *fd);
int socket_settimetolive(socket_fd *fd, int ttl);
int socket_group_join( socket_fd *fd, const socket_addr *addr);
int socket_group_leave(socket_fd *fd, const socket_addr *addr);
int socket_recv_timeout_ms(socket_fd *fd, int msec);
int socket_send_timeout_ms(socket_fd *fd, int msec);


#endif /* !SOCKET_H */
