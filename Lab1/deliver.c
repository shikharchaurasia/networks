#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
    int port_number = atoi(argv[1]);
    int srv_socket_fd;
    struct addrinfo hints;
    struct addrinfo *server_info, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    
    if ((getaddrinfo(argv[1], argv[2], &hints, &server_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo\n");
        exit(2);
    }

    for (p = server_info; p != NULL; p = p->ai_next)
    {
        if ((srv_socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("socket");
            continue;
        }
        break;
    }
    char protocol[256] = {0}, file_name[256] = {0}, protocol_file[256] = {0};
    printf("Enter file to be transferred. Format: ftp <filename>\n");
    fgets(protocol_file, 256, stdin);
    // if user enters something like ftpl
    if(protocol_file[3] != ' '){
        // error
        printf("Errororororor\n");
        exit(9);
    }
    strncpy(protocol, protocol_file, 3);
    protocol[3] = '\0'; 
    printf("%s\n", protocol);
    if(strcmp(protocol, "ftp")==0){
        char *file_dummy = (char *)malloc(sizeof(char) * 256);
        strcpy(file_dummy, protocol_file+3);
        file_dummy[strlen(protocol_file)] = '\0';
        // printf("%s\n", file_dummy);
        // printf("%d\n", strlen(file_dummy));
        printf("Done copy hehe\n");
        char *token;
        token = strtok(file_dummy, " \t\n\0"); 
        if(token == NULL){
            exit(0);
        }
        printf("%s\n", token);
        printf("%d\n", strlen(token));  
        printf("Done copy hehe2\n");
              
        if(strlen(token) > 0){
            strcpy(file_name, token);
            printf("%d\n", strlen(file_name));
            if (does_file_exist(file_name) == true)
            {
                if (sendto(srv_socket_fd, protocol, strlen(protocol), 0, server_info->ai_addr, server_info->ai_addrlen) == -1)
                {
                    perror("sendto\n");
                    freeaddrinfo(server_info);
                    close(srv_socket_fd);
                    exit(0);
                }
                char *text_buffer = (char *)malloc(sizeof(char) * 256);
                int numbytes;
                struct sockaddr_storage server_end;
                int server_end_length = sizeof(struct sockaddr_storage);
                if ((numbytes = recvfrom(srv_socket_fd, text_buffer, 256, 0, (struct sockaddr *)&server_end, &server_end_length)) == -1)
                {
                    perror("recvfrom");
                    close(srv_socket_fd);
                    exit(1);
                }
                char *message = "yes";
                if (strcmp(text_buffer, message) == 0)
                {
                    printf("File transfer starts.\n");
                }
                else
                {
                    printf("No bueno.\n");
                }
            }
            // no
            else
            {
                fprintf(stderr, "File not found. Exiting.");
                exit(0);
            }

        }
        else{
            printf("error: usage ftp\n");
            exit(0);
        }

    }
    else{
        printf("EXITING>");
        exit(9);
    }
    freeaddrinfo(server_info);
    close(srv_socket_fd);
    return 0;
}