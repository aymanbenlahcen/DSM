#include "common_impl.h"
#include <poll.h>


int nb_read_lines(FILE *fd)
{
  char buffer[NAME_MACHINE_SIZE];
  int nb_lines=0;
  while(fgets(buffer,sizeof(buffer),fd)!=NULL)
  {
      nb_lines++;
  }
  return nb_lines;
}


void get_machine_file(FILE *fd,dsm_proc_t *proc_array)
{
  char buffer[NAME_MACHINE_SIZE];
  int i=0;
  fseek(fd,0,SEEK_SET);
  while(fgets(buffer,sizeof(buffer),fd)!=NULL)
  {
      buffer[strlen(buffer)-1]='\0';
      strcpy(proc_array[i].connect_info.name,buffer);
      i++;
  }
}


//reshuffling the pollfds table to get a non hollow table
void reshuffle_pollfd(struct pollfd *pollfds,int *num_procs)
{

  int j,i;
  for(i = 0; j <*num_procs-1; j+=2)
  {
      if(pollfds[i].fd==-1){
        for(j=i;j<*num_procs;j++){
          pollfds[j].fd = pollfds[j+2].fd;
          pollfds[j].revents = pollfds[j+2].revents;
          pollfds[j+2].fd = -1;
          pollfds[j+2].revents = -1;
        }
      }
  }
  (*num_procs)--;
}

size_t send_size(int fd,size_t size)
{
  size=do_write(fd,&size,sizeof(size_t));
  return size;
}

size_t do_write(int fd, void *buf,size_t size_to_send){
    size_t size_send=0;

    do {
      size_send+= write(fd, buf+size_send,(size_to_send-size_send));
  }while (size_send < size_to_send);

  return size_send;
}

size_t read_size(int fd,size_t *size)
{
  size_t s;
  s=do_read(fd,size,sizeof(size_t));
  return s;
}

size_t do_read(int fd, void *buf,size_t size_to_read){
    size_t size_read = 0;

    memset(buf,0,sizeof(*buf));

    do {
      size_read += read(fd, buf+size_read,(size_to_read-size_read));
  }while (size_read < size_to_read);

  return size_read;
}

size_t do_read2(int fd, void *buf,size_t size_to_read){
    size_t size_read = 0;

    memset(buf,0,sizeof(*buf));

    do {
      printf("t qqskdljf qdsklfjq sdfklqdjfs qkdlsfjlkqsdjf\n");
      fflush(stdout);
      size_read += read(fd, buf+size_read,(size_to_read-size_read));
      printf("t qqskdljf qdsklfjq sdfklqdjfs qkdlsfjlkqsdjf\n");
      fflush(stdout);
  }while (size_read < size_to_read);

  printf("t qqskdljf qdsklfjq sdfklqdjfs qkdlsfjlkqsdjf\n");
  fflush(stdout);
  return size_read;
}
