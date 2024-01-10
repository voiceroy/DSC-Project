#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_BUF 512

int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    int client_socket, status;
    char buffer[MAX_BUF + 1];

    if (argc < 3) {
        fprintf(stderr, "Usage: client IPADDR PORT\n");
        return EXIT_FAILURE;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    // Fill in the server struct
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &server.sin_addr);

    // Try to connect to the server
    printf("Connecting to server: %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    status = connect(client_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
    if (status == -1) {
        perror("Connection error");
        return EXIT_FAILURE;
    }

    // Recieve response from the server
    status = recv(client_socket, buffer, MAX_BUF, 0);
    buffer[status] = '\0';

    // Print the response
    printf("Recieved from server: %s", buffer);

    close(client_socket);
    return EXIT_SUCCESS;
}
