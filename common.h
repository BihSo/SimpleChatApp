#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <gtk/gtk.h> // مكتبة الواجهة

// الثوابت
#define MAX_MSG_LEN 256
#define SHM_NAME "/chat_shm"
#define TCP_PORT 8080

// هيكل الرسالة
typedef struct {
    char sender[32];
    char text[MAX_MSG_LEN];
    int is_new;
} ChatMessage;

// تعريف دالة الإرسال (Callback)
typedef void (*send_func_t)(const char *message);

// --- دوال الواجهة (تم دمجها من gui.h) ---
void init_gui(int argc, char *argv[], send_func_t send_callback, const char *title);
void gui_append_message(const char *sender, const char *message);
void gui_set_status(const char *status_text, gboolean is_connected);
int run_gui();

// --- دوال التشغيل (تم دمجها من client.h) ---
// هذه هي الدوال التي كانت في client.h سابقاً
int run_shm_client(int id, const char *name);
int run_tcp_client(const char *ip, const char *name);

#endif // COMMON_H