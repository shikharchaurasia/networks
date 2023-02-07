#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <netdb.h>
#include <unistd.h>

// main goal: to make use of UDP and send between client and server.
/*
Done by:
Shikhar Chaurasia (Student # 1006710016)
and
Gunin Wasan (Student # 1007147749)
*/
int set_cursor_filedata(const char *str, int n)
{
    int cursor = 0;
    int count = 0;
    while(1){
        if(str[cursor] == ':'){
            count++;
        }
        cursor++;
        if(count == n){
            return cursor;
        }
    }
    return cursor;
}
struct Packet
{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char *filename;
    char filedata[1000];
};
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
        free(text_buffer);
        char packet_receive_buffer[1100] = {0}, dummy_receive_buffer[1100] = {0};
        struct Packet packet;
        bool finished_receive = false;
        FILE *created_fp = NULL;
        while(finished_receive == false){

            memset(packet.filedata, '\0', 1000);
            memset(packet_receive_buffer, '\0', 1100);
            if ((numbytes = recvfrom(srv_socket_fd, packet_receive_buffer, 1100, 0, (struct sockaddr *)&sender_info, &sender_info_size)) == -1)
            {   
                perror("recvfrom packets\n");
                close(srv_socket_fd);
                exit(1);
            }
            // received the string
            // convert the received string to packet.
            // split into: 1) total frag 2) current frag 3) packet size
            // copy the message in itself
            
            memcpy(dummy_receive_buffer, packet_receive_buffer, sizeof(packet_receive_buffer));
            char *token;
            // made use of dummy receive buffer because strtok modifies original buffer
            token = strtok(dummy_receive_buffer, ":");
            // printf("%s\n", token)
            packet.total_frag = atoi(token);
            // printf("EEEEE%lu\n", packet.total_frag);
            token = strtok(NULL, ":");
            packet.frag_no = atoi(token);
            token = strtok(NULL, ":");
            packet.size = atoi(token); 
            
            // if we are dealing with first packet.
            // if(packet.frag_no == 1){
            token = strtok(NULL, ":");
            packet.filename = (char *)malloc(sizeof(char) * strlen(token));
            memcpy(packet.filename, token, strlen(token));
                // strcpy(packet.filename, token);
            // }
            printf("packet total_frag: %d\n", packet.total_frag);
            printf("packet curr frag: %d\n", packet.frag_no);
            printf("packet curr size: %d\n", packet.size);
            printf("packet file name: %s\n", packet.filename);
            int result = set_cursor_filedata(packet_receive_buffer, 4);
            // int counter =0;
            // for(int u =0; u<sizeof(packet_receive_buffer);u++){
            //     if(packet_receive_buffer[u]==':'&&counter<4){
            //         counter++;
            //     }
            //     else if (counter>=4){
            //         for(int k = 0; k<packet.size;k++){
            //             //printf("HI I AM DYING HERE \n");
            //             packet.filedata[k]=packet_receive_buffer[u+k]; 
            //         }
            //         break;
            //     }

            // }
            int i = result, k = 0;
            memcpy(packet.filedata, packet_receive_buffer+result, packet.size);
            if(created_fp == NULL){
                created_fp = fopen(packet.filename, "ab");      
            }
            int rv = fwrite(packet.filedata, sizeof(char), packet.size, created_fp);
            
            // }
            // printf("packet file name: %s\n", packet.filename);
            printf("%s\n", packet.filedata);
            free(packet.filename);
            memset(packet.filedata, '\0', 1000);
            if(packet.frag_no == packet.total_frag){
                finished_receive = true;
            }
            char receipt[4] = {'A', 'C', 'K', '\0'};
            if ((numbytes = sendto(srv_socket_fd, receipt, strlen(receipt), 0, (struct sockaddr *)&sender_info, sender_info_size)) == -1)
            {
                perror("sendto");
                close(srv_socket_fd);
                exit(1);
            }


        }
        printf("I HAVE EXITED LOL \n");
        // free(packet.filename);
        fclose(created_fp);
        close(srv_socket_fd);
    }
    return 0;
}
