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
#include <pthread.h>


// main goal: to make use of TCP and send texts between client and server.
/*
Done by:
Shikhar Chaurasia (Student # 1006710016)
and
Gunin Wasan (Student # 1007147749)
*/

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
#define LOGOUT 14
#define CREATE_SESS 15
#define LIST 16
#define REGISTER 17

#define MAX_NAME 25
#define MAX_DATA 1024

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};


char *userName; // global set for user name
// done so that we can set userName in send and confirm in receive.
int commandControlName(char* command);
int commandControlArgs(char* command);
void* sendThread(void* sendSocket); // thread for sending a message
void* rcvThread(void* rcvSocket); // thread for receiving a message

int main(int argc, char **argv)
{

    // incorrect usage of the client
    if (argc != 3)
    {
        fprintf(stderr, "client usage: client <server address> <tcp port num>\n");
        exit(0);
    }
    // initialization of variables
    // int port_number = atoi(argv[1]);
    int srv_socket_fd;
    struct addrinfo hints;
    struct addrinfo *server_info, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_STREAM; // for (TCP)
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
        if (connect(srv_socket_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(srv_socket_fd);
            perror("connect");
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

    pthread_t receiving; //  receiving thread
    pthread_t sending; //  seding thread
    
    userName = (char *)malloc(sizeof(char) * 1024);
    //  Now we will create a receiving thread, 
    // here rcvThread is the function that handles receiving messages parallely
    if (pthread_create(&receiving, NULL, rcvThread, (void*) &srv_socket_fd) != 0) {
        // error in receiving thread
        perror("receiving thread");
        exit(1);
    }

    //  Then we will create a sending thread
    // here sendThread is the function that handles sending messages parallely
    if (pthread_create(&sending, NULL, sendThread, (void*) &srv_socket_fd) != 0) {
        // error in sending thread
        perror("sending thread");
        exit(1);
    }
    
    pthread_join(receiving, NULL);
    pthread_join(sending, NULL);
    close(srv_socket_fd);
    freeaddrinfo(server_info);
    free(userName);
    return 0;
}

void* sendThread(void* sendSocket) {
    int srv_socket_fd = *((int*)sendSocket);
    char *message = (char *)malloc(sizeof(char) * 1024);

    while (1) {
        // printf("Enter text message: ");
        fgets(message, 1024, stdin);
        char *message2 = (char *)malloc(sizeof(char) * 1024);
        strcpy(message2, message);
        char *sepSpace = strtok(message, " ");  
        char **sepWords = (char **)malloc(sizeof(char *) * 1024);  
        int numWords = 0;
        int allowToSend = 0;

        while (sepSpace != NULL) {
            sepWords[numWords] = (char *)malloc(sizeof(char) * (strlen(sepSpace) + 1));  
            strcpy(sepWords[numWords], sepSpace); 
            numWords++;
            sepSpace = strtok(NULL, " "); // next word here
        }
        int getType, getSize;
        char* source = userName;
        char* data = (char *)malloc(sizeof(char) * 1024);
        if(numWords>0){
            if(sepWords[0][0]=='/'){
                // so now we check if the command is valid or not.
                getType=commandControlName(sepWords[0]);
                if(getType !=-1){
                    if(commandControlArgs(sepWords[0]) == (numWords-1)){
                        // all good we can go ahead.
                        char *dataMessage = (char *)malloc(sizeof(char) * 1024);
                        if(getType==LOGIN || getType==REGISTER){
                            strcpy(userName, sepWords[1]);
                        }
                        for(int i=1; i<numWords; i++){
                            strcat(dataMessage, sepWords[i]);
                            if (i < numWords - 1) {
                                strcat(dataMessage, " ");
                            }
                        }
                        strcpy(data, dataMessage);
                        allowToSend = 1;
                        free(dataMessage);
                        getSize = strlen(data);
                    }
                    else{
                        printf("Please enter correct number of arguments. \n");
                    }
                }
                else{
                    printf("Please enter a valid command. \n");
                }
            }
            else{
                // not a command. just a text message
                getType = MESSAGE;
                strcpy(data, message2);
                allowToSend = 1;
                getSize = strlen(data);
            }
        }
        if(allowToSend==1 && strcmp(userName, "")==0){
            allowToSend=0;
            printf("\nPlease use the /login command to Login first.\n");
        }
        
        if (allowToSend != 0)
        {
            char* sendMessage = (char *)malloc(sizeof(char) * 1024);
            sprintf(sendMessage, "%d:%d:%s:%s", getType, getSize, source, data);
            if (send(srv_socket_fd, sendMessage, 1024, 0) == -1)
            {
                perror("send");
                exit(1);
            }
            free(sendMessage);
            free(message2);
        }
        free(data);
    }

    free(message);    
    pthread_exit(NULL); // indicates thread end
}

void* rcvThread(void* rcvSocket) {
    int srv_socket_fd = *((int*)rcvSocket);
    char text_buffer[1024];

    while (1) {
        int rv = recv(srv_socket_fd, text_buffer, sizeof(text_buffer), 0);
        if (rv == -1) {
            perror("recv");
            exit(1);
        }
        else if(rv != 0){
            printf("\n%s\n",text_buffer);
        }
        else{
            printf("Server ended: Thank you for using the Text Conferencing Channel!\n");
            exit(0);
        }
    }
    pthread_exit(NULL); // indicates thread end
}

int commandControlName(char* command){
    command[strlen(command)] = '\0';

    if(strcmp(command, "/login") == 0 || strcmp(command, "/login\n") == 0){
        return LOGIN;
    }
    if(strcmp(command, "/logout") == 0 || strcmp(command, "/logout\n") == 0){
        return LOGOUT;
    }
    if(strcmp(command, "/joinsession") == 0 || strcmp(command, "/joinsession\n") == 0){
        return JOIN;
    }
    if(strcmp(command, "/leavesession") == 0 || strcmp(command, "/leavesession\n") == 0){
        return LEAVE_SESS;
    }
    if(strcmp(command, "/createsession") == 0 || strcmp(command, "/createsession\n") == 0){
        return CREATE_SESS;
    }
    if(strcmp(command, "/list") == 0 || strcmp(command, "/list\n") == 0){
        return QUERY;
    }
    if(strcmp(command, "/quit") == 0 || strcmp(command, "/quit\n") == 0){
        return EXIT;
    }
    if(strcmp(command, "/message") == 0 || strcmp(command, "/message\n") == 0){
        return MESSAGE;
    }
    if(strcmp(command, "/register") == 0 || strcmp(command, "/register\n") == 0){
        return REGISTER;
    }
    return -1;
}

int commandControlArgs(char* command){
    command[strlen(command)] = '\0';

    if(strcmp(command, "/login") == 0){
        return 4;
    }
    if(strcmp(command, "/logout\n") == 0){
        return 0;
    }
    if(strcmp(command, "/joinsession") == 0){
        return 1;
    }
    if(strcmp(command, "/leavesession\n") == 0){
        return 0;
    }
    if(strcmp(command, "/createsession") == 0){
        return 1;
    }
    if(strcmp(command, "/list\n") == 0){
        return 0;
    }
    if(strcmp(command, "/quit\n") == 0){
        printf("\nQuit Command Executed: Thank you for using the Text Conferencing Channel!\n");
        exit(0);
        return 0;
    }
    if(strcmp(command, "/message\n") == 0){
        return 0;
    }
    if(strcmp(command, "/register") == 0){
        return 2;
    }
    return -1;
}