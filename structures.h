#include <stddef.h>
#include <arpa/inet.h>

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
    size_t max_msg_hist;
} server_config;
