#ifndef CLIENT_H
#define CLIENT_H

// Entry point for Shared Memory Client
// Returns 0 for Exit, 1 for Back to Menu
int run_shm_client(int id, const char *name);

// Entry point for TCP Client
// Returns 0 for Exit, 1 for Back to Menu
int run_tcp_client(const char *ip, const char *name);

#endif // CLIENT_H
