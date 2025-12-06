#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Constants
#define MAX_MSG_LEN 256
#define SHM_NAME "/chat_shm"
#define SEM_MUTEX_NAME "/chat_sem_mutex"
#define SEM_FULL_NAME "/chat_sem_full"
#define SEM_EMPTY_NAME "/chat_sem_empty"
#define TCP_PORT 8080
#define WS_BUFFER_SIZE 1024

// Message Structure for Shared Memory
typedef struct {
    char sender[32];
    char text[MAX_MSG_LEN];
    int is_new; // Simple flag to indicate a new message
} ChatMessage;

// Function pointer type for sending messages (to be implemented by specific backends)
typedef void (*send_func_t)(const char *message);

#endif // COMMON_H
