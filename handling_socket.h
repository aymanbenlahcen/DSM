/*
 * handling_socket.h
 */

 #ifndef HANDLING_SOCKET_H_
 #define HANDLING_SOCKET_H_

 #include <poll.h>
#include <netinet/in.h>
#include "common_impl.h"

 int do_socket(int domain, int type, int protocol);

 void do_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

 void get_addr(char* addr,char* port,struct sockaddr_in* sock_serv);

void init_serv_addr(int *port_num,char ip[17],struct sockaddr_in *serv_addr);

 int do_bind(int sock, const struct sockaddr *addr, int adrlen);

 void do_listen(int s, int backlog);

 int do_accept(int s, struct sockaddr*addr, socklen_t* addrlen);



 #endif
