#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

// main goal: to make use of UDP and send between client and server.
// author: shikhar chaurasia
// student number: 1006710016

int main(int argc, char **argv)
{
    if (argc != 2)
        exit(0);
    int port_number = argv[1];
    int srv_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    int bind_value = bind(srv_socket_fd, )
}