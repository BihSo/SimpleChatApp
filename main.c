#include "common.h"

void print_usage() {
    printf("Usage:\n");
    printf("  For Local Mode (Shared Memory):\n");
    printf("    ./chat_app local <Client_ID> <Name>\n");
    printf("    Example: ./chat_app local 1 Alice\n\n");
    printf("  For Network Mode (TCP):\n");
    printf("    ./chat_app network <Server_IP> <Name>\n");
    printf("    Example: ./chat_app network 127.0.0.1 Bob\n");
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        print_usage();
        return 1;
    }

    const char *mode = argv[1];

    if (strcmp(mode, "local") == 0) {
        
        int id = atoi(argv[2]);
        const char *name = argv[3];

        if (id != 1 && id != 2) {
            printf("Error: Client ID must be 1 or 2 for local mode.\n");
            return 1;
        }
        
        run_shm_client(id, name);

    } else if (strcmp(mode, "network") == 0) {
        const char *ip = argv[2];
        const char *name = argv[3];
        
        run_tcp_client(ip, name);

    } else {
        printf("Error: Unknown mode '%s'\n", mode);
        print_usage();
        return 1;
    }

    return 0;
}