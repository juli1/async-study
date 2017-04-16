/**
 * This is an attempt to use the POSIX aio framework.
 * The attempt has been aborted because the framework
 * is use hard to use, non performante and there are
 * way better ways to do async I/O on Linux.
 *
 * See epoll.c for more details
 *
 * WARNING: this code DOES NOT work
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <aio.h>
#include <signal.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 1024

typedef struct {
   struct aiocb* aio;
   int id;
   int socket;
   int bytes_read;
}request_t;


struct aiocb* aiocb_list[NREQUESTS];
request_t requests[NREQUESTS];
int sockets[NREQUESTS];

int socket_connect(char *host, in_port_t port){
   struct sockaddr_in addr;
   struct hostent *hp = NULL;
   int on = 1, sock;     

   if((hp = gethostbyname(host)) == NULL){
      herror("gethostbyname");
      exit(1);
   }

   bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
   addr.sin_port = htons(port);
   addr.sin_family = AF_INET;
   sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

   if(sock == -1){
      perror("setsockopt");
      exit(1);
   }

   if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
      perror("connect");
      exit(1);

   }
   return sock;
}

volatile int completed = 0;

void aio_completion_handler( int signo, siginfo_t *info, void *context )
{
   struct aiocb *req;
   request_t* r;

   /* Ensure it's our signal */
   if (info->si_signo == SIGIO) {

      r = (request_t*)info->si_value.sival_ptr;
      printf("Worker %d, socket %d\n", r->id, r->socket);
      req = r->aio;


      if (req->aio_lio_opcode == LIO_READ)
      {
         int ret = aio_return( req );
         printf ("Worker %d, Socket %d read, ret=%d\n", r->id, req->aio_fildes, ret);
         if (aio_error( req ) == 0) {
            if (ret == 0)
            {

               shutdown(req->aio_fildes, SHUT_RDWR); 
               close(req->aio_fildes);
               completed++;
               printf ("Worker %d, completed=%d\n", r->id, completed);
            }
            else
            {
               r->bytes_read+= ret;
               req->aio_offset = r->bytes_read;
               memset(req->aio_buf, '\0', 1024);

               if (aio_read(req))
               {
                  printf ("error during read\n");
               }

            }
         }
         return;
      }

      if (req->aio_lio_opcode == LIO_WRITE)
      {
         if (aio_error( req ) == 0) {
            memset(req->aio_buf, '\0', 1024);
            req->aio_nbytes = 1024;
            req->aio_offset = 0;


            if (aio_read(req))
            {
               printf ("error during write\n");
            }
         }
         return;
      }

   }

   return;
}

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]){
   int fd;
   struct sigaction sig_act;

   if(argc < 3){
      fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
      exit(1); 
   }

   /* Set up the signal handler */
   sigemptyset(&sig_act.sa_mask);
   sig_act.sa_flags = SA_RESTART | SA_SIGINFO;
   sig_act.sa_sigaction = aio_completion_handler;
   if (sigaction( SIGIO, &sig_act, NULL ))
   {
      printf ("sigaction error");
   }

   char* message = malloc (100);
   sprintf (message,"GET / HTTP/1.0\r\nHost: %s\r\n\r\n",argv[1]);

   int n;
   for (n = 0 ; n < NREQUESTS ; n++)
   {
      requests[n].id = n;
      requests[n].bytes_read = 0;
      requests[n].socket = socket_connect(argv[1], atoi(argv[2])); 
      printf ("Worker %d socket %d\n", n, sockets[n]);
   }


   for (n = 0 ; n < NREQUESTS ; n++)
   {
      struct aiocb* my_aiocb = malloc (sizeof(struct aiocb));
      requests[n].aio = my_aiocb;
      bzero( (char *)my_aiocb, sizeof(struct aiocb) );
      my_aiocb->aio_fildes = requests[n].socket;
      my_aiocb->aio_buf = malloc(BUFSIZE);
      memcpy(my_aiocb->aio_buf, message, strlen(message));
      my_aiocb->aio_nbytes = strlen(message);
      my_aiocb->aio_offset = 0;

      /* Link the AIO request with a thread callback */
      my_aiocb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
      my_aiocb->aio_sigevent.sigev_signo = SIGIO;
      my_aiocb->aio_sigevent.sigev_value.sival_ptr = &(requests[n]);

      aiocb_list[n] = my_aiocb;

      if (aio_write(my_aiocb))
      {
         printf ("error during write\n");
      }

   }

   while (completed < NREQUESTS)
   {
   }

   return 0;
}

