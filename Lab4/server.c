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
#define NS_NAK 17
#define LG_ACK 18
#define REGISTER 19

#define MAX_NAME 25
#define MAX_DATA 1024
#define MAX_SESSION 10
#define USERINFO_FILE "userInfoDetails.txt"
struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};


struct user_info{
    char username[25];
    char password[20];
    int user_status;
    int session_id;
};

struct session{
    int sessionID;
    int sessionCount;
    char **list_of_users;
    struct session *next_session;
};

struct session* head_session = NULL;

int count_sessions = 0;

int userID = 0;
void getUserData(struct user_info *userDetails) {
    FILE *userFile = fopen(USERINFO_FILE, "r");
    if (userFile == NULL)
    {
        printf("Couldnt retrieve user details. Please try again.\n");
        return;
        // exit(0);
    }
    char getUserDetails[50];
    // we will iterate until we reach last line
    while (fgets(getUserDetails, 50, userFile) != NULL) {
        // separate through space and put them in userDetails struct.
        sscanf(getUserDetails, "%s %s", userDetails[userID].username, userDetails[userID].password);
        userDetails[userID].user_status = 0; //initially all logged out
        userID++;
    }
    fclose(userFile);    
}

void registerUserData(struct user_info *userDetails, char *userName, char* userPassword) {
    FILE *userFile = fopen(USERINFO_FILE, "a");
    if (userFile == NULL)
    {
        //If file doesnt open we dont register
        printf("Couldnt register user details. Please try again.\n");
        return;
        // exit(0);
    }
    // first we will append the data to the file.
    fprintf(userFile, "\n%s %s", userName, userPassword);
    // now we will add the data to user_info struct as well
    strcpy(userDetails[userID].username, userName);
    strcpy(userDetails[userID].password, userPassword);
    userDetails[userID].user_status = 0; //initially all logged out
    userID++;
    fclose(userFile);
}


// based on different TYPE, we do different executions/responses.
// client reps the file descriptor; message reps the command user has input
int parse_and_execute(struct user_info *users, int client, char *client_message){
    // parse the incoming client message into a packet.
    struct message client_packet;
    // break down the string into individual components.
    // char copy_of_client_message[MAX_DATA];
    char *copy_of_client_message = (char *)malloc(sizeof(char) * 1024);
    strcpy(copy_of_client_message, client_message);
    // memcpy(copy_of_client_message, client_message, strlen(client_message));
    char components[4][MAX_DATA];

	int i = 0;
	char *token = strtok(copy_of_client_message, ":");
	while(token != NULL && i < 4){
        memset(components[i], '\0', sizeof(components[i]));
		strncpy(components[i], token, MAX_DATA - 1);
		components[i][MAX_DATA-1] = '\0';
		token = strtok(NULL, ":");
		i++;
	}

    client_packet.type = atoi(components[0]);
    client_packet.size = atoi(components[1]);
    printf("cpd: %s\n", components[3]);
    memset(client_packet.source, '\0', sizeof(client_packet.source));
    memset(client_packet.data, '\0', sizeof(client_packet.data));

    strncpy((char *)client_packet.source, components[2], MAX_NAME);
    strncpy((char *)client_packet.data, components[3], MAX_DATA);

    // first extract message contents from the packet.

    if(client_packet.type == LOGIN){
        // data format: clientID:password:serverIP:server-port
        // first extract your data.

        char data[MAX_DATA];
		memcpy(data, client_packet.data, client_packet.size);
		char arguments[4][MAX_DATA];

		int i = 0;
		char *token = strtok(data, " ");
		while(token != NULL && i < 4){
			strncpy(arguments[i], token, MAX_DATA - 1);
			arguments[i][MAX_DATA-1] = '\0';
			token = strtok(NULL, " ");
			i++;
		}

        // we have our 4 arguments in our array of 4 arguments.
        // now we want to check if username exists or not. 
        int found_user = 0;
        for(i = 0; i < userID; i++){
            // if the ith user has the same username as the packet's username
            if(strcmp(users[i].username, arguments[0]) == 0){
                found_user = 1;
                // if not logged in.
                if(users[i].user_status == 0){
                    // if the password was correct.
                    if(strcmp(users[i].password, arguments[1]) == 0){ 
                        users[i].user_status = 1;
                        char sendMessage[1024];
                        char data[25] = "Login Successful.";
                        sprintf(sendMessage, "%d:%d:%s:%s", LO_ACK, (int)strlen(data), client_packet.source, data);
                        if(send(client, sendMessage, 1024, 0) == -1){
                            perror("send");
                            exit(1);
                        }
                        break;
                    }
                    // password was incorrect.
                    else{
                        char sendMessage[1024];
                        char data[25] = "Incorrect Password.";
                        sprintf(sendMessage, "%d:%d:%s:%s", LO_NAK, (int)strlen(data), client_packet.source, data);
                        if(send(client, sendMessage, 1024, 0) == -1){
                            perror("send");
                            exit(1);
                        }
                        break;
                    }
                }
                else{
                    // user is logged in to begin with - send LO_NAK
                    char sendMessage[1024];
                    char data[25] = "Already Logged In.";
                    sprintf(sendMessage, "%d:%d:%s:%s", LO_NAK, (int)strlen(data), client_packet.source, data);
                    if(send(client, sendMessage, 1024, 0) == -1){
                        perror("send");
                        exit(1);
                    }
                    break;
                }
            }
        }
        // user not found.
        if(found_user == 0){
            char sendMessage[1024];
            char data[25] = "Incorrect Username.";
            sprintf(sendMessage, "%d:%d:%s:%s", LO_NAK, (int)strlen(data), client_packet.source, data);
            if(send(client, sendMessage, 1024, 0) == -1){
                perror("send");
                exit(1);
            }
        }
    }
    else if(client_packet.type == QUERY){
        // message argument format: none
        // list all active sessions and users.
        // first the sessions.
        int i = 0;
        char active_sessions[300] = "Active Sessions: ";
        char active_users[300] = "Active Users: ";
        if(count_sessions != 0){
            // make sure ack message has 0 in sessions.
            struct session *sptr = head_session;
            while(sptr != NULL && i <= 4){
                char sess[5];
                sprintf(sess, "%d",sptr->sessionID);
                strcat(active_sessions, sess);
                strcat(active_sessions, " ");
                sptr = sptr->next_session;
                i++;
            }
        }
        else{
            char msg[10] = "None.";
            strcat(active_sessions, msg);
        }
        int found = 0;
        for(i = 0; i < userID; i++){
            if(users[i].user_status == 1){
                found = 1;
                strcat(active_users, users[i].username);
                strcat(active_users, " ");
            }
        }
        if(found == 0){
            char msg[10] = "None.";
            strcat(active_users, msg);
        }

        char data[700];
        strcat(data, active_users);
        strcat(data, "\n");
        strcat(data, active_sessions);
        strcat(data, "\n");
        char sendMessage[1024];
        sprintf(sendMessage, "%d:%d:%s:%s", QU_ACK, (int)strlen(data), client_packet.source, data);
        if(send(client, sendMessage, 1024, 0) == -1){
            perror("send");
            exit(1);
        }

    }
    else if(client_packet.type == LOGOUT){
        // message argument format: none
        int found = 0;
        for(i = 0; i < userID; i++){
            if(strcmp(users[i].username, (const char *)client_packet.source) == 0){
                // only if user is logged in, log them out.
                if(users[i].user_status == 1){
                    found = 1;
                    users[i].user_status = 0;
                }
                break;
            }
        }
        if(found == 1){
            // LG_ACK
            char sendMessage[1024];
            char data[50] = "Logged Out.\n";
            sprintf(sendMessage, "%d:%d:%s:%s", LG_ACK, (int)strlen(data), client_packet.source, data);
            if(send(client, sendMessage, 1024, 0) == -1){
                perror("send");
                exit(1);
            }
        }
        
    }
    
    else if(client_packet.type == JOIN){
        // message argument format: sessionID
        // first, check if 
        
    }
    else if(client_packet.type == NEW_SESS){
        // message argument format: sessionID
        // check if sessionID already exists or not.
        printf("%s\n", client_packet.data);
        int session_id = atoi((const char *)client_packet.data); 
        // printf("SESSION ID IS %d\n", session_id);
        struct session *sptr = head_session;
        int flag = 0;
        if(head_session != NULL){
            while(sptr != NULL){
                if(sptr->sessionID == session_id){
                    flag = 1;
                    break;
                }
                sptr = sptr->next_session;
            }
        }
        printf("F%d\n", flag);
        // if sessionID already exists, send a NS_NAK.
        if(flag == 1){
            // send NS_NAK
            char sendMessage[1024];
            char data[25] = "Session Already Exists.";
            sprintf(sendMessage, "%d:%d:%s:%s", NS_NAK, (int)strlen(data), client_packet.source, data);
            if(send(client, sendMessage, 1024, 0) == -1){
                perror("send");
                exit(1);
            }
            return 0;
        }
        // increment number of sessions.
        count_sessions = count_sessions + 1;
        // sessionID DNE - create one.
        struct session* new_session = (struct session *)malloc(sizeof(struct session));
        printf("HHEHEHE\n");
        // assign the sessionID passed as the argument.
        new_session->sessionID = session_id;
        // list_of_users is an array of char strings. - maximum 10 allowed per session.
        new_session->list_of_users = (char **)malloc(MAX_SESSION * sizeof(char *));

        for(i = 0; i < MAX_SESSION; i++){
            new_session->list_of_users[i] = (char *)malloc(sizeof(char) * MAX_NAME);
        }
        printf("HERE!!!\n");
        // assign the first user as the one creating the session.
        new_session->sessionCount = 1;
        strncpy(new_session->list_of_users[0], (const char*)client_packet.source, MAX_NAME);
        printf("HERE!!!!!!!!\n");
        // now we need to insert the session into a session queue.
        new_session->next_session = NULL;

        if(head_session == NULL){
            head_session = new_session;
        }
        else{
            sptr = head_session;
            while(sptr->next_session != NULL){
                sptr = sptr->next_session;
            }
            sptr->next_session = new_session;
        }
        // send ACK for the session just created.
        char sendMessage[1024];
        char data[50] = "New Session Created\n";
        sprintf(sendMessage, "%d:%d:%s:%s", NEW_SESS, (int)strlen(data), client_packet.source, data);
        if(send(client, sendMessage, 1024, 0) == -1){
            perror("send");
            exit(1);
        }
        
    }
    
    else if(client_packet.type == LEAVE_SESS){
        // message argument format: none
        // check which session you are a part of.
        
    }
    else{
        return MESSAGE;
    }
    return 0;

}

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
    struct user_info users[1000];
    getUserData(users);
    
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
                        int checkCommand = parse_and_execute(users, i, text_buffer);
                        if(checkCommand==MESSAGE){
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
                        else{
                            printf("COMMAND: ");
                            printf("%s\n", text_buffer);
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    }
    return 0;
}
