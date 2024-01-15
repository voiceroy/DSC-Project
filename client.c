#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MSG_SIZE 513
#define SNDR_SIZE 19
#define MAX_EVENTS 2
#define QUIT_CMD "QUIT"

typedef struct {
    char message[MSG_SIZE];
    char sender[SNDR_SIZE];
} msg;

int receive_messages(int client_socket) {
    msg recvd_msg;
    int bytes_received = recv(client_socket, &recvd_msg, sizeof(msg), 0);
    if (bytes_received <= 0) {
        printf("Server disconnected\n");
        return -1;
    }

    printf("%s: %s\n", recvd_msg.sender, recvd_msg.message);
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

    // Watch for stdin and recv
    int epoll_fd = epoll_create(MAX_EVENTS);
    struct epoll_event events[MAX_EVENTS];

    // Register the socket for read events
    struct epoll_event recv_event = {.events = EPOLLIN, .data.fd = client_socket};
    status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &recv_event);
    if (status == -1) {
        perror("Epoll error: ");
        return EXIT_FAILURE;
    }

    // Register the stdin for input events
    struct epoll_event input_event = {.events = EPOLLIN, .data.fd = STDIN_FILENO};
    status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &input_event);
    if (status == -1) {
        perror("Epoll error: ");
        return EXIT_FAILURE;
    }

    int loop_status = 0;
    while (!loop_status) {
        printf("> ");
        fflush(stdout);

        int occured = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (status == -1) {
            perror("Epoll error:");
            return EXIT_FAILURE;
        }

        for (size_t i = 0; i < occured; i++) {
            // We got some message
            if (events[i].data.fd == client_socket && events[i].events & EPOLLIN) {
                status = receive_messages(client_socket);
                if (status == -1) {
                    loop_status = 1;
                    break;
                }
            }

            // We got some input from the user
            if (events[i].data.fd == STDIN_FILENO && events[i].events & EPOLLIN) {
                fgets(buffer, sizeof(buffer), stdin);
                buffer[strcspn(buffer, "\n")] = '\0';
                if (strncmp(buffer, QUIT_CMD, sizeof(QUIT_CMD)) == 0) {
                    loop_status = 1;
                    break;
                }

                // Don't send empty messages
                if (strlen(buffer) > 0) {
                    status = send_message(client_socket, buffer);
                }
                bzero(buffer, sizeof(buffer));
            }
        }
    }

    close(epoll_fd);
    close(client_socket);
    return EXIT_SUCCESS;
}
