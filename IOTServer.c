#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct sockaddr_in
{
   short sin_family; /*must be AF_INET*/
   u_short sin_port;
   struct in_addr sin_addr;
   char sin_zero[8];
} 

int main(int argc, char *argv[])
{ 
   int sockfd, newsockfd, portno, clilen, n; 
   // sockfd, newsockfd are file descriptors, portno stores the port number
   // on which the server accepts connections, clilen stores the size of the
   // address of the client, n is the return value of the read() and write()

  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  
