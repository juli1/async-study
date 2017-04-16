/**
 * This is an example of how to use epoll for doing
 * asynchronous communication.
 *
 * Similar synchronuous version of the program available
 * in sync.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

/**
 * Structure to keep track of the client
 * state and have the FD. This is associated
 * with the epoll event.
 */
typedef struct {
   int state; // 0 = waiting for connection ; 1 = waiting for the reply
   int fd;   // associated file descriptor
}client_data_t;

int sockets[NREQUESTS];
client_data_t client_data[NREQUESTS];
char* message = NULL;
 
/**
 * Create a non-blocking socket 
 */
int create_socket(){
   int on = 1, sock;     
   int flags;
   sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

   if(sock == -1){
      perror("setsockopt");
      exit(1);
   }

   /* make the socket non blocking */
   flags = fcntl (sock, F_GETFL, 0);
   if (flags == -1)
   {
      perror ("fcntl");
      exit (1);
   }

   flags |= O_NONBLOCK;
   if (fcntl (sock, F_SETFL, flags) == -1)
   {
      perror ("fcntl");
      exit (1);
   }


   return sock;
}


int main(int argc, char *argv[]){
   int fd;
   int epoll_fd;
   struct sockaddr_in addr;
   int n;
   struct hostent *hp;
   struct epoll_event events[NREQUESTS];
   struct epoll_event event;
   int nfinished;

   nfinished = 0;

   if(argc < 3){
      fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
      exit(1); 
   }

   if((hp = gethostbyname(argv[1])) == NULL){
      herror("gethostbyname");
      exit(1);
   }
   bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
   addr.sin_port = htons(atoi(argv[2]));
   addr.sin_family = AF_INET;

   message = malloc (100);
   sprintf (message,"GET / HTTP/1.0\r\nHost: %s\r\n\r\n",argv[1]);

   /* create the epoll that will wait for all communications (main loop) */
   epoll_fd = epoll_create (NREQUESTS);
   if (epoll_fd == -1)
   {
      perror ("epoll");
      exit(2);
   }

   /* Create client sockets and add them to epoll */
   for (n = 0 ; n < NREQUESTS ; n++)
   {
      sockets[n] = create_socket(); 

      /*
       * state = 0 ; we are waiting the connect() to return
       */
      client_data[n].fd = sockets[n];
      client_data[n].state = 0;

      /*
       * associate the client_data with the event so that 
       * it is available when epoll returns
       */
      event.data.ptr = &client_data[n];
      event.events = EPOLLOUT;

      if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, sockets[n], &event) == -1)
      {
         perror ("epoll_ctl");
         exit (1);
      }
   }

   /* Start connecting to the server */
   for (n = 0 ; n < NREQUESTS ; n++)
   {
      if(connect(sockets[n], (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
      }

   }


   while (nfinished < NREQUESTS)
   {

      /* waiting for any epoll event */
      int answers = epoll_wait (epoll_fd, events, NREQUESTS, -1);
      for (int i = 0; i < answers; i++)
      {
         /* get the associated socket and state */
         int client_socket = ((client_data_t*)events[i].data.ptr)->fd;
         int client_state = ((client_data_t*)events[i].data.ptr)->state;

         /* if there is an error -> exit */
         if ((events[i].events & EPOLLERR) ||
               (events[i].events & EPOLLHUP))
         {
            perror("epoll_wait");
            exit(2);
         }


         /* If state == 0, it means the connect() just returns. Then, we send
          * the request to the HTTP server.
          */
         if (client_state == 0)
         {
            write(client_socket, message, strlen(message));
            /*
             * change the state, we are now waiting for data (state == 1)
             */
            ((client_data_t*)events[i].data.ptr)->state = 1;


            /*
             * now, we are no longer waiting for data to write but to read
             */
            struct epoll_event new_event;

            new_event.data.ptr = (client_data_t*)events[i].data.ptr;
            new_event.events = EPOLLIN;

            if (epoll_ctl (epoll_fd, EPOLL_CTL_MOD, client_socket, &new_event) == -1)
            {
               perror ("epoll_ctl");
               exit (1);
            }

         }
         else
         {
            while (1)
            {

               char buffer[BUFFER_SIZE + 1];
               memset(buffer,'\0', BUFFER_SIZE + 1);
               int size = read(client_socket, buffer, BUFFER_SIZE);

               // If read returns EAGAIN, it means there is nothing to read
               // for now, we will have more later.
               if (size == EAGAIN)
               {
                  break;
               }

               printf (buffer);

               /**
                * size == 0, there is nothing more to read, let's declare
                * this communication done
                */
               if (size == 0)
               {
                  if (epoll_ctl (epoll_fd, EPOLL_CTL_DEL, client_socket, &events[i]) == -1)
                  {
                     perror ("epoll_ctl del");
                     exit (3);
                  }
                  close (client_socket);
                  nfinished++;
                  break;
               }

               if ((size > 0) && (size < BUFFER_SIZE))
               {
                  break;
               }

            }

         }
      }
   }
   return 1;
}

