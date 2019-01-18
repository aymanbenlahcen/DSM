#include "dsm.h"

int DSM_NODE_NUM; /* nombre de processus dsm */
int DSM_NODE_ID;  /* rang (= numero) du processus */
dsm_proc_conn_t *my_proc_array;

//pthread_mutex_t info_mutex=PTHREAD_MUTEX_INITIALIZER;
sem_t sem_info;
int proc_waiting_to_quit=0;

static int get_real_rank(int rank)
{
  if(rank>DSM_NODE_ID)
  {
    rank--;
  }
  return rank;
}

static int get_real_rank_bis(int rank)
{
  if(rank>=DSM_NODE_ID)
  {
    rank++;
  }
  return rank;
}

/* indique l'adresse de debut de la page de numero numpage */
static char *num2address( int numpage )
{
  char *pointer = (char *)(BASE_ADDR+(numpage*(PAGE_SIZE)));

  if( pointer >= (char *)TOP_ADDR ){
    fprintf(stderr,"[%i] Invalid address %d !\n", DSM_NODE_ID,numpage);
    return NULL;
  }
  else return pointer;
}

static void dsm_write_type(int fd, void *buf,size_t size,int type_msg)
{
  do_write(fd,&type_msg,sizeof(daemon_signs_t));
  do_write(fd,buf,size);
}

static int address2num( char * page_addr )
{

  char *pointer = (char *)(page_addr);
  int numpage=0;
  if( pointer >= (char *)TOP_ADDR){
    fprintf(stderr,"[%i] Invalid address %p!\n", DSM_NODE_ID,page_addr);
    fflush(stdout);
    return 0;
  }
  while(pointer>(char *)BASE_ADDR)
  {
    pointer=(char *)(pointer-PAGE_SIZE);
    numpage++;
  }
  return numpage ;
}

/* fonctions pouvant etre utiles */
static void dsm_change_info( int numpage, dsm_page_state_t state, dsm_page_owner_t owner)
{
  if ((numpage >= 0) && (numpage < PAGE_NUMBER)) {
    if (state != NO_CHANGE )
    table_page[numpage].status = state;
    if (owner >= 0 )
    table_page[numpage].owner = owner;
    return;
  }
  else {
    fprintf(stderr,"[%i] Invalid page number !\n", DSM_NODE_ID);
    return;
  }
}

static dsm_page_owner_t get_owner( int numpage)
{
  return table_page[numpage].owner;
}

static dsm_page_state_t get_status( int numpage)
{
  return table_page[numpage].status;
}

/* Allocation d'une nouvelle page */
static void dsm_alloc_page( int numpage )
{
  char *page_addr = num2address( numpage );
  mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return ;
}

/* Changement de la protection d'une page */
static void dsm_protect_page( int numpage , int prot)
{
  char *page_addr = num2address( numpage );
  mprotect(page_addr, PAGE_SIZE, prot);
  return;
}

static void dsm_free_page( int numpage )
{
  char *page_addr = num2address( numpage );
  munmap(page_addr, PAGE_SIZE);
  return;
}

static void dsm_recv(int fd,int num_page)
  {
      char *recv_page= malloc(PAGE_SIZE);

      do_read(fd, recv_page, PAGE_SIZE);

      dsm_alloc_page(num_page);

      memcpy(num2address(num_page), recv_page, PAGE_SIZE);

      dsm_change_info(num_page, WRITE, DSM_NODE_ID);
    //  printf("%d %d res %d teztstst\n",DSM_NODE_ID,num_page,res);

  }

 static void dsm_send(int fd, int num_page) {
    char *page_addr = num2address(num_page);

    char *send_page = malloc(PAGE_SIZE);

    memcpy(send_page, page_addr, PAGE_SIZE);

    do_write(fd,send_page, PAGE_SIZE);
    //printf("%d %d res %d teztstst\n",DSM_NODE_ID,num_page,res);
    fflush(stdout);
    dsm_free_page(num_page);
  }

  void handle_sigusr1(int signum){ //signal handler
    sem_wait(&sem_info);
    sem_post(&sem_info);
  }

static void *dsm_comm_daemon( void *arg)
{
  dsm_proc_conn_t *proc_array = (dsm_proc_conn_t *) arg;
  struct pollfd *request_table = malloc(DSM_NODE_NUM * sizeof(struct pollfd));
  int k;
  int check;
  int num_page=0;
  int new_owner[2];
  daemon_signs_t type_request;
  daemon_signs_t type;

  memset(request_table,0, sizeof(struct pollfd) * DSM_NODE_NUM);
  for(k = 0; k < DSM_NODE_NUM; k++) {
    request_table[k].fd = proc_array[k].fd;
    request_table[k].events = POLLIN;
  }

  while(1)
  {
    do{
      check = poll(request_table, DSM_NODE_NUM, -1);
    }while(check == -1 && errno == EINTR);

    for(k = 0; k < DSM_NODE_NUM; k++){
    //  printf("%d revent %d : %d %d %d\n\n",DSM_NODE_ID,k,request_table[k].revents,POLLIN,POLLHUP);
      fflush(stdout);
      if(request_table[k].revents==0){
        continue;
      }
      if(request_table[k].revents == POLLIN){

        do_read(request_table[k].fd, &type_request,sizeof(daemon_signs_t));

        printf("[%d] : requête : %d venant de %d\n",DSM_NODE_ID,type_request,get_real_rank_bis(k));
        fflush(stdout);

        switch(type_request) {
          case ASK_PAGE :
            do_read(request_table[k].fd,&num_page,sizeof(int));
            if(get_owner(num_page)!=DSM_NODE_ID){
              type=NO_PAGE_OWNER;
              do_write(request_table[k].fd,&type,sizeof(daemon_signs_t));
            }
            else{
              dsm_change_info(num_page,NO_CHANGE,k+1);
              type=RECV_PAGE;
              do_write(request_table[k].fd,&type,sizeof(daemon_signs_t));
              dsm_send(request_table[k].fd,num_page);
            }

          break;
          case CHANGE_OWNER:
            do_read(request_table[k].fd,&new_owner,2*sizeof(int));
            dsm_change_info(new_owner[0],NO_CHANGE,new_owner[1]);
          break;
          case QUIT:
            proc_waiting_to_quit++;
            printf("[%d] : %d est prêt à quitter dsm\n",DSM_NODE_ID,get_real_rank_bis(k));
            if(proc_waiting_to_quit==DSM_NODE_NUM){
              free(request_table);
              pthread_exit(NULL);
            }

          break;
          case RECV_PAGE:
            dsm_recv(my_proc_array[k].fd,num_page);
            sem_post(&sem_info);
          break;
          case NO_PAGE_OWNER:
            sem_post(&sem_info);
          break;
          default :
          break;
        }

      }
    }
  }
}




  static void inform_dsm_change_owner(int num_page)
  {
    int new_owner[2]={num_page,get_owner(num_page)};

    int i=0;
    for (i = 0; i < DSM_NODE_NUM; i++)
    {
      dsm_write_type(my_proc_array[i].fd,new_owner,2*sizeof(int),CHANGE_OWNER);
    }
  }

  static void dsm_handler( void *page_addr)
  {
    sem_init(&sem_info,0,0);
    int num_page=address2num(page_addr);
    printf("[%i] FAULTY  ACCESS !!! \n",DSM_NODE_ID);
    fflush(stdout);
    int rank_page_owner=get_owner(num_page);
    int rank_to_contact=get_real_rank(rank_page_owner);

    //sem_wait(&sem_info);
    //pthread_kill(comm_daemon,SIGUSR1);

    fflush(stdout);
    while(rank_page_owner!=DSM_NODE_ID)
    {
      printf("[%d] : demande page %d à %d\n",DSM_NODE_ID,num_page,rank_page_owner);
      fflush(stdout);
      dsm_write_type(my_proc_array[rank_to_contact].fd,&num_page,sizeof(int),ASK_PAGE);
      fflush(stdout);
      sem_wait(&sem_info);
      rank_page_owner=get_owner(num_page);
      rank_to_contact=get_real_rank(rank_page_owner);
      printf("[%d] : nouveau rang détenteur de la page %d est %d\n",DSM_NODE_ID,num_page,rank_page_owner);
    }
    inform_dsm_change_owner(num_page);
  }

  /* traitant de signal adequat */
  static void segv_handler(int sig, siginfo_t *info, void *context)
  {
    /* A completer */
    /* adresse qui a provoque une erreur */

    void  *addr = info->si_addr;
    /* Si ceci ne fonctionne pas, utiliser a la place :*/
    /*
    #ifdef __x86_64__
    void *addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
    #elif __i386__
    void *addr = (void *)(context->uc_mcontext.cr2);
    #else
    void  addr = info->si_addr;
    #endif
    */
    /*
    pour plus tard (question ++):
    dsm_access_t access  = (((ucontext_t *)context)->uc_mcontext.gregs[REG_ERR] & 2) ? WRITE_ACCESS : READ_ACCESS;
    */
    /* adresse de la page dont fait partie l'adresse qui a provoque la faute */
    void  *page_addr  = (void *)(((unsigned long) addr) & ~(PAGE_SIZE-1));

    if ((addr >= (void *)BASE_ADDR) && (addr < (void *)TOP_ADDR))
    {
      dsm_handler(page_addr);

    }
    else
    {
      /* SIGSEGV normal : ne rien faire*/
    }
  }


  int do_connection(arg_t args)
  {
    int port=args.port;
    char ip[17];
    strcpy(ip,args.ip);
    struct sockaddr_in sock_serv;
    char port2[10];
    int sock;
    sprintf(port2,"%d",port);

    //get address info from the server
    get_addr(ip,port2,&sock_serv);

    //get the socket
    sock = do_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    //connect to the server remote socket
    do_connect(sock,(struct sockaddr*)&sock_serv,sizeof(sock_serv));
    do_write(sock,&DSM_NODE_ID,sizeof(int));
    return sock;
  }

  /* Seules ces deux dernieres fonctions sont visibles et utilisables */
  /* dans les programmes utilisateurs de la DSM                       */
  char *dsm_init(int argc, char **argv)
  {
    struct sigaction act;
    int index;
    char ip[17];
    char *port_launcher;
    struct sockaddr_in sock_serv ;
    int s;

    strcpy(ip,argv[1]);
    port_launcher=argv[2];
    DSM_NODE_ID=atoi(argv[3]);

    //get address info from the server
    get_addr(ip,port_launcher,&sock_serv);

    //get the socket
    s = do_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    //connect to the server remote socket
    do_connect(s,(struct sockaddr*)&sock_serv,sizeof(sock_serv));
    /* Envoi du nom de machine au lanceur */
    //char name[NAME_MACHINE_SIZE];
    //gethostname(name,NAME_MACHINE_SIZE);
    do_write(s,&DSM_NODE_ID,sizeof(int));
    /* Envoi du pid au lanceur */
    pid_t pid = getpid();
    //printf("%d test\n\n",DSM_NODE_ID);
    //fflush(stdout);
    //printf("%d : %d\n\n",DSM_NODE_ID,pid);
    //fflush(stdout);
    do_write(s,(void *)&pid,sizeof(pid_t));

    /* Creation de la socket d'ecoute pour les */
    /* connexions avec les autres processus dsm */
    struct sockaddr_in sock_listen;
    int listen_sock;
    int port_num;

    srand(getpid());
    do{
      listen_sock=do_socket(AF_INET,SOCK_STREAM,0);
      init_serv_addr(&port_num,ip,&sock_listen);
    }while(do_bind(listen_sock,(struct sockaddr*)&sock_listen,sizeof(sock_listen))==-1);

    /* Envoi du numero de port au lanceur */
    /* pour qu'il le propage à tous les autres */
    /* processus dsm */
    do_write(s,&port_num,sizeof(int));
    //printf("port : %d pid : %d\n",port_num,pid);

    //récupère le nombre de processus distants
    do_read(s,&DSM_NODE_NUM,sizeof(int));
    DSM_NODE_NUM--;

    do_listen(listen_sock,DSM_NODE_NUM);

    //printf("nb_proc : %d\n",nb_proc_dist);
    //fflush(stdout);
    //récupération du tableau proc_array
    my_proc_array = malloc(DSM_NODE_NUM * sizeof(dsm_proc_conn_t));
    int i;

    for(i=0;i<DSM_NODE_NUM;i++)
    {
      do_read(s,&my_proc_array[i],sizeof(dsm_proc_conn_t));
    }
    int sync=1;
    do_write(s,&sync,sizeof(int));
    do_read(s,&sync,sizeof(int));

    //se connecte aux autres machines
    if(DSM_NODE_NUM-DSM_NODE_ID>0){
      // printf("conn :%d\n\n",DSM_NODE_ID);
      //fflush(stdout);
      arg_t arguments;
      for(i=DSM_NODE_ID;i<DSM_NODE_NUM;i++)
      {
        memset(arguments.ip,'\0',17*sizeof(char));
        arguments.port=my_proc_array[i].port;
        strcpy(arguments.ip,my_proc_array[i].ip);
        // printf("%d : %d %s\n\n",DSM_NODE_ID,arguments.port,arguments.ip);
        //fflush(stdout);
        int res=do_connection(arguments);
        if(res !=-1)
          my_proc_array[i].fd=res;
      }
    }

    if(DSM_NODE_ID>0){
      struct sockaddr_in sock_client;
      int socket,rank_client;
      socklen_t addrlen;
      //printf("serv : %d\n\n",DSM_NODE_ID);
      //fflush(stdout);

      fflush(stdout);
      for(i=0;i<DSM_NODE_ID;i++)
      {
        memset(&sock_client,'\0',sizeof(sock_client));
        do{
          addrlen=sizeof(sock_client);
          socket=do_accept(listen_sock, (struct sockaddr *) &sock_client, &addrlen);
        }while(socket==-1 && errno==EINTR);
        do_read(socket,&rank_client,sizeof(int));
        // printf("%d rank client : %d\n",DSM_NODE_ID,rank_client);
        //fflush(stdout);

        my_proc_array[rank_client].fd=socket;
      }
    }

    /* reception du nombre de processus dsm envoye */
    /* par le lanceur de programmes (DSM_NODE_NUM)*/

    /* reception de mon numero de processus dsm envoye */
    /* par le lanceur de programmes (DSM_NODE_ID)*/

    /* reception des informations de connexion des autres */
    /* processus envoyees par le lanceur : */
    /* nom de machine, numero de port, etc. */

    /* initialisation des connexions */
    /* avec les autres processus : connect/accept */

    /* Allocation des pages en tourniquet */
    for(index = 0; index < PAGE_NUMBER; index ++){
      if ((index % (DSM_NODE_NUM+1)) == DSM_NODE_ID)
      dsm_alloc_page(index);
      dsm_change_info( index, WRITE, index % (DSM_NODE_NUM+1));
    }

    /* mise en place du traitant de SIGSEGV */
    memset(&act,0,sizeof(sigaction));
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = segv_handler;
    sigaction(SIGSEGV, &act, NULL);

  /*  //SIGUSR1 handler
    struct sigaction action_sigusr1;
    memset(&action_sigusr1,0,sizeof(struct sigaction));
    action_sigusr1.sa_handler=handle_sigusr1;
    sigaction(SIGUSR1,&action_sigusr1,NULL);*/


    /* creation du thread de communication */
    /* ce thread va attendre et traiter les requetes */
    /* des autres processus */
    pthread_create(&comm_daemon, NULL, dsm_comm_daemon, (void *)my_proc_array);
    // printf("fin : %d\n\n",DSM_NODE_ID);
    //fflush(stdout);
    /* Adresse de début de la zone de mémoire partagée */
    return ((char *)BASE_ADDR);
  }

  static void inform_quit(void)
  {
    int i=0;
    daemon_signs_t type=QUIT;
    for (i = 0; i < DSM_NODE_NUM; i++)
    {
      do_write(my_proc_array[i].fd,&type,sizeof(daemon_signs_t));
    }
  }

  void dsm_finalize( void )
  {
    /* fermer proprement les connexions avec les autres processus */
    /* terminer correctement le thread de communication */
    printf("[%d] attend les autres processus\n",DSM_NODE_ID);
    fflush(stdout);
    inform_quit();

    pthread_join(comm_daemon,NULL);
    int i=0;
    for(i=0;i<DSM_NODE_NUM;i++)
    {
      close(my_proc_array[i].fd);
    }
    free(my_proc_array);

    for(i=0;i<PAGE_NUMBER;i++)
    {
      if(get_owner(i)==DSM_NODE_ID)
      {
        dsm_free_page(i);
      }
    }

    /* pour le moment, on peut faire : */

    return;
  }
