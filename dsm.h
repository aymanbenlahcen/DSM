#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common_impl.h"
#include "handling_socket.h"
#include <semaphore.h>


/* fin des includes */

#define TOP_ADDR    (0x40000000)
#define PAGE_NUMBER (100)
#define PAGE_SIZE   (sysconf(_SC_PAGE_SIZE))
#define BASE_ADDR   (TOP_ADDR - (PAGE_NUMBER * PAGE_SIZE))

typedef enum
{
   NO_ACCESS,
   READ_ACCESS,
   WRITE_ACCESS,
   UNKNOWN_ACCESS
} dsm_page_access_t;

typedef enum
{
   INVALID,
   READ_ONLY,
   WRITE,
   NO_CHANGE
} dsm_page_state_t;

typedef int dsm_page_owner_t;

typedef struct
{
   dsm_page_state_t status;
   dsm_page_owner_t owner;
} dsm_page_info_t;

dsm_page_info_t table_page[PAGE_NUMBER];

typedef struct{
  int port;
  char ip[17];
}arg_t;

typedef enum daemon_signs{
	ASK_PAGE=0,
	CHANGE_OWNER=1,
	QUIT=2,
  RECV_PAGE=3,
  NO_PAGE_OWNER=4,

}daemon_signs_t;

pthread_t comm_daemon;
extern int DSM_NODE_ID;
extern int DSM_NODE_NUM;
extern sem_t sem_info;


char *dsm_init( int argc, char **argv);
void  dsm_finalize( void );
