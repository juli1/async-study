/**
 * This is an example of how to use threads for doing
 * multiple communications synchronously
 *
 * Similar synchronuous version of the program available
 * in epoll.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

int sockets[NREQUESTS];
pthread_t threads[NREQUESTS];
char* message = NULL;


int socket_connect(char *host, in_port_t port)
{
   struct hostent *hp;
   struct sockaddr_in addr;
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

/**
 * Function worker for each thread
 */
void * thread_handling(void *arg)
{
   int* socket = arg;
   char buffer[BUFFER_SIZE];

   // send the request to get the page
   write(*socket, message, strlen(message));
   bzero(buffer, BUFFER_SIZE);
  
   // print the result
   while(read(*socket, buffer, BUFFER_SIZE - 1) != 0){
      fprintf(stdout, "%s", buffer);
      bzero(buffer, BUFFER_SIZE);
   }

   // close the socket
   shutdown(*socket, SHUT_RDWR); 
   close(*socket); 
}


int main(int argc, char *argv[])
{
   int fd;
   int n;

   if(argc < 3)
   {
      fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
      exit(1); 
   }

   message = malloc (100);
   sprintf (message,"GET / HTTP/1.0\r\nHost: %s\r\n\r\n",argv[1]);

   // Create the socket for each thread
   for (n = 0 ; n < NREQUESTS ; n++)
   {
      sockets[n] = socket_connect(argv[1], atoi(argv[2])); 
   }

   // Start the threads
   for (n = 0 ; n < NREQUESTS ; n++)
   {
      pthread_create(&threads[n], NULL, thread_handling, &sockets[n]);
   }

   // Wait for thread completions
   for (n = 0 ; n < NREQUESTS ; n++)
   {
      pthread_join(threads[n], NULL);
   }
   exit (1);
}

