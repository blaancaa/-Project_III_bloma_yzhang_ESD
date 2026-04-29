#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <netdb.h>

#define SERVER_BUFFER_SIZE 4096
#define REPLY_BUFFER_SIZE 256
#define MAX_SAMPLES 10

#define I2C_DEVICE "/dev/i2c-1"
#define acc_ADDR 0x68
#define color_ADDR 0x29

#define acc_PWR_MGMT_1 0x6B
#define acc_ACCEL_XOUT_H 0x3B
#define acc_SENSITIVITY 16384.0

#define color_BIT 0x80
#define color_ENABLE 0x00
#define color_ATIME 0x01
#define color_ID 0x12
#define color_CDATAL 0x14

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

// function to write information to I2C
int i2c_write_bytes(int fd, int addr, unsigned char *data, int len)
{
    struct i2c_rdwr_ioctl_data packets; // sturcture for I2C
    struct i2c_msg messages[1];

    messages[0].addr = addr;
    messages[0].flags = 0;
    messages[0].len = len;
    messages[0].buf = data;

    packets.msgs = messages;
    packets.nmsgs = 1;

    return ioctl(fd, I2C_RDWR, &packets);
}

int i2c_read_register(int fd, int addr, unsigned char reg, unsigned char *data, int len)
{
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    messages[0].addr = addr;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = &reg;

    messages[1].addr = addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len = len;
    messages[1].buf = data;

    packets.msgs = messages;
    packets.nmsgs = 2;

    return ioctl(fd, I2C_RDWR, &packets);
}

int sensors_init(void)
{
    int fd;
    unsigned char data[2];
    unsigned char id;

    fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0)
    {
        error("ERROR opening I2C device");
    }

    /* Wake up MPU6050 */
    data[0] = acc_PWR_MGMT_1;
    data[1] = 0x00;
    if (i2c_write_bytes(fd, acc_ADDR, data, 2) < 0)
    {
        error("ERROR waking up MPU6050");
    }

    usleep(100000);

    /* Read TCS34725 ID */
    if (i2c_read_register(fd, color_ADDR, color_ID | color_BIT, &id, 1) < 0)
    {
        error("ERROR reading TCS34725 ID");
    }

    printf("TCS34725 Sensor ID: 0x%02X\n", id);

    /* Enable TCS34725: PON + AEN */
    data[0] = color_ENABLE | color_BIT;
    data[1] = 0x03;
    if (i2c_write_bytes(fd, color_ADDR, data, 2) < 0)
    {
        error("ERROR enabling TCS34725");
    }

    usleep(3000);

    /* ATIME = 0xD5, about 100 ms integration time */
    data[0] = color_ATIME | color_BIT;
    data[1] = 0xD5;
    if (i2c_write_bytes(fd, color_ADDR, data, 2) < 0)
    {
        error("ERROR configuring TCS34725 ATIME");
    }

    usleep(120000);

    return fd;
}

int read_sensors(int fd, SensorSample *sample)
{
    unsigned char acc_data[6];
    unsigned char color_data[8];

    if (i2c_read_register(fd, acc_ADDR, acc_ACCEL_XOUT_H, acc_data, 6) < 0)
    {
        return -1;
    }

    int16_t acc_x = (int16_t)((acc_data[0] << 8) | acc_data[1]);
    int16_t acc_y = (int16_t)((acc_data[2] << 8) | acc_data[3]);
    int16_t acc_z = (int16_t)((acc_data[4] << 8) | acc_data[5]);

    sample->ax = acc_x / acc_SENSITIVITY;
    sample->ay = acc_y / acc_SENSITIVITY;
    sample->az = acc_z / acc_SENSITIVITY;

    if (i2c_read_register(fd, color_ADDR, color_CDATAL | color_BIT, color_data, 8) < 0)
    {
        return -1;
    }

    uint16_t clear = (uint16_t)((color_data[1] << 8) | color_data[0]);
    uint16_t red = (uint16_t)((color_data[3] << 8) | color_data[2]);
    uint16_t green = (uint16_t)((color_data[5] << 8) | color_data[4]);
    uint16_t blue = (uint16_t)((color_data[7] << 8) | color_data[6]);

    if (clear == 0)
    {
        sample->r = 0;
        sample->g = 0;
        sample->b = 0;
    }
    else
    {
        sample->r = ((double)red / clear) * 255.0;
        sample->g = ((double)green / clear) * 255.0;
        sample->b = ((double)blue / clear) * 255.0;
    }

    return 0;
}


// construction the message UDP
void build_sensor_message(char *buffer, int buffer_size, SensorSample samples[], int count)
{
    int offset = 0; // indicate the writing position in the buffer

    bzero(buffer, buffer_size);

    for (int i = 0; i < count; i++)
    {
        offset += snprintf(buffer + offset, buffer_size - offset,
                           "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                           i,
                           samples[i].ax,
                           samples[i].ay,
                           samples[i].az,
                           samples[i].r,
                           samples[i].g,
                           samples[i].b); // format going to be printed: index, x, y,z, r, g, b

        if (offset >= buffer_size)
        {
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    int i2c_fd;
    socklen_t serverlen;
    char send_buffer[SERVER_BUFFER_SIZE];
    char reply_buffer[REPLY_BUFFER_SIZE];
    struct sockaddr_in serv_addr;
    struct hostent *server;
    SensorSample samples[MAX_SAMPLES];

    if (argc < 3)
    {
        fprintf(stderr, "ERROR, server IP and port required\n");
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(0);
    }

    portno = atoi(argv[2]);

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));

   server = gethostbyname(argv[1]);
   if(server == NULL)
   {
        fprintf(stderr, "ERROR: no such host");
        exit(0);
   }
   
   bzero((char*) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(portno);
   bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
   

   serverlen = sizeof(serv_addr);

    /* Initialize sensors */
    i2c_fd = sensors_init();

    while (1)
    {
        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            if (read_sensors(i2c_fd, &samples[i]) < 0) // read a sample and save it in sample[i]
            {
                error("ERROR reading sensors");
            }

            printf("Sample %d -> AX: %.3f AY: %.3f AZ: %.3f R: %.3f G: %.3f B: %.3f\n",
                   i,
                   samples[i].ax,
                   samples[i].ay,
                   samples[i].az,
                   samples[i].r,
                   samples[i].g,
                   samples[i].b);

            sleep(1); // the system take 1 sample in a second
        }

        build_sensor_message(send_buffer, SERVER_BUFFER_SIZE, samples, MAX_SAMPLES); // change 10 samples into text

        /* Send UDP datagram to server */
        n = sendto(sockfd, send_buffer, strlen(send_buffer), 0,
                   (struct sockaddr *)&serv_addr, serverlen);
        if (n < 0)
        {
            error("ERROR sending to socket");
        }

        printf("\nData sent to server:\n%s", send_buffer);

        bzero(reply_buffer, REPLY_BUFFER_SIZE);

        /* Receive server reply */
        n = recvfrom(sockfd, reply_buffer, REPLY_BUFFER_SIZE - 1, 0,
                     (struct sockaddr *)&serv_addr, &serverlen);
        if (n < 0)
        {
            error("ERROR receiving from socket");
        }

        reply_buffer[n] = '\0';
        printf("Server reply: %s\n\n", reply_buffer);
    }

    close(i2c_fd);
    close(sockfd);
    return 0;
}
