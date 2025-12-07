#include "common.h"
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAX_CLIENTS 10

int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast(const char *msg, int sender_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0 && clients[i] != sender_fd) {
            send(clients[i], msg, strlen(msg), 0);
            send(clients[i], "\n", 1, 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *client_handler(void *arg) {
    int fd = *(int *)arg;
    free(arg);
    char buffer[2048];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int len = recv(fd, buffer, sizeof(buffer) - 1, 0);
        
        if (len <= 0) break;
        buffer[strcspn(buffer, "\n")] = 0;
        printf("[Log] Received from Client %d: %s\n", fd, buffer);
        broadcast(buffer, fd);
    }

    close(fd);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == fd) {
            clients[i] = 0;
            break;
        }
    }
    client_count--;
    pthread_mutex_unlock(&clients_mutex);
    
    printf("[-] Client %d disconnected. Total clients: %d\n", fd, client_count);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    signal(SIGPIPE, SIG_IGN);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TCP_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed! Port might be busy");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("==========================================\n");
    printf("   🚀 TCP Chat Server Running on Port %d\n", TCP_PORT);
    printf("==========================================\n");
    printf("Waiting for connections...\n\n");
    printf("To connect clients, open new terminals and run:\n");
    printf("  ./chat_app network 127.0.0.1 <Name>\n\n");
    printf("Logs:\n");

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENTS) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == 0) {
                    clients[i] = new_socket;
                    break;
                }
            }
            client_count++;
            
            printf("[+] New Client Connected! (Socket: %d) Total: %d\n", new_socket, client_count);
            
            int *arg = malloc(sizeof(int));
            *arg = new_socket;
            pthread_t tid;
            pthread_create(&tid, NULL, client_handler, arg);
            pthread_detach(tid);
        } else {
            printf("Refused connection: Server Full\n");
            close(new_socket);
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    return 0;
}