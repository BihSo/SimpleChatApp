#include "gui.h"
#include "client.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

void cleanup_all() {
    // Clean up shared memory
    shm_unlink(SHM_NAME);
    
    // Clean up semaphores
    sem_unlink("/sem_c1_to_c2_data");
    sem_unlink("/sem_c1_to_c2_space");
    sem_unlink("/sem_c2_to_c1_data");
    sem_unlink("/sem_c2_to_c1_space");
}

int main(int argc, char *argv[]) {
    // Clean up any leftover resources from crashed sessions
    cleanup_all();
    
    // Register cleanup for normal exit
    atexit(cleanup_all);
    
    while (1) {
        char name[32] = {0};
        int id = 0;
        char ip[32] = {0};

        int mode = create_login_window(argc, argv, name, &id, ip);

        if (mode == -1) break;

        int ret = 0;
        if (mode == 1) {
            if (id != 1 && id != 2) {
                fprintf(stderr, "Invalid ID. Must be 1 or 2.\n");
                continue;
            }
            ret = run_shm_client(id, name);
        } else if (mode == 2) {
            ret = run_tcp_client(ip, name);
        }
        
        if (ret == 0) break;
    }

    return 0;
}
