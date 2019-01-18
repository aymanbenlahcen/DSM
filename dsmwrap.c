#include "common_impl.h"
#include "handling_socket.h"
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

typedef struct arg{
   int port;
   char ip[17];
}arg_t;


void *create_server(void *s)
{
  int *socket=(int *)s;

  int rang=0;
  do_read(*socket,&rang,sizeof(int));
  printf("Fd de mon pote est %d\n",rang);
  fflush(stdout);
  do_write(*socket,socket,sizeof(int));

  return 0;
}

void *create_connection(void *s)
{

  arg_t *args=(arg_t *) s;
  int port=args->port;
  char ip[17];
  strcpy(ip,args->ip);
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

  int rang=0;
  do_write(sock,&sock,sizeof(int));
  do_read(sock,&rang,sizeof(int));
  printf("Fd de mon pote le serveur est %d\n",rang);
  fflush(stdout);
  printf("test\n");
  fflush(stdout);

  return 0;
}

int main(int argc, char **argv)
{
   /* processus intermediaire pour "nettoyer" */
   /* la liste des arguments qu'on va passer */
   /* a la commande a executer vraiment */

   /* creation d'une socket pour se connecter au */
   /* au lanceur et envoyer/recevoir les infos */
   /* necessaires pour la phase dsm_init */
   char ip[17];
   char *port_launcher;
   struct sockaddr_in sock_serv ;
   int s;
   int my_rank=0;


   strcpy(ip,argv[1]);
   port_launcher=argv[2];
   my_rank=atoi(argv[3]);

   //get address info from the server
   get_addr(ip,port_launcher,&sock_serv);

   //get the socket
   s = do_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
   //connect to the server remote socket
   do_connect(s,(struct sockaddr*)&sock_serv,sizeof(sock_serv));

   /* Envoi du nom de machine au lanceur */
   //char name[NAME_MACHINE_SIZE];
   //gethostname(name,NAME_MACHINE_SIZE);
   do_write(s,&my_rank,sizeof(int));
   /* Envoi du pid au lanceur */
   pid_t pid = getpid();
   do_write(s,&pid,sizeof(pid_t));

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
   int nb_proc_dist=0;

   do_read(s,&nb_proc_dist,sizeof(int));
   nb_proc_dist--;
   do_listen(listen_sock,nb_proc_dist);

   //printf("nb_proc : %d\n",nb_proc_dist);
   //fflush(stdout);
   //récupération du tableau proc_array
   dsm_proc_conn_t *my_proc_array = malloc(nb_proc_dist * sizeof(dsm_proc_conn_t));
   int i;

   for(i=0;i<nb_proc_dist;i++)
   {
     do_read(s,&my_proc_array[i],sizeof(dsm_proc_conn_t));
   }

   //se connecte aux autres machines
   pthread_t *tab_thread_connection=malloc(nb_proc_dist-my_rank*sizeof(pthread_t));
   if(nb_proc_dist-my_rank>0){
     arg_t arguments;
     memset(arguments.ip,'\0',17*sizeof(char));
     for(i=0;i<nb_proc_dist-my_rank;i++)
     {
       arguments.port=my_proc_array[i].port;
       strcpy(arguments.ip,my_proc_array[i].ip);
       printf("rang : %d %d %s\n",my_rank,arguments.port,arguments.ip);
       fflush(stdout);
       int res=pthread_create(&tab_thread_connection[i],NULL,&create_connection,(void *)&arguments);
       printf("res1 : %d\n\n",res);
       fflush(stdout);
     }
     printf("prout Je suis %d et je suis mort et mon rang est %d\n\n",getpid(),my_rank);
     fflush(stdout);
   }
   pthread_t *tab_thread_server =malloc(my_rank*sizeof(pthread_t));
   if(my_rank>0){
     struct sockaddr_in sock_client;
     int s_create;
     socklen_t addrlen;
     for(i=0;i<my_rank;i++)
     {
       memset(&sock_client,'\0',sizeof(sock_client));
       do{
         addrlen=sizeof(sock_client);
         s_create=do_accept(listen_sock, (struct sockaddr *) &sock_client, &addrlen);
         printf("Je suis %d et je suis mort et mon rang est %d\n\n",getpid(),my_rank);
         fflush(stdout);
       }while(s_create==-1 && errno==EINTR);
       printf("rank : %d\n",my_rank);
       fflush(stdout);
       int res=pthread_create(&tab_thread_server[i],NULL,&create_server,&s_create);
       printf("res2 : %d\n\n",res);
       fflush(stdout);
     }
   }

   char **newargv=(char **)malloc((argc-3)*sizeof(char *));
   int j;
   for(j=0;j<argc-3;j++)
   {
     newargv[j]=argv[j+3];
   }

   if(my_rank>0){

   pthread_join(tab_thread_server[0],NULL);
   printf("test1\n");
   fflush(stdout);
    }
   else{
     pthread_join(tab_thread_connection[0],NULL);
     printf("test2\n");
     fflush(stdout);
   }

   /*
   printf("Je suis %d et je suis mort et mon rang est %d\n\n",getpid(),my_rank);
   fflush(stdout);*/
   //execv(newargv[0],newargv);
   return 0;
}
