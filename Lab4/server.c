#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define LOGIN 1
#define LO_ACK 2
#define LO_NAK 3
#define EXIT 4
#define JOIN 5
#define JN_ACK 6
#define JN_NAK 7
#define LEAVE_SESS 8
#define NEW_SESS 9
#define NS_ACK 10
#define MESSAGE 11
#define QUERY 12
#define QU_ACK 13

#define MAX_NAME 25
#define MAX_DATA 1024

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};


struct user_info{
    char username[25];
    char password[20];
};



int user_status[5] = {0, 0, 0, 0, 0};

int sessions [5] = {0, 0, 0, 0, 0};

typedef struct messagepacket{
    char command_text; 
    char arguments[4][200];
    int count;
} MessagePacket;

// MessagePacket parse_received_message(char *rcv_message){
//     int count = 0;
//     int i = 0;
//     MessagePacket parsed_message = {0};
//     char *token = strtok(rcv_message, " ");
//     strcpy
//     count++;
//     while(token != NULL){

//     }
// }
// main goal: to make use of TCP and send between client and server.
/*
Done by:
Shikhar Chaurasia (Student # 1006710016)
and
Gunin Wasan (Student # 1007147749)
*/

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv)
{
    struct user_info users[5] = {
        {"gunin", "wasan"},
        {"shikhar", "chaurasia"},
        {"ali", "gardezi"},
        {"shaheryar", "arshad"},
        {"gandharv", "nagrani"}
    };
    fd_set master;      //master fd list
    fd_set read_fds;    //temp file descriptor list for select()
    int fdmax;      //max file descriptor number
    int newfd;      //newly accept()ed socket descriptor

    // int client_id = 0;
    // incorrect usage of the server
    if (argc != 2)
    {
        fprintf(stderr, "server usage: server <tcp port number>\n");
        exit(0);
    }
    int port_number = atoi(argv[1]);
    int yes = 1;
    int srv_socket_fd; //this is our listener fd
    struct addrinfo hints;
    struct addrinfo *server_info, *p;
    // hints: indicates the protocols and socket types required.
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP here
    hints.ai_flags = AI_PASSIVE; // use my IP
    // used for DNS lookup - returns ip address witin server_info using the port number.
    if ((getaddrinfo(NULL, argv[1], &hints, &server_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo\n");
        exit(2);
    }
    for (p = server_info; p != NULL; p = p->ai_next)
    {
        if ((srv_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("listener: socket");
            continue;
        }
        setsockopt(srv_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
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

    if (listen(srv_socket_fd, 10) == -1){
        perror("listen");
        close(srv_socket_fd);
        exit(1);
    }
    
    freeaddrinfo(server_info);
    FD_ZERO(&master);
    FD_SET(srv_socket_fd, &master);
    fdmax = srv_socket_fd;
    printf("Server running on port: %d\n", port_number);
    struct sockaddr_storage sender_info;
    socklen_t addrlen;
    char text_buffer[1024];
    int nbytes;
    while (1)
    {
        read_fds = master; // copy it


        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        int i, j;
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == srv_socket_fd) {
                    // handle new connections
                    addrlen = sizeof sender_info;
                    newfd = accept(srv_socket_fd, (struct sockaddr *)&sender_info, &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } 
                    
                    else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection\n");
                            // inet_ntop(sender_info.ss_family, get_in_addr((struct sockaddr*)&sender_info),remoteIP, INET6_ADDRSTRLEN), newfd);
                    }
                } 
                else {
                    // handle data from a client
                    if ((nbytes = recv(i, text_buffer, sizeof text_buffer, 0)) <= 0) {
                        // got error or connection closed by client
                        printf("%s\n", text_buffer);
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } 
                    else {
                        // we got some data from a client
                        for(j = 0; j <= fdmax; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                if (j != srv_socket_fd && j != i) {
                                    if (send(j, text_buffer, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                        printf("%s\n", text_buffer);

                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors

        // int deliver_socket_fd = accept(srv_socket_fd, NULL, NULL);
        // // sender's information - used in recv
        // // struct sockaddr_storage sender_info;
        // // int sender_info_size = sizeof(struct sockaddr_storage);
        // char *text_buffer = (char *)malloc(sizeof(char) * 1024);
        // int numbytes;

        // int receivedBytes = 0;
    
        // if (((numbytes = recv(deliver_socket_fd, text_buffer, 1024, 0)) == -1))
        // {
        //     perror("recv");
        //     close(srv_socket_fd);
        //     exit(1);
        // }        

        // char *msg = "recvACK";
        // client_id++;
        // char client_id_str[20];
        // sprintf(client_id_str, "%d", client_id);

        // // if (send(deliver_socket_fd, msg, strlen(msg), 0) == -1) {
        // //     perror("send");
        // //     exit(1);
        // // }
        // if (send(deliver_socket_fd, client_id_str, strlen(client_id_str), 0) == -1) {
        //     perror("send");
        //     exit(1);
        // }
        // printf("Client 1: %s\n", text_buffer);
        // free(text_buffer);
        // printf("Closing socket. \n");
        // close(deliver_socket_fd);
        // printf("Client Socket Disconnected.\n");
    }
    return 0;
}
