#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include "handling_socket.h"
#include "common_impl.h"


//create a new socket
int do_socket(int domain, int type, int protocol) {
  int sockfd;
  int yes = 1;
  //create the socket
  sockfd = socket(domain,type,protocol);

  //check for socket validity
  if(sockfd==-1)
  {
    ERROR_EXIT("Error : socket making");
  }

  // set socket option, to prevent "already in use" issue when rebooting the server right on
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  ERROR_EXIT("ERROR setting socket options");

  return sockfd;
}

//connect the socket to the server
void do_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  int res;
  do{
    res = connect(sockfd, addr, addrlen);
  }
  while(res==-1 && errno==EINTR);

  if (res != 0) {
    ERROR_EXIT("Error : connection problem");
  }
}

//get address info from the server
void get_addr(char* addr,char* port,struct sockaddr_in* sock_serv){
  memset(sock_serv,'\0',sizeof(*sock_serv));
  sock_serv->sin_family=AF_INET;
  sock_serv->sin_port=htons(atoi(port));
  inet_aton(addr,&sock_serv->sin_addr);
}


//the server accept a new connection
int do_accept(int s, struct sockaddr*addr, socklen_t* addrlen)
{
  int accept_d = accept(s,addr , addrlen);

  if(accept_d == -1) {
    perror("Error in accept\n");
  }
  return accept_d;
}

//create a listening socket
void do_listen(int s, int backlog)
{
  int l_sock = listen(s, backlog);

  if(l_sock==-1)
  {
    ERROR_EXIT("Error : listen problem");
  }
}

//binding the socket
int do_bind(int sock, const struct sockaddr *addr, int adrlen){

  int binded_sock = bind(sock,addr,adrlen);

  return binded_sock;
}


//initialize the server address
void init_serv_addr(int *port_num,char ip[17],struct sockaddr_in *serv_addr){

  //clean the serv_add structure
  memset(serv_addr,'\0',sizeof(*serv_addr));

  //create a random port
  *port_num=(rand()%45000)+1024;

  //internet family protocol
  serv_addr->sin_family = AF_INET;

  //we bind to any ip form the host
  //get the client IP address
  struct hostent * host;
  char *namen=malloc(NAME_MACHINE_SIZE*sizeof(char));
  gethostname(namen,NAME_MACHINE_SIZE);
  host = gethostbyname(namen);

  serv_addr->sin_addr=*(struct in_addr *)host->h_addr;
  char *ip_tmp;
  ip_tmp=inet_ntoa(serv_addr->sin_addr);
  strcpy(ip,ip_tmp);
  //we bind on the tcp port specified
  serv_addr->sin_port = htons(*port_num);
  free(namen);
}
