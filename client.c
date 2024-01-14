#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MSG_SIZE 513
#define SNDR_SIZE 19

char QUIT_CMD[] = "QUIT";

typedef struct {
    char message[MSG_SIZE];
    char sender[SNDR_SIZE];
} MSG;

int receive_messages(int client_socket) {
    MSG recvd_MSG;
    int bytes_received;

    bytes_received = recv(client_socket, &recvd_MSG, sizeof(MSG), 0);
    if (bytes_received <= 0) {
        printf("Server disconnected\n");
        return -1;
    }

    printf("%s: %s\n", recvd_MSG.sender, recvd_MSG.message);

    return 0;
}

int send_message(int client_socket, char message[MSG_SIZE]) {
    if (send(client_socket, message, strlen(message), 0) == -1) {
        perror("Message sending failed");
    }

    return 0;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    int client_socket, status;
    char buffer[MSG_SIZE];

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
    printf("Connected\n");
    printf("Use %s to quit\n", QUIT_CMD);

    struct pollfd pfds[] = {{.fd = client_socket, .events = POLLIN}, {.fd = STDIN_FILENO, .events = POLLIN}};
    while (true) {
        printf("> ");
        fflush(stdout);

        int event_count = poll(pfds, sizeof(pfds) / sizeof(*pfds), -1);
        if (event_count < 0) {
            perror("Poll error");
            return EXIT_FAILURE;
        }

        if (pfds[0].revents & POLLIN) {
            status = receive_messages(client_socket);
            if (status == -1) {
                break;
            }
        }

        if (pfds[1].revents & POLLIN) {
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0';
            if (strncmp(buffer, QUIT_CMD, sizeof(QUIT_CMD)) == 0) {
                break;
            }

            status = send_message(client_socket, buffer);
            bzero(buffer, sizeof(buffer));
            if (status == -1) {
                break;
            }
        }
    }

    close(client_socket);
    return EXIT_SUCCESS;
}
