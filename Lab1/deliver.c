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

// main goal: to make use of UDP and send between client and server.
/*
Done by:
Shikhar Chaurasia (Student # 1006710016)
and
Gunin Wasan (Student # 1007147749)
*/
// struct packet
// {
//     unsigned int total_frag;
//     unsigned int frag_no;
//     unsigned int size;
//     char *filename;
//     char filedata[1000];
// }

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
        // printf("%s\n", file_dummy);
        // printf("%d\n", strlen(file_dummy));
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
        // printf("%s\n", token);
        // printf("%d\n", strlen(token));
        // if not only space/enter, then length of token > 0
        if (strlen(token) > 0)
        {
            // formally create the file_name.
            strcpy(file_name, token);
            // checks if the file_name exists or not.
            if (does_file_exist(file_name) == true)
            {
                // send protocol (ftp) to the server
                struct timeval start;
                gettimeofday(&start, NULL);
                
                if (sendto(srv_socket_fd, protocol, strlen(protocol), 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
                {
                    perror("sendto\n");
                    free(file_dummy);
                    freeaddrinfo(server_info);
                    close(srv_socket_fd);
                    exit(0);
                }
                // prepare to receive a message from the server after successfully sending to the server.
                char *text_buffer = (char *)malloc(sizeof(char) * 256);
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
                struct timeval end;
                gettimeofday(&end, NULL);
                struct timeval result;
                printf("Time taken is : %ld micro seconds\n",
                    ((end.tv_sec * 1000000 + end.tv_usec) -
                    (start.tv_sec * 1000000 + start.tv_usec)));

                char *message = "yes";
                // if received message was yes
                if (strcmp(text_buffer, message) == 0)
                {
                    printf("A file transfer can start.\n");
                    // start Lab 2 from here.
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