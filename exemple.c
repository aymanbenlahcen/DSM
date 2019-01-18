#include "dsm.h"

int main(int argc, char **argv)
{
   char *pointer;
   char *current;
   int value;


   pointer = dsm_init(argc,argv);
   current = pointer;

   printf("[%i] Coucou, mon adresse de base est : %p\n", DSM_NODE_ID, pointer);
   fflush(stdout);

   /*
   if (DSM_NODE_ID == 0)
     {

       current += 4*sizeof(int);
       value = *((int *)current);

       printf("[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
       fflush(stdout);
       current += PAGE_SIZE;
        *((int *)current) = 1800;
        value = *((int *)current);
        fprintf(stdout, "[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
        fflush(stdout);

    }
    else if (DSM_NODE_ID == 1)
     {
       current += PAGE_SIZE;
       current += 16*sizeof(int);

       value = *((int *)current);
       printf("[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
       fflush(stdout);

      /*current -= PAGE_SIZE;
       *((int *)current) = 1200;
       value = *((int *)current);
       fprintf(stdout, "[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
       fflush(stdout);
     }
     else if (DSM_NODE_ID == 2) {

       current += 4*sizeof(int);
       ((int *)current) = 960;
       value = *((int *)current);
       fprintf(stdout, "[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
       fflush(stdout);
     }*/


       current+=DSM_NODE_ID*PAGE_SIZE;

       value = *((int *)current);
       printf("[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
       fflush(stdout);

       int i,max=1;
       int j,max2=1;
       current+=DSM_NODE_ID*PAGE_SIZE;
       for(i=0;i<max;i++){
         for(j=0;j<max2;j++){
           if(DSM_NODE_ID==j){
             current += PAGE_SIZE;

             *((int *)current) = 1200;
             value = *((int *)current);
             printf("[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
             fflush(stdout);
          }
        }
       }

   dsm_finalize();

   return 1;
}
