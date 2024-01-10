#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_BUF 512

int main(int argc, char *argv[]) {
    int server_socket, client_socket, port, status, max_connections;
    struct sockaddr_in server, client; // IPv4 only
    socklen_t client_length = sizeof(struct sockaddr_in);
    char *msg = "Hello World\n";

    if (argc < 3) {
        fprintf(stderr, "Usage: server PORT MAXCONNECTIONS\n");
        return EXIT_FAILURE;
    }

    // Process the required input
    port = atoi(argv[1]);
    max_connections = atoi(argv[2]);

    // Fill in the server struct
    memset(&server, 0, sizeof(struct sockaddr_in));
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

    printf("Server listening on port: %d\nMax connections: %d\n", ntohs(server.sin_port), max_connections);

    // Listen for connections
    status = listen(server_socket, max_connections);
    if (status == -1) {
        perror("Listen error");
        return EXIT_FAILURE;
    }

    client_socket = accept(server_socket, (struct sockaddr *)&client, &client_length);
    while (client_socket) {
        printf("Client %s connected\n", inet_ntoa(client.sin_addr));

        // Send data to client
        status = send(client_socket, msg, strlen(msg), 0);
        if (status == -1) {
            perror("Send error");
            continue;
        }

        close(client_socket);
        client_socket = accept(server_socket, (struct sockaddr *)&client, &client_length);
    }

    close(server_socket);
    return EXIT_SUCCESS;
}
