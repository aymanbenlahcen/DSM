#include "handling_socket.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <poll.h>


/* variables globales */
typedef int pipe_t[2];

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL;

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

void usage(void)
{
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void sigchld_handler(int sig)
{
  /* on traite les fils qui se terminent */
  /* pour eviter les zombies */
  int res;

  do {
    res = waitpid(-1, NULL, WNOHANG);
  }while(res > 0);

}


int main(int argc, char *argv[])
{
  if (argc < 3){
    usage();
  }
  else {
    pid_t pid;
    int num_procs = 0;
    int i,j;
    FILE *fd_machine_file;
    int nb_lines_machine_file=0;
    struct sigaction signal_sigchld;
    struct sockaddr_in sock_serv,sock_client;
    int port_num;
    char ip[17]="";
    int listen_sock;
    socklen_t addrlen;
    int *tab_sock=NULL;
    struct pollfd *pollfds_fils=NULL;
    char str[1024];
    char *current_directory=getcwd(str,1024);
    /* Mise en place d'un traitant pour recuperer les fils zombies*/
    /* XXX.sa_handler = sigchld_handler; */

    memset (&signal_sigchld, 0, sizeof(signal_sigchld));
    signal_sigchld.sa_handler=sigchld_handler;
    sigaction(SIGCHLD, &signal_sigchld, NULL);

    /* lecture du fichier de machines */
    /* 1- on recupere le nombre de processus a lancer */
    fd_machine_file=fopen(argv[1],"r");
    nb_lines_machine_file= nb_read_lines(fd_machine_file);
    if (fd_machine_file == NULL)
    {
      ERROR_EXIT("Error in opening the file");
    }
    //nb_lines_machine_file=nb_read_lines("machine_file");
    /* 2- on recupere les noms des machines : le nom de */
    /* la machine est un des elements d'identification */
    //tab_machine=malloc(nb_lines_machine_file*100*sizeof(char));
    num_procs=nb_lines_machine_file;
    proc_array= malloc(num_procs * sizeof(dsm_proc_t));

    get_machine_file(fd_machine_file,proc_array);

    /* creation de la socket d'ecoute */
    /* + ecoute effective */
    srand(getpid());

    listen_sock=do_socket(AF_INET,SOCK_STREAM,0);
    do{
      init_serv_addr(&port_num,ip,&sock_serv);
      addrlen=sizeof(sock_serv);
    }while(do_bind(listen_sock,(struct sockaddr *) &sock_serv,addrlen)==-1);

    do_listen(listen_sock,nb_lines_machine_file);
    fflush(stdout);
    /* creation des fils */
    pollfds_fils= malloc(num_procs * 2* sizeof(struct pollfd));
    memset(pollfds_fils,0, sizeof(struct pollfd) * 2*num_procs);

    for(i = 0; i < num_procs ; i++) {

      /* creation du tube pour rediriger stdout */
      pipe_t stdout_tubes;
      if(pipe(stdout_tubes)==-1)
      ERROR_EXIT("Erreur création du tube stdout du fils\n");

      /* creation du tube pour rediriger stderr */
      pipe_t stderr_tubes;
      if(pipe(stderr_tubes)==-1)
      ERROR_EXIT("Erreur création du tube stdout du fils\n");

      pid = fork();

      if(pid == -1) ERROR_EXIT("fork");

      if (pid == 0) { /* fils */

        /* redirection stdout */
        close(stdout_tubes[0]);
        dup2(stdout_tubes[1],STDOUT_FILENO);

        /* redirection stderr */
        close(stderr_tubes[0]);
        dup2(stderr_tubes[1],STDERR_FILENO);

        /* Creation du tableau d'arguments pour le ssh */
        char **newargv=(char **)malloc((argc+5)*sizeof(char *));
        newargv[0]="ssh";
        newargv[1]=proc_array[i].connect_info.name;
        char tmp_curr_dir[2048];
        sprintf(tmp_curr_dir,"%s/bin/%s",current_directory,argv[2]);
        newargv[2]=tmp_curr_dir;
        newargv[3]=ip;
        char tmp[20];
        sprintf(tmp,"%d",port_num);
        newargv[4] = tmp;
        char temp[20];
        sprintf(temp,"%d",i);
        newargv[5]=temp;

        for(j=6;j<argc+4;j++)
        {
          newargv[j]=argv[j-4];
        }
        newargv[argc + 4]  = NULL;

        /*for(j=0;j<argc+4;j++)
        {
          fprintf(stdout,"argv[%d]=%s\n\n\n\n",j,newargv[j]);
          fflush(stdout);
        }*/
        /* jump to new prog : */
        execvp("ssh",newargv);
      } else  if(pid > 0) { /* pere */
        /* fermeture des extremités des tubes non utiles */
        close(stdout_tubes[1]);
        close(stderr_tubes[1]);

        pollfds_fils[2*i].fd=stderr_tubes[0];
        pollfds_fils[2*i+1].fd=stdout_tubes[0];
        pollfds_fils[2*i].events = POLLIN;
        pollfds_fils[2*i+1].events = POLLIN;

        fcntl(stdout_tubes[0],F_SETFL, O_NONBLOCK);
        fcntl(stderr_tubes[0], F_SETFL, O_NONBLOCK);


        proc_array[i].connect_info.rank = i;
        num_procs_creat++;
      }
    }

    tab_sock = malloc(num_procs * sizeof(int));

    for(i = 0; i < num_procs_creat ; i++){
      /* on accepte les connexions des processus dsm */
      memset(&sock_client,'\0',sizeof(sock_client));
      do {
        tab_sock[i]=do_accept(listen_sock, (struct sockaddr *) &sock_client, &addrlen);
      }while (tab_sock[i] == -1 && errno==EINTR);

      /*  On recupere le nom de la machine distante */
      /* 1- d'abord la taille de la chaine */
      /* 2- puis la chaine elle-meme */
      int rang;
      do_read(tab_sock[i],(void *)&rang,sizeof(int));
      printf("rang test : %d %d\n\n",rang,proc_array[rang].connect_info.rank);
      fflush(stdout);
      sprintf(proc_array[rang].connect_info.ip,"%s", inet_ntoa(sock_client.sin_addr));

      /* On recupere le pid du processus distant  */
      pid_t pid_fils;
      do_read(tab_sock[i],(void *)&pid_fils,sizeof(pid_t));
      proc_array[rang].pid = pid_fils;

      /* On recupere le numero de port de la socket */
      /* d'ecoute des processus distants */
      do_read(tab_sock[i],&port_num,sizeof(int));
      proc_array[rang].connect_info.port=port_num;
      proc_array[rang].connect_info.fd=tab_sock[i];
      printf("%d %s %d %d  %d\n\n",tab_sock[i],proc_array[rang].connect_info.name,proc_array[rang].connect_info.rank,proc_array[rang].pid,port_num);
      fflush(stdout);

    }

    /* envoi du nombre de processus aux processus dsm*/
    for(i = 0; i < num_procs ; i++){
      do_write(tab_sock[i],&num_procs,sizeof(int));
    }

    /* envoi des infos de connexion aux processus */
    for(i = 0; i < num_procs ; i++){
      for(j=0;j<num_procs;j++){
        if(j!=i){
          do_write(proc_array[i].connect_info.fd,&proc_array[j].connect_info,sizeof(dsm_proc_conn_t));
        }
      }
    }
    int sync=2;
    for(i = 0; i < num_procs ; i++){
          do_read(tab_sock[i],&sync,sizeof(int));
    }

    sync=2;
    for(i=0;i<num_procs;i++){
      do_write(tab_sock[i],&sync,sizeof(int));
      close(tab_sock[i]);
    }

    free(tab_sock);

    /* gestion des E/S : on recupere les caracteres */
    /* sur les tubes de redirection de stdout/stderr */
    int ret_poll;

    /*for( i=0;i<num_procs;i++)
    {
      printf("%d : stderr : %d %d stdout : %d %d\n\n",i,pollfds_fils[2*i].fd,pollfds_fils[2*i].events,pollfds_fils[i+1].fd,pollfds_fils[2*i+1].events);
      fflush(stdout);
    }*/
  //  int reshuffle=0; // si on doit compresser le tableau car on a fermé des sockets
    int num_procs_tmp=num_procs;
    while(num_procs_tmp>0)
    {
      /*je recupere les infos sur les tubes de redirection
      jusqu'à ce qu'ils soient inactifs (ie fermes par les
      processus dsm ecrivains de l'autre cote ...)*/
      do{
        ret_poll = poll(pollfds_fils,2*num_procs, -1);
      }while(ret_poll==-1 && errno==EINTR);

      if (ret_poll < 0)
      {
        ERROR_EXIT("poll() failed");
      }
      else if (ret_poll == 0)
      {
        ERROR_EXIT("time out connexion\n");
      }

      //processing every file descriptors
      for (i = 0; i < num_procs*2; i++)
      {
        //printf("revent %i : %d %d %d\n\n",i,pollfds_fils[i].revents,POLLIN,POLLHUP);
        //fflush(stdout);
        if(pollfds_fils[i].revents == 0){  //if there is no activity on the socket we go directly to the next one
          continue;
        }
        else if(!(pollfds_fils[i].revents & POLLIN) && !(pollfds_fils[i].revents & POLLHUP)){   //if there is an error on the current socket
          char err[10];
          sprintf(err,"%d",pollfds_fils[i].revents);
          ERROR_EXIT(err);
        }
        else if(pollfds_fils[i].revents & POLLHUP)
        {
          //printf("%d %d\n",pollfds_fils[i].fd,pollfds_fils[i+1].fd);
          //fflush(stdout);
          printf("Processus %d has terminated\n",i/2);
          fflush(stdout);
          close(pollfds_fils[i].fd);
          pollfds_fils[i].fd=-1;
          pollfds_fils[i].revents=0;
          if(!(i%2)){
            close(pollfds_fils[i+1].fd);
            pollfds_fils[i+1].fd=-1;
            pollfds_fils[i+1].revents=0;
          }
          else{
            close(pollfds_fils[i-1].fd);
            pollfds_fils[i-1].fd=-1;
            pollfds_fils[i-1].revents=0;
          }
          //reshuffle=1;
          num_procs_tmp--;
        }
        else{
          char c;
          if(i%2)
            fprintf(stdout, "%s %i stdout :\n",proc_array[i/2].connect_info.name,proc_array[i/2].connect_info.rank);
          else
            fprintf(stdout, "%s %i stderr :\n",proc_array[i/2].connect_info.name,proc_array[i/2].connect_info.rank);

          int ret=do_read(pollfds_fils[i].fd,&c,sizeof(char));
          printf("%c",c);
          fflush(stdout);
          while(ret>0){
            ret=do_read(pollfds_fils[i].fd,&c,sizeof(char));
            printf("%c",c);
            fflush(stdout);
          }
          fprintf(stdout, "\n");
          fflush(stdout);
        }
      }
      /*if(reshuffle){
        reshuffle_pollfd(pollfds_fils,&num_procs);
        reshuffle=0;
      }*/
    }

    /* on attend les processus fils */
    //fait dans le signal handler
    /* on ferme les descripteurs proprement */
    //fait dans la lecture de tube
    /* on ferme la socket d'ecoute */
    close(listen_sock);
    /* on nettoie les variables du tas*/
    free(pollfds_fils);
    free(proc_array);

  }
  printf("fin totale\n\n");
  fflush(stdout);
  exit(EXIT_SUCCESS);
}
