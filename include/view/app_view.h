#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "model/app_model.h"
#include <gtk/gtk.h>

typedef struct {
  GtkApplicationWindow *main_window;
  GtkEntry *name_entry;
  GtkEntry *email_entry;
  GtkButton *add_button;
  GtkListBox *users_list;
} AppView;

AppView *app_view_new(GtkApplication *app);
void app_view_free(AppView *view);

void app_view_display_users(AppView *view, GPtrArray *users);
void app_view_add_user_row(AppView *view, User *user);
void app_view_clear_inputs(AppView *view);

gchar *app_view_get_name_input(AppView *view);
gchar *app_view_get_email_input(AppView *view);

GtkApplicationWindow *app_view_get_window(AppView *view);

#endif
