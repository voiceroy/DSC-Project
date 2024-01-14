#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// 1 extra for null terminator
#define MSG_SIZE 513
#define SNDR_SIZE 19

#define MAX_CLIENTS 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    struct in_addr client_addr;
    int client_socket;
} client_info;

typedef struct {
    char message[MSG_SIZE];
    char sender[SNDR_SIZE];
} MSG;

client_info clients[MAX_CLIENTS];

void add_client(client_info client_struct) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_socket == 0) {
            clients[i] = client_struct;
            break;
        }
    }

    pthread_mutex_unlock(&mutex);
}

void remove_client(client_info client_struct) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_socket == client_struct.client_socket) {
            memset((void *)&clients[i], 0, sizeof(client_info));
            break;
        }
    }

    pthread_mutex_unlock(&mutex);
}

void broadcast_message(MSG bdcast_msg, int sender) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_socket != 0 && clients[i].client_socket != sender) {
            send(clients[i].client_socket, &bdcast_msg, sizeof(MSG), 0);
        }
    }
}

void *handle_client(void *arg) {
    client_info client_struct = *(client_info *)arg;

    add_client(client_struct);
    while (true) {
        char buffer[MSG_SIZE];
        int recv_length = recv(client_struct.client_socket, buffer, sizeof(buffer), 0);
        if (recv_length <= 0) {
            remove_client(client_struct);
            printf("Client %s disconnected\n", inet_ntoa(client_struct.client_addr));
            close(client_struct.client_socket);
            break;
        }

        // pack bdcast_msg
        MSG bdcast_msg;
        strcpy(bdcast_msg.message, buffer);
        strcpy(bdcast_msg.sender, inet_ntoa(client_struct.client_addr));

        pthread_mutex_lock(&mutex);
        broadcast_message(bdcast_msg, client_struct.client_socket);
        pthread_mutex_unlock(&mutex);
    }


    return NULL;
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket, port, status;
    struct sockaddr_in server, client; // IPv4 only
    socklen_t client_length = sizeof(struct sockaddr_in);

    if (argc < 2) {
        fprintf(stderr, "Usage: server PORT\n");
        return EXIT_FAILURE;
    }
    port = atoi(argv[1]);

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

    printf("Server listening on port: %d\nMax connections: %d\n", ntohs(server.sin_port), MAX_CLIENTS);

    // Listen for connections
    status = listen(server_socket, MAX_CLIENTS);
    if (status == -1) {
        perror("Listen error");
        return EXIT_FAILURE;
    }

    while (true) {
        client_socket = accept(server_socket, (struct sockaddr *)&client, &client_length);
        if (client_socket == -1) {
            perror("Connect error");
            continue;
        }
        printf("Client %s connected\n", inet_ntoa(client.sin_addr));

        // Create a thread for each client
        pthread_t client_tid;
        client_info client_struct = {client.sin_addr, client_socket};
        pthread_create(&client_tid, NULL, handle_client, &client_struct);
    }

    close(server_socket);
    return EXIT_SUCCESS;
}
