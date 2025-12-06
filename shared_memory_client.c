#include "gui.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

typedef struct {
    ChatMessage c1_to_c2;
    int c1_has_msg;
    
    ChatMessage c2_to_c1;
    int c2_has_msg;
    
    // Client activity flags
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
        shm_ptr2->c1_has_msg = 1;
    } else {
        strncpy(shm_ptr2->c2_to_c1.sender, my_name, 31);
        strncpy(shm_ptr2->c2_to_c1.text, text, MAX_MSG_LEN - 1);
        shm_ptr2->c2_has_msg = 1;
    }

    sem_post(sem_send_data);
    gui_append_message("Me", text);
}

void *receive_thread_func_v2(void *arg) {
    while (running) {
        if (sem_wait(sem_recv_data) != 0) {
            perror("sem_wait");
            break;
        }

        ChatMessage *msg;
        if (client_id == 1) {
            msg = &shm_ptr2->c2_to_c1;
        } else {
            msg = &shm_ptr2->c1_to_c2;
        }
        
        gui_append_message(msg->sender, msg->text);
        sem_post(sem_recv_space);
    }
    return NULL;
}

#include "client.h"

int run_shm_client(int id, const char *name) {
    client_id = id;
    strncpy(my_name, name, 31);
    
    if (client_id != 1 && client_id != 2) {
        fprintf(stderr, "Client ID must be 1 or 2\n");
        return 1;
    }

    char title[64];
    snprintf(title, sizeof(title), "Shared Memory Chat - %s (Client %d)", my_name, client_id);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Failed to create shared memory.\n\n"
            "This usually means:\n"
            "- Permission denied\n"
            "- System doesn't support POSIX shared memory");
        gtk_window_set_title(GTK_WINDOW(dialog), "Shared Memory Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        perror("shm_open");
        return 1;
    }

    if (ftruncate(fd, sizeof(SharedState2)) == -1) {
        perror("ftruncate");
        return 1;
    }

    shm_ptr2 = mmap(NULL, sizeof(SharedState2), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr2 == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    if (client_id == 1) {
        sem_send_data = sem_open("/sem_c1_to_c2_data", O_CREAT, 0666, 0);
        sem_send_space = sem_open("/sem_c1_to_c2_space", O_CREAT, 0666, 1);
        sem_recv_data = sem_open("/sem_c2_to_c1_data", O_CREAT, 0666, 0);
        sem_recv_space = sem_open("/sem_c2_to_c1_space", O_CREAT, 0666, 1);
    } else {
        sem_send_data = sem_open("/sem_c2_to_c1_data", O_CREAT, 0666, 0);
        sem_send_space = sem_open("/sem_c2_to_c1_space", O_CREAT, 0666, 1);
        sem_recv_data = sem_open("/sem_c1_to_c2_data", O_CREAT, 0666, 0);
        sem_recv_space = sem_open("/sem_c1_to_c2_space", O_CREAT, 0666, 1);
    }

    if (sem_send_data == SEM_FAILED || sem_send_space == SEM_FAILED ||
        sem_recv_data == SEM_FAILED || sem_recv_space == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    // Check if this client ID is already active
    if (client_id == 1) {
        if (shm_ptr2->client1_active) {
            GtkWidget *dialog = gtk_message_dialog_new(NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Client ID 1 is already in use!\n\n"
                "Another instance is already connected with Client ID 1.\n"
                "Please use Client ID 2 or close the other instance.");
            gtk_window_set_title(GTK_WINDOW(dialog), "Client ID Conflict");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            munmap(shm_ptr2, sizeof(SharedState2));
            return 1;
        }
        shm_ptr2->client1_active = 1;
    } else {
        if (shm_ptr2->client2_active) {
            GtkWidget *dialog = gtk_message_dialog_new(NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Client ID 2 is already in use!\n\n"
                "Another instance is already connected with Client ID 2.\n"
                "Please use Client ID 1 or close the other instance.");
            gtk_window_set_title(GTK_WINDOW(dialog), "Client ID Conflict");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            munmap(shm_ptr2, sizeof(SharedState2));
            return 1;
        }
        shm_ptr2->client2_active = 1;
    }

    // Reset running flag for new connection
    running = 1;

    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_thread_func_v2, NULL);

    int argc = 0;
    char **argv = NULL;
    init_gui(argc, argv, send_message_v2, title);
    int ret = run_gui();

    running = 0;
    
    pthread_cancel(recv_tid);
    pthread_join(recv_tid, NULL);
    
    // Clear active flag
    if (client_id == 1) {
        shm_ptr2->client1_active = 0;
    } else {
        shm_ptr2->client2_active = 0;
    }
    
    munmap(shm_ptr2, sizeof(SharedState2));
    
    sem_close(sem_send_data);
    sem_close(sem_send_space);
    sem_close(sem_recv_data);
    sem_close(sem_recv_space);
    
    return ret;
}
