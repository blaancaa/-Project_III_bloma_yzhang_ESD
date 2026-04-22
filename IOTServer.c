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
  
  if(argc < 2)
  {
     fprintf(stderr, "ERROR, no port provided");
     exit(1);
  }
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0); //int domain, int type, int protocol
  if (sockfd < 0)
  {
     error("ERROR opening socket");
  }
  
  bzero((char *) &serv_addr, sizeof(serv_addr)); //bzero sets values in a buffer to zero
  
  // port number in which the server will listen for connections
  portno = atoi(argv[1]); //is passed in as an argument (argv 1) and we use atoi to convert from string of digits to an integer
  
  //struct configuration
  serv_addr.sin_family = AF_INET; /*must be AF_INET*/
  serv_addr.sin_port = htons(portno);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
     error("ERROR on binding");
  }
  
  listen(sockfd,5); //allows the process to listen to on the socket for connection   ~/.local/share/keyrings
  
  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (newsockfd < 0)
  {
    error("ERROR on accept");
  }
  
  bzero(buffer,256);
  n = read(newsockfd,buffer,255); // we would only get to this point after a client has successfully connected to our server.  
  //This code initializes the buffer using the bzero() function, and then reads from the socket. Note that the read call 
  //the new file descriptor
  if (n < 0) error("ERROR reading from socket");
  {
    printf("Here is the message: %s",buffer);
  }
  
  n = write(newsockfd,"I got your message",18);
  if (n < 0) 
  {
    error("ERROR writing to socket");
  }
  
  return 0;
}//main ends
 
void error(char *msg)
{
  perror(msg);
  exit(1); 
}
