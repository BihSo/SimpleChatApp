#include "common.h"
#include <ctype.h>

static GtkWidget *text_view;
static GtkTextBuffer *text_buffer;
static GtkWidget *entry;
static GtkWidget *status_label;
static GtkWidget *chat_window = NULL;
static send_func_t on_send_message = NULL;

typedef struct {
    char *sender;
    char *message;
} UiUpdateData;

static void on_send_clicked(GtkWidget *widget, gpointer data) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (text && strlen(text) > 0 && on_send_message) {
        on_send_message(text);
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}

static void on_entry_activate(GtkWidget *widget, gpointer data) {
    on_send_clicked(widget, data);
}

static gboolean update_ui_safe(gpointer user_data) {
    UiUpdateData *data = (UiUpdateData *)user_data;
    
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(text_buffer, &end);
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[16];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", t);
    
    char formatted_msg[MAX_MSG_LEN + 100];
    snprintf(formatted_msg, sizeof(formatted_msg), "[%s] %s: %s\n",
    timestamp, data->sender, data->message);
    
    GtkTextTag *tag = NULL;
    if (strcmp(data->sender, "Me") == 0) {
        tag = gtk_text_buffer_create_tag(text_buffer, NULL, "foreground", "#0066CC", "weight", PANGO_WEIGHT_SEMIBOLD, NULL);
    } else {
        tag = gtk_text_buffer_create_tag(text_buffer, NULL, "foreground", "#00AA00", "weight", PANGO_WEIGHT_SEMIBOLD, NULL);
    }
    
    gtk_text_buffer_insert_with_tags(text_buffer, &end, formatted_msg, -1, tag, NULL);
    
    GtkTextMark *mark = gtk_text_buffer_create_mark(text_buffer, NULL, &end, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), mark, 0.0, TRUE, 0.0, 1.0);
    gtk_text_buffer_delete_mark(text_buffer, mark);

    free(data->sender);
    free(data->message);
    free(data);
    
    return FALSE;
}

void gui_append_message(const char *sender, const char *message) {
    UiUpdateData *data = malloc(sizeof(UiUpdateData));
    data->sender = strdup(sender);
    data->message = strdup(message);
    g_idle_add(update_ui_safe, data);
}

void gui_set_status(const char *status_text, gboolean is_connected) {
    if (!status_label) return;
    gtk_label_set_text(GTK_LABEL(status_label), status_text);
    PangoAttrList *attrs = pango_attr_list_new();
    if (is_connected) {
        pango_attr_list_insert(attrs, pango_attr_foreground_new(0, 50000, 0));
    } else {
        pango_attr_list_insert(attrs, pango_attr_foreground_new(50000, 0, 0));
    }
    gtk_label_set_attributes(GTK_LABEL(status_label), attrs);
    pango_attr_list_unref(attrs);
}

void init_gui(int argc, char *argv[], send_func_t send_callback, const char *title) {
    if (gtk_init_check(&argc, &argv) == FALSE) {
        fprintf(stderr, "Failed to initialize GTK\n");
        exit(1);
    }
    
    on_send_message = send_callback;

    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(chat_window), title);
    gtk_window_set_default_size(GTK_WINDOW(chat_window), 450, 550);
    g_signal_connect(chat_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background: #f0f0f0; }"
        "textview { font-family: 'Sans'; font-size: 14px; }"
        "entry { padding: 5px; border-radius: 5px; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(chat_window), vbox);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Type a message...");
    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

    GtkWidget *send_btn = gtk_button_new_with_label("Send");
    g_signal_connect(send_btn, "clicked", G_CALLBACK(on_send_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), send_btn, FALSE, FALSE, 0);

    status_label = gtk_label_new("● Ready");
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);

    gtk_widget_show_all(chat_window);
}

int run_gui() {
    gtk_main();
    return 0;
}