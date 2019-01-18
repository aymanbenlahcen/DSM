#ifndef COMMON_IMPL_H
#define COMMON_IMPL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>

/* autres includes (eventuellement) */

#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}

#define NAME_MACHINE_SIZE 50

/* definition du type des infos */
/* de connexion des processus dsm */
struct dsm_proc_conn  {
   int rank;
   int port;
   char ip[17];
   char name[NAME_MACHINE_SIZE];
   int fd;
   /* a completer */
};
typedef struct dsm_proc_conn dsm_proc_conn_t;

/* definition du type des infos */
/* d'identification des processus dsm */
struct dsm_proc {
  pid_t pid;
  int fd_stderr;
  int fd_stdout;
  dsm_proc_conn_t connect_info;
};
typedef struct dsm_proc dsm_proc_t;

int creer_socket(int type, int *port_num);

int nb_read_lines(FILE *fd_machine_file);

void get_machine_file(FILE *fd,dsm_proc_t *proc_array);

void reshuffle_pollfd(struct pollfd *pollfds,int *num_procs);

size_t do_read(int fd, void *buf,size_t size_to_send);

size_t do_read2(int fd, void *buf,size_t size_to_read);

size_t do_write(int fd, void *buf,size_t size_to_send);

size_t send_size(int fd,size_t size);

size_t read_size(int fd,size_t *size);
#endif
