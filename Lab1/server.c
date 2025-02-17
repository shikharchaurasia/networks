#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
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

int main(int argc, char **argv)
{
    // incorrect usage of the server
    while (1)
    {
        if (argc != 2)
        {
            fprintf(stderr, "server usage: server <udp port number>\n");
            exit(0);
        }
        int port_number = atoi(argv[1]);
        int srv_socket_fd;
        struct addrinfo hints;
        struct addrinfo *server_info, *p;
        // hints: indicates the protocols and socket types required.
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP
        // used for DNS lookup - returns ip address witin server_info using the port number.
        if ((getaddrinfo(NULL, argv[1], &hints, &server_info)) != 0)
        {
            fprintf(stderr, "getaddrinfo\n");
            exit(2);
        }
        for (p = server_info; p != NULL; p = p->ai_next)
        {
            if ((srv_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
            {
                perror("listener: socket");
                continue;
            }
            if (bind(srv_socket_fd, p->ai_addr, p->ai_addrlen) == -1)
            {
                close(srv_socket_fd);
                perror("listener: bind");
                continue;
            }
            break;
        }
        if (p == NULL)
        {
            fprintf(stderr, "socket: error creating socket\n");
            exit(1);
        }
        freeaddrinfo(server_info);
        printf("Server running on port: %d\n", port_number);
        // sender's information - used in recvfrom
        struct sockaddr_storage sender_info;
        int sender_info_size = sizeof(struct sockaddr_storage);
        char *text_buffer = (char *)malloc(sizeof(char) * 256);
        int numbytes;
        if ((numbytes = recvfrom(srv_socket_fd, text_buffer, 256, 0, (struct sockaddr *)&sender_info, &sender_info_size)) == -1)
        {
            perror("recvfrom");
            close(srv_socket_fd);
            exit(1);
        }
        char ftp[4] = {'f', 't', 'p', '\0'};
        text_buffer[numbytes] = '\0';
        if (strcmp(text_buffer, ftp) == 0)
        {
            char *message = "yes";
            if ((numbytes = sendto(srv_socket_fd, message, strlen(message), 0, (struct sockaddr *)&sender_info, sender_info_size)) == -1)
            {
                perror("sendto");
                close(srv_socket_fd);
                exit(1);
            }
        }
        else
        {
            char *message = "no";
            if ((numbytes = sendto(srv_socket_fd, message, strlen(message), 0, (struct sockaddr *)&sender_info, sender_info_size)) == -1)
            {
                perror("sendto");
                close(srv_socket_fd);
                exit(1);
            }
        }
        close(srv_socket_fd);
        free(text_buffer);
    }
    return 0;
}
