#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int server_socket, port, status, max_connections;
    struct sockaddr_in server; // IPv4 only

    if (argc < 3) {
        fprintf(stderr, "Usage: server PORT MAXCONNECTIONS\n");
        return EXIT_FAILURE;
    }
    // Process the required input
    port = atoi(argv[1]);
    max_connections = atoi(argv[2]);

    // Fill in the server struct
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    // Get a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    // Bind the socket to the address
    status = bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr));
    if (status == -1) {
        perror("Bind error");
        return EXIT_FAILURE;
    }

    printf("Server listening on port: %d, with max connections: %d\n", ntohs(server.sin_port), max_connections);

    close(server_socket);
    return 0;
}
