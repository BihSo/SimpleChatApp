#include "gui.h"
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>

static GtkWidget *text_view;
static GtkTextBuffer *text_buffer;
static GtkWidget *entry;
static GtkWidget *status_label;
static send_func_t on_send_message = NULL;

// Structure to pass data to the main thread for UI updates
typedef struct {
    char *sender;
    char *message;
} UiUpdateData;

// Helper function to trim whitespace
static char* trim_whitespace(const char *str) {
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return NULL;
    
    const char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    size_t len = end - str + 1;
    char *trimmed = malloc(len + 1);
    memcpy(trimmed, str, len);
    trimmed[len] = '\0';
    return trimmed;
}

// Callback for the "Send" button
static void on_send_clicked(GtkWidget *widget, gpointer data) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (text && strlen(text) > 0) {
        char *trimmed = trim_whitespace(text);
        if (trimmed && strlen(trimmed) > 0) {
            if (on_send_message) {
                on_send_message(trimmed);
            }
            free(trimmed);
            gtk_entry_set_text(GTK_ENTRY(entry), "");
        } else {
            if (trimmed) free(trimmed);
        }
    }
}

// Callback for "Enter" key in the entry box
static void on_entry_activate(GtkWidget *widget, gpointer data) {
    on_send_clicked(widget, data);
}

// Function executed on the main thread to update the UI
static gboolean update_ui_safe(gpointer user_data) {
    UiUpdateData *data = (UiUpdateData *)user_data;
    
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(text_buffer, &end);
    
    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[16];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", t);
    
    // Format message with timestamp
    char formatted_msg[MAX_MSG_LEN + 100];
    snprintf(formatted_msg, sizeof(formatted_msg), "[%s] %s: %s\n", 
             timestamp, data->sender, data->message);
    
    // Create text tag for color based on sender
    GtkTextTag *tag = NULL;
    if (strcmp(data->sender, "Me") == 0) {
        // My messages in blue
        tag = gtk_text_buffer_create_tag(text_buffer, NULL,
                                         "foreground", "#0066CC",
                                         "weight", PANGO_WEIGHT_SEMIBOLD,
                                         NULL);
    } else {
        // Other messages in green
        tag = gtk_text_buffer_create_tag(text_buffer, NULL,
                                         "foreground", "#00AA00",
                                         "weight", PANGO_WEIGHT_SEMIBOLD,
                                         NULL);
    }
    
    gtk_text_buffer_insert_with_tags(text_buffer, &end, formatted_msg, -1, tag, NULL);
    
    // Scroll to bottom
    GtkTextMark *mark = gtk_text_buffer_create_mark(text_buffer, NULL, &end, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), mark, 0.0, TRUE, 0.0, 1.0);
    gtk_text_buffer_delete_mark(text_buffer, mark);

    free(data->sender);
    free(data->message);
    free(data);
    
    return FALSE; // Remove source
}

// Public function to append message (Thread-safe)
void gui_append_message(const char *sender, const char *message) {
    UiUpdateData *data = malloc(sizeof(UiUpdateData));
    data->sender = strdup(sender);
    data->message = strdup(message);
    g_idle_add(update_ui_safe, data);
}

// Update connection status
void gui_set_status(const char *status_text, gboolean is_connected) {
    if (!status_label) return;
    
    gtk_label_set_text(GTK_LABEL(status_label), status_text);
    
    PangoAttrList *attrs = pango_attr_list_new();
    if (is_connected) {
        pango_attr_list_insert(attrs, pango_attr_foreground_new(0, 50000, 0)); // Green
    } else {
        pango_attr_list_insert(attrs, pango_attr_foreground_new(50000, 0, 0)); // Red
    }
    gtk_label_set_attributes(GTK_LABEL(status_label), attrs);
    pango_attr_list_unref(attrs);
}

static int gui_exit_code = 0; // 0 = Exit, 1 = Back
static GtkWidget *chat_window = NULL;

// Store data before destroying window
static char stored_name[32] = {0};
static int stored_id = 0;
static char stored_ip[32] = {0};

static void on_back_clicked(GtkWidget *widget, gpointer data) {
    gui_exit_code = 1;
    if (chat_window) {
        gtk_widget_destroy(chat_window);
    }
    gtk_main_quit();
}

// Initialize the GUI
// argc, argv: Command line arguments
// send_callback: Function to call when the user clicks "Send"
// title: Window title (e.g., "Client 1")
void init_gui(int argc, char *argv[], send_func_t send_callback, const char *title) {
    gtk_init(&argc, &argv);
    
    on_send_message = send_callback;

    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(chat_window), title);
    gtk_window_set_default_size(GTK_WINDOW(chat_window), 500, 600);
    g_signal_connect(chat_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Apply CSS styling
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background: #f5f5f5; }"
        "textview { background: white; font-size: 14px; padding: 10px; }"
        "entry { font-size: 14px; padding: 8px; border-radius: 5px; }"
        "button { background: #4CAF50; color: white; font-weight: bold; padding: 8px 16px; border-radius: 5px; }"
        "button:hover { background: #45a049; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(chat_window), vbox);

    // Top Bar (Back Button)
    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), top_box, FALSE, FALSE, 0);

    GtkWidget *back_btn = gtk_button_new_with_label("Back to Menu");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(top_box), back_btn, FALSE, FALSE, 0);

    GtkWidget *title_label = gtk_label_new(title);
    gtk_widget_set_name(title_label, "title");
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.2));
    gtk_label_set_attributes(GTK_LABEL(title_label), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_pack_start(GTK_BOX(top_box), title_label, TRUE, FALSE, 0);

    // Chat Area with frame
    GtkWidget *frame = gtk_frame_new("Messages");
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), 10);
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

    // Input Area
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Type your message...");
    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

    GtkWidget *send_btn = gtk_button_new_with_label("Send");
    g_signal_connect(send_btn, "clicked", G_CALLBACK(on_send_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), send_btn, FALSE, FALSE, 0);

    // Status Bar
    status_label = gtk_label_new("● Connected");
    gtk_widget_set_name(status_label, "status");
    PangoAttrList *status_attrs = pango_attr_list_new();
    pango_attr_list_insert(status_attrs, pango_attr_foreground_new(0, 50000, 0)); // Green
    gtk_label_set_attributes(GTK_LABEL(status_label), status_attrs);
    pango_attr_list_unref(status_attrs);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 5);

    gtk_widget_show_all(chat_window);
}

// Login Window Logic
static GtkWidget *login_window;
static GtkWidget *entry_name, *entry_id, *entry_ip;
static GtkWidget *radio_shm, *radio_ws;
static int login_result = -1;

static GtkWidget *btn_start_server;
static pid_t server_pid = -1;

static void show_error_dialog(GtkWindow *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(parent,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), "Error");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static gboolean is_valid_ip(const char *ip) {
    if (!ip || strlen(ip) == 0) return FALSE;
    int a, b, c, d;
    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) return FALSE;
    if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255) return FALSE;
    return TRUE;
}

static void on_start_server_clicked(GtkWidget *widget, gpointer data) {
    pid_t pid = fork();
    if (pid == 0) {
        execl("./tcp_server", "tcp_server", NULL);
        perror("execl failed");
        exit(1);
    } else if (pid > 0) {
        server_pid = pid;
        gtk_button_set_label(GTK_BUTTON(btn_start_server), "✓ Server Running");
        gtk_widget_set_sensitive(btn_start_server, FALSE);
        
        // Disable IP field to prevent changes
        gtk_widget_set_sensitive(entry_ip, FALSE);
        
        // Show success message
        const char *ip = gtk_entry_get_text(GTK_ENTRY(entry_ip));
        if (!ip || strlen(ip) == 0) ip = "127.0.0.1";
        
        GtkWidget *info = gtk_message_dialog_new(GTK_WINDOW(login_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Server started successfully!\n\n"
            "The server is now running on %s port %d.\n\n"
            "Other users can connect to this server by:\n"
            "1. Selecting 'Network Mode'\n"
            "2. Entering the IP: %s\n"
            "3. Clicking Connect (DO NOT start another server)", 
            ip, TCP_PORT, ip);
        gtk_window_set_title(GTK_WINDOW(info), "Server Started");
        gtk_dialog_run(GTK_DIALOG(info));
        gtk_widget_destroy(info);
    } else {
        perror("fork failed");
        show_error_dialog(GTK_WINDOW(login_window), "Failed to start server. Please try again.");
    }
}

static void on_login_clicked(GtkWidget *widget, gpointer data) {
    const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
    
    // Validate name
    if (!name || strlen(name) == 0 || strlen(name) > 31) {
        show_error_dialog(GTK_WINDOW(login_window), "Please enter a valid name (1-31 characters).");
        return;
    }
    
    gboolean is_shm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_shm));
    
    if (is_shm) {
        // Validate Client ID
        const char *id_text = gtk_entry_get_text(GTK_ENTRY(entry_id));
        int id = atoi(id_text);
        if (id != 1 && id != 2) {
            show_error_dialog(GTK_WINDOW(login_window), "Client ID must be 1 or 2 for Shared Memory mode.");
            return;
        }
    } else {
        // Validate IP Address
        const char *ip = gtk_entry_get_text(GTK_ENTRY(entry_ip));
        if (!is_valid_ip(ip)) {
            show_error_dialog(GTK_WINDOW(login_window), "Please enter a valid IP address (e.g., 127.0.0.1).");
            return;
        }
    }
    
    // Store values before quitting
    const char *name_text = gtk_entry_get_text(GTK_ENTRY(entry_name));
    if (name_text) {
        strncpy(stored_name, name_text, 31);
        stored_name[31] = '\0';
    }
    
    if (is_shm) {
        const char *id_text = gtk_entry_get_text(GTK_ENTRY(entry_id));
        if (id_text) {
            stored_id = atoi(id_text);
        }
    } else {
        const char *ip_text = gtk_entry_get_text(GTK_ENTRY(entry_ip));
        if (ip_text) {
            strncpy(stored_ip, ip_text, 31);
            stored_ip[31] = '\0';
        } else {
            printf("DEBUG: IP text is NULL\n");
        }
    }
    
    login_result = is_shm ? 1 : 2;
    gtk_main_quit();
}

static void on_mode_toggled(GtkWidget *widget, gpointer data) {
    gboolean is_shm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_shm));
    
    // Get stored widgets
    GtkWidget *shm_box = (GtkWidget*)g_object_get_data(G_OBJECT(radio_shm), "config_box");
    GtkWidget *tcp_box = (GtkWidget*)g_object_get_data(G_OBJECT(radio_ws), "config_box");
    
    // Show/hide entire configuration boxes
    if (shm_box) gtk_widget_set_visible(shm_box, is_shm);
    if (tcp_box) gtk_widget_set_visible(tcp_box, !is_shm);
}

static void on_window_destroy(GtkWidget *widget, gpointer data) {
    // Store values before widgets are destroyed
    if (login_result != -1 && GTK_IS_ENTRY(entry_name)) {
        const char *name_text = gtk_entry_get_text(GTK_ENTRY(entry_name));
        if (name_text) {
            strncpy(stored_name, name_text, 31);
            stored_name[31] = '\0';
        }
        
        if (login_result == 1 && GTK_IS_ENTRY(entry_id)) {
            const char *id_text = gtk_entry_get_text(GTK_ENTRY(entry_id));
            if (id_text) {
                stored_id = atoi(id_text);
            }
        } else if (GTK_IS_ENTRY(entry_ip)) {
            const char *ip_text = gtk_entry_get_text(GTK_ENTRY(entry_ip));
            if (ip_text) {
                strncpy(stored_ip, ip_text, 31);
                stored_ip[31] = '\0';
            }
        }
    }
}

int create_login_window(int argc, char *argv[], char *name, int *id, char *ip) {
    login_result = -1; // Reset result
    gtk_init(&argc, &argv);

    login_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(login_window), "Chat Application");
    gtk_window_set_default_size(GTK_WINDOW(login_window), 480, 420);
    gtk_window_set_position(GTK_WINDOW(login_window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(login_window), FALSE);
    g_signal_connect(login_window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    g_signal_connect(login_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 25);
    gtk_container_add(GTK_CONTAINER(login_window), main_vbox);

    // Title
    GtkWidget *title_label = gtk_label_new("Chat Application");
    PangoAttrList *title_attrs = pango_attr_list_new();
    pango_attr_list_insert(title_attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(title_attrs, pango_attr_scale_new(1.6));
    gtk_label_set_attributes(GTK_LABEL(title_label), title_attrs);
    pango_attr_list_unref(title_attrs);
    gtk_box_pack_start(GTK_BOX(main_vbox), title_label, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(main_vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);

    // Name Input
    GtkWidget *name_label = gtk_label_new("Enter Your Name:");
    PangoAttrList *label_attrs = pango_attr_list_new();
    pango_attr_list_insert(label_attrs, pango_attr_weight_new(PANGO_WEIGHT_SEMIBOLD));
    gtk_label_set_attributes(GTK_LABEL(name_label), label_attrs);
    pango_attr_list_unref(label_attrs);
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(main_vbox), name_label, FALSE, FALSE, 0);
    
    entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_name), "e.g., Alice, Bob, Charlie");
    gtk_box_pack_start(GTK_BOX(main_vbox), entry_name, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);

    // Mode Selection
    GtkWidget *mode_label = gtk_label_new("Choose Mode:");
    PangoAttrList *mode_attrs = pango_attr_list_new();
    pango_attr_list_insert(mode_attrs, pango_attr_weight_new(PANGO_WEIGHT_SEMIBOLD));
    gtk_label_set_attributes(GTK_LABEL(mode_label), mode_attrs);
    pango_attr_list_unref(mode_attrs);
    gtk_widget_set_halign(mode_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(main_vbox), mode_label, FALSE, FALSE, 0);

    // Shared Memory Mode
    radio_shm = gtk_radio_button_new_with_label(NULL, "Local Mode (2 clients on same PC)");
    gtk_box_pack_start(GTK_BOX(main_vbox), radio_shm, FALSE, FALSE, 0);

    // TCP Mode
    radio_ws = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_shm), 
                                                            "Network Mode (Multiple clients)");
    gtk_box_pack_start(GTK_BOX(main_vbox), radio_ws, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);

    // Shared Memory Configuration Box
    GtkWidget *shm_config_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    
    GtkWidget *shm_info = gtk_label_new("ℹ Local Mode: Two instances communicate via shared memory");
    gtk_widget_set_halign(shm_info, GTK_ALIGN_START);
    PangoAttrList *info_attrs = pango_attr_list_new();
    pango_attr_list_insert(info_attrs, pango_attr_style_new(PANGO_STYLE_ITALIC));
    pango_attr_list_insert(info_attrs, pango_attr_scale_new(0.9));
    pango_attr_list_insert(info_attrs, pango_attr_foreground_new(20000, 20000, 50000));
    gtk_label_set_attributes(GTK_LABEL(shm_info), info_attrs);
    pango_attr_list_unref(info_attrs);
    gtk_box_pack_start(GTK_BOX(shm_config_box), shm_info, FALSE, FALSE, 0);
    
    GtkWidget *id_label = gtk_label_new("Client ID:");
    gtk_widget_set_halign(id_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(shm_config_box), id_label, FALSE, FALSE, 0);
    
    entry_id = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_id), "Enter 1 or 2");
    gtk_entry_set_max_length(GTK_ENTRY(entry_id), 1);
    gtk_box_pack_start(GTK_BOX(shm_config_box), entry_id, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(main_vbox), shm_config_box, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(radio_shm), "config_box", shm_config_box);

    // TCP Configuration Box
    GtkWidget *tcp_config_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_visible(tcp_config_box, FALSE);
    
    GtkWidget *tcp_info = gtk_label_new("ℹ Network Mode: Connect multiple clients over TCP/IP");
    gtk_widget_set_halign(tcp_info, GTK_ALIGN_START);
    PangoAttrList *tcp_info_attrs = pango_attr_list_new();
    pango_attr_list_insert(tcp_info_attrs, pango_attr_style_new(PANGO_STYLE_ITALIC));
    pango_attr_list_insert(tcp_info_attrs, pango_attr_scale_new(0.9));
    pango_attr_list_insert(tcp_info_attrs, pango_attr_foreground_new(20000, 20000, 50000));
    gtk_label_set_attributes(GTK_LABEL(tcp_info), tcp_info_attrs);
    pango_attr_list_unref(tcp_info_attrs);
    gtk_box_pack_start(GTK_BOX(tcp_config_box), tcp_info, FALSE, FALSE, 0);
    
    GtkWidget *ip_label = gtk_label_new("Server IP Address:");
    gtk_widget_set_halign(ip_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(tcp_config_box), ip_label, FALSE, FALSE, 0);
    
    entry_ip = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_ip), "127.0.0.1");
    gtk_box_pack_start(GTK_BOX(tcp_config_box), entry_ip, FALSE, FALSE, 0);
    
    btn_start_server = gtk_button_new_with_label("⚙ Start Local Server");
    g_signal_connect(btn_start_server, "clicked", G_CALLBACK(on_start_server_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(tcp_config_box), btn_start_server, FALSE, FALSE, 3);
    
    gtk_box_pack_start(GTK_BOX(main_vbox), tcp_config_box, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(radio_ws), "config_box", tcp_config_box);

    g_signal_connect(radio_shm, "toggled", G_CALLBACK(on_mode_toggled), NULL);

    // Connect Button
    GtkWidget *connect_btn = gtk_button_new_with_label("Connect");
    gtk_widget_set_size_request(connect_btn, -1, 40);
    g_signal_connect(connect_btn, "clicked", G_CALLBACK(on_login_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(main_vbox), connect_btn, FALSE, FALSE, 5);

    gtk_widget_show_all(login_window);
    
    // Hide TCP config by default (after show_all)
    gtk_widget_hide(tcp_config_box);
    
    gtk_main();

    if (login_result != -1) {
        strncpy(name, stored_name, 31);
        if (login_result == 1) {
            *id = stored_id;
        } else {
            strncpy(ip, stored_ip, 31);
        }
        // Disconnect destroy signal to prevent double gtk_main_quit
        g_signal_handlers_disconnect_by_func(login_window, G_CALLBACK(gtk_main_quit), NULL);
        
        // Destroy login window as we are proceeding to chat
        gtk_widget_destroy(login_window);
    }
    
    return login_result;
}

int run_gui() {
    gui_exit_code = 0; // Reset
    gtk_main();
    return gui_exit_code;
}
