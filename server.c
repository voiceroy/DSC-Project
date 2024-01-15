#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MSG_SIZE 513
#define SNDR_SIZE 19 // inet_ntoa() always returns a string of size 19

typedef struct {
    struct in_addr client_addr;
    int client_socket;
    size_t index;
} client_info;

typedef struct {
    char message[MSG_SIZE];
    char sender[SNDR_SIZE];
} msg;

typedef struct {
    size_t max_clients;
    size_t max_events;
} server_config;

void add_client(server_config config, client_info *clients, client_info client_struct) {
    for (int i = 0; i < config.max_clients; i++) {
        if (clients[i].client_socket == 0) {
            clients[i] = client_struct;
            clients[i].index = i;
            break;
        }
    }
}

void remove_client(client_info *clients, client_info client_struct) {
    memset(&clients[client_struct.index], 0, sizeof(client_info));
}

msg pack_msg(char *buffer, struct in_addr client_addr) {
    msg bdcast_msg;
    strcpy(bdcast_msg.message, buffer);
    strcpy(bdcast_msg.sender, inet_ntoa(client_addr));

    return bdcast_msg;
}

void disconnect_client(client_info *clients, size_t client_index, int client_fd, int epoll_fd) {
    printf("Client %s disconnected\n", inet_ntoa(clients[client_index].client_addr));
    remove_client(clients, clients[client_index]);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
}

void broadcast_message(server_config config, client_info *clients, msg bdcast_msg, int sender) {
    for (int i = 0; i < config.max_clients; i++) {
        if (clients[i].client_socket != 0 && clients[i].client_socket != sender) {
            send(clients[i].client_socket, &bdcast_msg, sizeof(msg), 0);
        }
    }
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket, port, status;
    struct sockaddr_in server, client; // IPv4 only
    socklen_t client_length = sizeof(struct sockaddr_in);
    server_config config = {10, 10};

    if (argc < 2) {
        fprintf(stderr, "Usage: server PORT [MAX_CLIENTS]\n");
        return EXIT_FAILURE;
    }
    port = atoi(argv[1]);

    if (argc == 3) {
        config.max_clients = atoi(argv[2]);
        if (config.max_clients <= 0) {
            config.max_clients = 10;
        }
        config.max_events = config.max_clients;
    }

    client_info *clients = calloc(config.max_clients, sizeof(client_info));
    if (clients == NULL) {
        fprintf(stderr, "Calloc error\n");
        return EXIT_FAILURE;
    }

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

    printf("Server listening on port: %d\nMax clients: %zu\n", ntohs(server.sin_port), config.max_clients);

    // Listen for connections
    status = listen(server_socket, config.max_clients);
    if (status == -1) {
        perror("Listen error");
        return EXIT_FAILURE;
    }

    // Create epoll instance
    int epoll_fd = epoll_create(config.max_events);
    if (epoll_fd == -1) {
        perror("Epoll error: ");
        return EXIT_FAILURE;
    }

    // Add server socket to epoll events
    struct epoll_event event = {.events = EPOLLIN, .data.fd = server_socket};
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
        perror("Epoll error: ");
        close(epoll_fd);
        return EXIT_FAILURE;
    }

    // Buffer to write the events to
    struct epoll_event *events = calloc(config.max_events, sizeof(struct epoll_event));
    if (events == NULL) {
        fprintf(stderr, "Calloc error\n");
        return EXIT_FAILURE;
    }

    while (true) {
        int num_events = epoll_wait(epoll_fd, events, config.max_events, -1);
        if (num_events == -1) {
            perror("Epoll error: ");
            break;
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_socket) {
                // Accept new connections
                client_socket = accept(server_socket, (struct sockaddr *)&client, &client_length);
                if (client_socket == -1) {
                    perror("Connect error");
                    continue;
                }
                printf("Client %s connected\n", inet_ntoa(client.sin_addr));

                // Add the new client socket to epoll events
                event.events = EPOLLIN;
                event.data.fd = client_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                    perror("Epoll error: ");
                    close(client_socket);
                    continue;
                }

                // Create client_info for the new client
                client_info client_struct = {client.sin_addr, client_socket};
                add_client(config, clients, client_struct);
            } else {
                // Handle data from existing clients
                int client_fd = events[i].data.fd;
                int client_index = -1;

                // Find the client index
                for (int j = 0; j < config.max_clients; j++) {
                    if (clients[j].client_socket == client_fd) {
                        client_index = j;
                        break;
                    }
                }

                // Client not found
                if (client_index == -1) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                    continue;
                }

                char buffer[MSG_SIZE];
                int recv_length = recv(client_fd, buffer, sizeof(buffer), 0);
                if (recv_length <= 0) { // Client disconnected
                    disconnect_client(clients, client_index, client_fd, epoll_fd);
                } else { // Received a message from the client, broadcast it
                    msg bdcast_msg = pack_msg(buffer, clients[client_index].client_addr); // pack bdcast_msg
                    broadcast_message(config, clients, bdcast_msg, client_fd);
                    bzero(buffer, sizeof(buffer)); // Clear out the client message
                }
            }
        }
    }

    close(server_socket);
    close(epoll_fd);
    return EXIT_SUCCESS;
}
