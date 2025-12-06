#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "common.h"

// Initialize the GUI
// argc, argv: Command line arguments
// send_callback: Function to call when the user clicks "Send"
// title: Window title (e.g., "Client 1")
void init_gui(int argc, char *argv[], send_func_t send_callback, const char *title);

// Show Login Window and wait for user input
// Returns 1 for Shared Memory, 2 for WebSocket, -1 on cancel
// Populates name, id (if SHM), ip (if WS)
int create_login_window(int argc, char *argv[], char *name, int *id, char *ip);

// Append a message to the chat area (Thread-safe)
void gui_append_message(const char *sender, const char *message);

// Update connection status (Thread-safe)
void gui_set_status(const char *status_text, gboolean is_connected);

// Start the GTK main loop
// Returns 0 for Exit, 1 for Back to Menu
int run_gui();

#endif // GUI_H
