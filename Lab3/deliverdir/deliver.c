#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>

// constants used for the estimated RTT
#define ALPHA 0.125
#define BETA 0.25

// main goal: to make use of UDP and send between client and server.
/*
Done by:
Shikhar Chaurasia (Student # 1006710016)
and
Gunin Wasan (Student # 1007147749)
*/
int resent_flags[500000] = {0};
struct Packet
{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char *filename;
    char filedata[1000];
};

int timeval_subtract (struct timeval *result, struct timeval *x,struct timeval  *y)  
{  
  /* Perform the carry for the later subtraction by updating y. */  
  if (x->tv_usec < y->tv_usec) {  
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;  
    y->tv_usec -= 1000000 * nsec;  
    y->tv_sec += nsec;  
  }  
  if (x->tv_usec - y->tv_usec > 1000000) {  
    int nsec = (y->tv_usec - x->tv_usec) / 1000000;  
    y->tv_usec += 1000000 * nsec;  
    y->tv_sec -= nsec;  
  }  
  
  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */  
  result->tv_sec = x->tv_sec - y->tv_sec;  
  result->tv_usec = x->tv_usec - y->tv_usec;  
  
  /* Return 1 if result is negative. */  
  return x->tv_sec < y->tv_sec;  
}

// function to count number of digits to send as a packet
int count_digits(int number){
    int c= 0;
    while(number!=0){
        number/=10;
        c++;
    }
    return c;
}

// function that checks if the file exists or not
bool does_file_exist(const char *file_name)
{
    struct stat buffer;
    return stat(file_name, &buffer) == 0 ? true : false;
}

int main(int argc, char **argv)
{
    // incorrect usage of the client
    if (argc != 3)
    {
        fprintf(stderr, "client usage: deliver <server address> <udp port num>\n");
        exit(0);
    }
    // initialization of variables
    int port_number = atoi(argv[1]);
    int srv_socket_fd;
    struct addrinfo hints;
    struct addrinfo *server_info, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM; // for datagram sockets (UDP)
    // do a dns lookup and get the ip addresses in the form of a linked list stored into server_info
    if ((getaddrinfo(argv[1], argv[2], &hints, &server_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo\n");
        exit(2);
    }
    // iterate thru the linked list and create a socket
    for (p = server_info; p != NULL; p = p->ai_next)
    {
        if ((srv_socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("socket");
            continue;
        }
        break;
    }
    if (p == NULL)
    {
        perror("socket\n");
        exit(0);
    }
    // socket creation was successful

    char protocol[256] = {0}, file_name[256] = {0}, protocol_file[256] = {0};
    printf("Enter file to be transferred. Format: ftp <filename>\n");
    fgets(protocol_file, 256, stdin);
    // if user enters something like ftpl
    if (protocol_file[3] != ' ')
    {
        printf("ftp not used. exiting.\n");
        exit(0);
    }
    // copy the protocol part of the input to protocol
    strncpy(protocol, protocol_file, 3);
    // null terminate it for better usage.
    protocol[3] = '\0';
    // check if protocol actually was ftp
    if (strcmp(protocol, "ftp") == 0)
    {
        // created a dummy so that strtok behaves as intended.
        char *file_dummy = (char *)malloc(sizeof(char) * 256);
        // copy the filename part into the filedummy
        strcpy(file_dummy, protocol_file + 3);
        // null terminate.
        file_dummy[strlen(protocol_file)] = '\0';
        // apply strtok to deal with singular ftp inputs or ftp with just spaces.
        char *token;
        token = strtok(file_dummy, " \t\n\0");
        // if only space was entered, token will be nullptr of length 0 - exit.
        if (token == NULL)
        {
            printf("error: usage ftp\n");
            free(file_dummy);
            exit(0);
        }

        // if not only space/enter, then length of token > 0
        if (strlen(token) > 0)
        {
            // formally create the file_name.
            strcpy(file_name, token);
            // checks if the file_name exists or not.
            if (does_file_exist(file_name) == true)
            {
                if (sendto(srv_socket_fd, protocol, strlen(protocol), 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
                {
                    perror("sendto\n");
                    free(file_dummy);
                    freeaddrinfo(server_info);
                    close(srv_socket_fd);
                    exit(0);
                }
                // send protocol (ftp) to the server
                struct timeval start_conn;
                gettimeofday(&start_conn, NULL);
                // prepare to receive a message from the server after successfully sending to the server.
                char *text_buffer = (char *)malloc(sizeof(char) * 256);
                char *ack_receipt = (char *)malloc(sizeof(char) * 10);
                int numbytes;
                struct sockaddr_storage server_end;
                int server_end_length = sizeof(struct sockaddr_storage);
                if ((numbytes = recvfrom(srv_socket_fd, text_buffer, 256, 0, (struct sockaddr *)&server_end, &server_end_length)) == -1)
                {
                    free(file_dummy);
                    perror("recvfrom");
                    close(srv_socket_fd);
                    exit(0);
                }
                struct timeval end_conn;
                gettimeofday(&end_conn, NULL);
                time_t rtt_sec = end_conn.tv_sec - start_conn.tv_sec;
                suseconds_t rtt_usec = end_conn.tv_usec - start_conn.tv_usec;
                long rtt = (rtt_sec * 1000000) + rtt_usec;
                printf("Time taken to set up connection is : %ld micro seconds\n", rtt);
                char *message = "yes";
                // if received message was yes
                if (strcmp(text_buffer, message) == 0)
                {
                    printf("A file transfer can start.\n");

                    // Lab 2 starts here

                    FILE *fileOpen;
                    long offset = 1000; // value to offset file pointer.
                    char fileContent[1000];
                    struct Packet *packet;

                    fileOpen = fopen(file_name, "rb");
                    if (fileOpen == NULL)
                    {
                        printf("An error occurred while opening the file.\n");
                        exit(0);
                    }
                    
                    long currentPointer = 0; // will be used to offset the file pointer.

                    // check the size of the file
                    fseek(fileOpen, 0, SEEK_END);
                    long size = ftell(fileOpen);
                    // set the file pointer back to the start
                    fseek(fileOpen, 0, SEEK_SET);
                    
                    // finding the total number of fragments the file will be split into
                    int total_frag = size/1000 + 1;
                    if(size%1000==0){
                        total_frag--;
                    }
                    // we're constructing an array of struct Packet
                    packet = (struct Packet *)malloc(total_frag * sizeof(struct Packet));
                    int currentFrag = 1;
                    int i;
                    while(currentFrag<=total_frag){
                        i = currentFrag-1;
                        size_t items_read = fread(fileContent, 1, 1000, fileOpen);
                        // read the data to max 1000 bytes in the file.
                        // store the data into struct.
                        packet[i].total_frag = total_frag;
                        packet[i].frag_no = currentFrag;
                        if(currentFrag==total_frag){
                            packet[i].size = size-currentPointer;
                        }
                        else{
                            packet[i].size = 1000;
                        }
                        packet[i].filename = file_name;
                        memcpy(packet[i].filedata, fileContent, packet[i].size);

                        //offsetting by 1000 here to move to the next data.
                        fseek(fileOpen, offset, currentPointer);
                        currentPointer+=1000;
                        currentFrag++;
                    }
                    
                    // the total fragments and the file name is static so we can count that separately
                    int staticCount = count_digits(total_frag)+strlen(file_name);
                    int totalSize;
                    int frag_no;
                    int sizeCount;
                    // initialization for the formula used for timeout later on
                    struct timeval est_rtt;
                    struct timeval sample_rtt;
                    struct timeval dev_rtt;

                    est_rtt.tv_sec = 0;
                    sample_rtt.tv_sec = 0;
                    dev_rtt.tv_sec = 0;
                    est_rtt.tv_usec = 0;
                    sample_rtt.tv_usec = 0;
                    dev_rtt.tv_usec = 0;
                    
                    // initializiation of timeout to be 1 seconds (RFC-6928)
                    struct timeval timeout;
                    timeout.tv_sec = 1;
                    timeout.tv_usec = 0;
                    // https://linux.die.net/man/3/setsockopt - set the timeout to the timeout params defined above.
                    // socket receives a timeval struct of the default timeout value we have set above.
                    if(setsockopt(srv_socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout, sizeof(timeout)) < 0){
                        printf("Error: setsockopt\n");
                        exit(1);
                    }
                    // bool resent = false;

                    for (i = 0; i < total_frag; i++) {
                        if(resent_flags[i] == 1){
                            printf("I RESEND \n");
                        }
                        
                        // we need to calculate the total size of the packet message
                        frag_no = count_digits(packet[i].frag_no);
                        sizeCount = count_digits(packet[i].size);
                        totalSize = staticCount+frag_no+sizeCount+4+packet[i].size;
                        char packetMessage[totalSize];
                        // total_frag + frag_no + sizeCount + colons + data + \n + 1extra
                        sprintf(packetMessage, "%d:%d:%d:%s:", packet[i].total_frag, packet[i].frag_no, packet[i].size, packet[i].filename);
                        
                        // so we need to store data in packetMessage now.
                        // to do that we do piece by piece.

                        int index = staticCount+frag_no+sizeCount+4;
                        for(int j=0;j<packet[i].size; j++){
                            packetMessage[index]=packet[i].filedata[j];
                            index++;
                        }
                        // printf("INDEX :%d\n",index);
                        packetMessage[index] = '\0';

                        if (sendto(srv_socket_fd, packetMessage, totalSize, 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
                        {
                            perror("sendto\n");
                            freeaddrinfo(server_info);
                            close(srv_socket_fd);
                            exit(0);
                        }
                        // send the packet message here
                        struct timeval start;
                        gettimeofday(&start, NULL);
                        if ((numbytes = recvfrom(srv_socket_fd, ack_receipt, 10, 0, (struct sockaddr *)&server_end, &server_end_length)) == -1)
                        {
                            // perror("recvfrom: timeout in receiving ACK");
                            printf("PACKET DROP - continuing \n");
                            resent_flags[i] = 1;
                            i--;
                            continue;
                        }
                        struct timeval end;
                        gettimeofday(&end, NULL);
                        time_t rtt_sec = end.tv_sec - start.tv_sec;
                        suseconds_t rtt_usec = end.tv_usec - start.tv_usec;
                        long rtt = (rtt_sec * 1000000) + rtt_usec;
                        // printf("Time taken to receive ACK/NACK packet is : %ld micro seconds\n", rtt);
                        char receipt[4] = {'A', 'C', 'K', '\0'};
                        if(strcmp(ack_receipt, receipt)!=0){
                            perror("did not receive ACK - resending.\n");
                            resent_flags[i] = 1;
                            i--;
                            continue;
                        }
                        // printf("ACK RECEIVED\n");
                        if(resent_flags[i] == 0){
                            timeval_subtract(&sample_rtt, &end, &start);
                            long total_microseconds_sample =  (sample_rtt.tv_sec * 1000000) + sample_rtt.tv_usec;
                            long total_microseconds_est = (est_rtt.tv_sec * 1000000) + est_rtt.tv_usec;
                            if(i == 0){
                                total_microseconds_est = total_microseconds_sample;
                                est_rtt.tv_sec = (int)((total_microseconds_est)/1000000);
                                est_rtt.tv_usec = total_microseconds_est % 1000000; 

                            }
                            total_microseconds_est = total_microseconds_est * (1-ALPHA) + ALPHA * total_microseconds_sample;

                            est_rtt.tv_sec = (int)((total_microseconds_est)/1000000);
                            est_rtt.tv_usec = total_microseconds_est % 1000000;

                            long total_microseconds_dev = (dev_rtt.tv_sec * 1000000) + dev_rtt.tv_usec;
                            total_microseconds_est = (est_rtt.tv_sec * 1000000) + est_rtt.tv_usec;
                            total_microseconds_sample =  (sample_rtt.tv_sec * 1000000) + sample_rtt.tv_usec;
                            total_microseconds_dev = total_microseconds_dev * (1-BETA) + BETA * fabs(total_microseconds_sample - total_microseconds_est);

                            dev_rtt.tv_sec = (int)((total_microseconds_est)/1000000);
                            dev_rtt.tv_usec = total_microseconds_est % 1000000;



                            total_microseconds_dev = (dev_rtt.tv_sec * 1000000) + dev_rtt.tv_usec;
                            total_microseconds_est = (est_rtt.tv_sec * 1000000) + est_rtt.tv_usec;
                            long total_microseconds_timeout = total_microseconds_est + 4 * total_microseconds_dev;

                            timeout.tv_sec = (int)((total_microseconds_timeout)/1000000);
                            timeout.tv_usec = total_microseconds_timeout % 1000000;
                            
                            if(setsockopt(srv_socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout, sizeof(timeout)) < 0){
                                printf("Error: setsockopt\n");
                                exit(1);
                            }
                            // printf("Sample RTT: %ld microseconds\n", total_microseconds_sample);
                            // printf("Estimated RTT: %ld microseconds\n", total_microseconds_est);
                            printf("Timeout %d RTT: %ld microseconds\n", i, total_microseconds_timeout);
                            

                        }
                    }          

                    fclose(fileOpen);
                }
                // if received message was no
                else
                {
                    printf("File transfer not possible.\n");
                }
            }
            // if file DNE
            else
            {
                free(file_dummy);
                fprintf(stderr, "File not found. Exiting.\n");
                exit(0);
            }
        }
        // if only spaces exist or enter pressed after ftp.
        else
        {
            free(file_dummy);
            printf("error: usage ftp\n");
            exit(0);
        }
    }
    // if protocol was not ftp
    else
    {
        printf("ftp not used. exiting.\n");
        exit(0);
    }
    freeaddrinfo(server_info);
    close(srv_socket_fd);
    return 0;
}