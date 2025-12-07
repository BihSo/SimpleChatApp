#include "common.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

typedef struct {
    ChatMessage c1_to_c2;
    ChatMessage c2_to_c1;
    int client1_active;
    int client2_active;
} SharedState2;

static SharedState2 *shm_ptr2;
static sem_t *sem_send_data, *sem_send_space;
static sem_t *sem_recv_data, *sem_recv_space;
static int client_id;
static char my_name[32];
static int running = 1;

void send_message_v2(const char *text) {
    if (strlen(text) == 0) return;

    sem_wait(sem_send_space);
    
    if (client_id == 1) {
        strncpy(shm_ptr2->c1_to_c2.sender, my_name, 31);
        strncpy(shm_ptr2->c1_to_c2.text, text, MAX_MSG_LEN - 1);
    } else {
        strncpy(shm_ptr2->c2_to_c1.sender, my_name, 31);
        strncpy(shm_ptr2->c2_to_c1.text, text, MAX_MSG_LEN - 1);
    }

    sem_post(sem_send_data);
    gui_append_message("Me", text);
}

void *receive_thread_func_v2(void *arg) {
    while (running) {
        if (sem_wait(sem_recv_data) != 0) break;

        ChatMessage *msg;
        if (client_id == 1) msg = &shm_ptr2->c2_to_c1;
        else msg = &shm_ptr2->c1_to_c2;
        
        gui_append_message(msg->sender, msg->text);
        sem_post(sem_recv_space);
    }
    return NULL;
}

int run_shm_client(int id, const char *name) {
    client_id = id;
    strncpy(my_name, name, 31);
    
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) return 1;

    ftruncate(fd, sizeof(SharedState2));

    shm_ptr2 = mmap(NULL, sizeof(SharedState2), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr2 == MAP_FAILED) return 1;

    if (client_id == 1) {
        if (shm_ptr2->client1_active) {
            printf("Error: Client ID 1 is already active.\n");
            munmap(shm_ptr2, sizeof(SharedState2));
            return 1;
        }
        shm_ptr2->client1_active = 1;

        sem_send_data = sem_open("/sem_c1_to_c2_data", O_CREAT, 0666, 0);
        sem_send_space = sem_open("/sem_c1_to_c2_space", O_CREAT, 0666, 1);
        sem_recv_data = sem_open("/sem_c2_to_c1_data", O_CREAT, 0666, 0);
        sem_recv_space = sem_open("/sem_c2_to_c1_space", O_CREAT, 0666, 1);
    } else {
        if (shm_ptr2->client2_active) {
            printf("Error: Client ID 2 is already active.\n");
            munmap(shm_ptr2, sizeof(SharedState2));
            return 1;
        }
        shm_ptr2->client2_active = 1;

        sem_send_data = sem_open("/sem_c2_to_c1_data", O_CREAT, 0666, 0);
        sem_send_space = sem_open("/sem_c2_to_c1_space", O_CREAT, 0666, 1);
        sem_recv_data = sem_open("/sem_c1_to_c2_data", O_CREAT, 0666, 0);
        sem_recv_space = sem_open("/sem_c1_to_c2_space", O_CREAT, 0666, 1);
    }

    running = 1;
    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_thread_func_v2, NULL);

    char title[64];
    snprintf(title, sizeof(title), "Local Chat - %s (Client %d)", my_name, client_id);

    init_gui(0, NULL, send_message_v2, title);
    run_gui();

    running = 0;
    pthread_cancel(recv_tid);
    pthread_join(recv_tid, NULL);

    if (client_id == 1) shm_ptr2->client1_active = 0;
    else shm_ptr2->client2_active = 0;
    
    munmap(shm_ptr2, sizeof(SharedState2));
    
    return 0;
}