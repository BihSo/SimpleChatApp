#include "common.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

static int sock_fd = -1;
static char my_name[32];
static int running = 1;

int tcp_connect(const char *hostname, int port) {
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) return -1;

    server = gethostbyname(hostname);
    if (server == NULL) return -1;

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return -1;

    return 0;
}

void send_message_tcp(const char *text) {
    if (sock_fd != -1) {
        char msg_with_name[MAX_MSG_LEN + 32];
        snprintf(msg_with_name, sizeof(msg_with_name), "%s: %s\n", my_name, text);
        send(sock_fd, msg_with_name, strlen(msg_with_name), 0);
        gui_append_message("Me", text);
    }
}

void *receive_thread_tcp(void *arg) {
    char buffer[2048];
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int len = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            if (running) gui_set_status("● Disconnected", FALSE);
            break;
        }
        
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) == 0) continue;
        
        char *sep = strstr(buffer, ": ");
        if (sep) {
            *sep = 0;
            char *message_text = sep + 2;
            if (strlen(message_text) > 0) {
                gui_append_message(buffer, message_text);
            }
        }
    }
    return NULL;
}

int run_tcp_client(const char *ip, const char *name) {
    strncpy(my_name, name, 31);
    char title[64];
    snprintf(title, sizeof(title), "Network Chat - %s", my_name);

    if (tcp_connect(ip, TCP_PORT) != 0) {
        printf("Connection Failed. Make sure server is running.\n");
        return 1;
    }

    running = 1;
    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_thread_tcp, NULL);

    init_gui(0, NULL, send_message_tcp, title);
    run_gui();

    running = 0;
    if (sock_fd != -1) {
        close(sock_fd);
        sock_fd = -1;
    }
    pthread_cancel(recv_tid);
    pthread_join(recv_tid, NULL);
    
    return 0;
}