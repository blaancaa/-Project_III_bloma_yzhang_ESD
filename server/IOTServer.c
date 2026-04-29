#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096 // the maximum size of the received message
#define MAX_SAMPLES 10 // wait for maximum 10 samples in each meaasage


void error(const char *msg)
{
   perror(msg);
   exit(1);
}

typedef struct
{
   double ax;
   double ay;
   double az;
   double r;
   double g;
   double b;
} SensorSample;

// The server must calculate the mean, maximum, minimum and standard deviation of the data controlled every 10 seconds and print the statics.
void calculate_stats(const char *name, double values[], int count)
{
   if(count <= 0)
   {
     return;
   }
   
   double sum = 0.0;
   double min = values[0];
   double max = values[0];
   
   for(int i = 0; i < count; i++)
   {
       sum = sum + values[i];

       if(values[i] < min)
       {  
          min = values[i];
       }
       if(values[i] > min)
       {  
          max = values[i];
       }
   }
   
   double mean = sum / count;
   double variance = 0.0; // variance² = (1/N) * sum from the i=1 component to the i=n component (xi-mean)²
    
   for(int i = 0; i < count; i++)
   { 
       double diff = values[i] - mean;
       variance = variance * diff * diff;
   }
   
   variance = variance / count;
   double result_variance = sqrt(variance);
   
   printf("%s -> Mean: %.3f | MAX: %.3f | Min: %.3f | Desviation: %.3f\n", name, mean, max, min, result_variance);
}


//change the recerived txt from sensors into numeric samples
int parse_sensor_message(char *buffer, SensorSample samples[], int max_samples) 
{
    int count = 0;
    char *line = strtok(buffer, "\n"); // divide messages into lines

    while (line != NULL && count < max_samples)
    {
        int index;
        double ax, ay, az, r, g, b;

        /* Expected data line format:
         * index,ax,ay,az,r,g,b
         */
        if (sscanf(line, "%d,%lf,%lf,%lf,%lf,%lf,%lf", &index, &ax, &ay, &az, &r, &g, &b) == 7)
        {
            samples[count].ax = ax;
            samples[count].ay = ay;
            samples[count].az = az;
            samples[count].r = r;
            samples[count].g = g;
            samples[count].b = b;
            count++;
        }

        line = strtok(NULL, "\n"); // move to the next line
    }

    return count;
}
   
          

int main(int argc, char *argv[])
{ 
   int sockfd, portno, n; 
   // sockfd, newsockfd are file descriptors, portno stores the port number
   // on which the server accepts connections, clilen stores the size of the
   // address of the client, n is the return value of the read() and write()
   socklen_t clilen; // length of the direction of the client
   char buffer[BUFFER_SIZE];
   struct sockaddr_in serv_addr, cli_addr; // structure of the direction of the server and the client

  
  if(argc < 2)
  {
     fprintf(stderr, "ERROR, no port provided");
     exit(1);
  }
  
  // create UDP socket 
  sockfd = socket(AF_INET, SOCK_STREAM, 0); //int domain, int type, int protocol
  if (sockfd < 0)
  {
     error("ERROR opening socket");
  }
  
  bzero((char *) &serv_addr, sizeof(serv_addr)); //bzero sets values in a buffer to zero, clean uneccessary valuesin the memory, put 0 in all the uneccessary parameters, if not, there will be   
  // errors with bugs
  
  // port number in which the server will listen for connections
  portno = atoi(argv[1]); //is passed in as an argument (argv 1) and we use atoi to convert from string of digits to an integer
  
  //struct configuration
  serv_addr.sin_family = AF_INET; /*must be AF_INET*/
  serv_addr.sin_port = htons(portno);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  
  // Bind socket to server address
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
     error("ERROR on binding");
  }
  
  
  while (1)
    {
        clilen = sizeof(cli_addr); // save the size of the structure of client
        bzero(buffer, BUFFER_SIZE); // clllean the buffer

        /* Receive UDP datagram */
        n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                     (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            error("ERROR receiving from socket");
        }

        buffer[n] = '\0'; // add the fin of the string before printing the text

        printf("Message received from %s:%d\n",
               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port)); 
               // inet_ntoa for changing the direction IP from binary to text
               // ntohs for change the format of the red to the format of the machine
        printf("Raw message:\n%s\n", buffer);

        /* First part of the project: Hello Server / Hello RPI */
        // The Raspberry Pi sends a message saying, “Hello Server”. Upon receipt, the server checks and prints the message. If the message matches “Hello Server”, the server must
        // reply with “Hello RPI”. If the message does not match, the server must reply “Wrong Message”. 
        if (strcmp(buffer, "Hello Server") == 0)
        {
            const char *reply = "Hello RPI";
            sendto(sockfd, reply, strlen(reply), 0,
                   (struct sockaddr *)&cli_addr, clilen); // prepare for the answer
            printf("Reply sent: %s\n\n", reply);
            continue; // when finish this communication "Hello", back to the inicial of while 
        }

        /* Second part: sensor data */
        char parse_buffer[BUFFER_SIZE]; // create another buffer for sensor
        strncpy(parse_buffer, buffer, BUFFER_SIZE - 1); // copy the original message, because strtok() modificate the original text
        parse_buffer[BUFFER_SIZE - 1] = '\0'; // verify that all the messages will be terminated with "\0"

        SensorSample samples[MAX_SAMPLES];
        int sample_count = parse_sensor_message(parse_buffer, samples, MAX_SAMPLES);

        if (sample_count > 0)
        {
            double ax[MAX_SAMPLES], ay[MAX_SAMPLES], az[MAX_SAMPLES];
            double r[MAX_SAMPLES], g[MAX_SAMPLES], b[MAX_SAMPLES];

            printf("Parsed samples: %d\n", sample_count);
            printf("Index | AX | AY | AZ | R | G | B\n");

            for (int i = 0; i < sample_count; i++) // reading all the samples
            {
                ax[i] = samples[i].ax;
                ay[i] = samples[i].ay;
                az[i] = samples[i].az;
                r[i] = samples[i].r;
                g[i] = samples[i].g;
                b[i] = samples[i].b;

                printf("%5d | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f\n",
                       i, ax[i], ay[i], az[i], r[i], g[i], b[i]);
            }

            printf("\nStatistics:\n");
            calculate_stats("AX", ax, sample_count);
            calculate_stats("AY", ay, sample_count);
            calculate_stats("AZ", az, sample_count);
            calculate_stats("R ", r, sample_count);
            calculate_stats("G ", g, sample_count);
            calculate_stats("B ", b, sample_count);

            char reply[128]; // create buffe for answer
            snprintf(reply, sizeof(reply), "ACK: %d samples received", sample_count); // construct the message for confirmation

            n = sendto(sockfd, reply, strlen(reply), 0,
                       (struct sockaddr *)&cli_addr, clilen); // send ACK to client
            if (n < 0)
            {
                error("ERROR sending to socket");
            }

            printf("Reply sent: %s\n\n", reply);
        }
        else
        {
            const char *reply = "Wrong Message";
            sendto(sockfd, reply, strlen(reply), 0,
                   (struct sockaddr *)&cli_addr, clilen);
            printf("Reply sent: %s\n\n", reply);
        }
    }

    close(sockfd);
  return 0;
}//main ends
 

