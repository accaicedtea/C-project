#include "view/app_view.h"
#include <stdio.h>

static void app_view_load_css(void) {
  GtkCssProvider *provider = gtk_css_provider_new();
  GError *error = NULL;
  gchar *css_content = NULL;
  // Load CSS file content
  if (!g_file_get_contents("ui/style.css", &css_content, NULL, &error)) {
    g_warning("Failed to read CSS file: %s", error->message);
    g_error_free(error);
    g_object_unref(provider);
    return;
  }

  // Parse CSS from string
  gtk_css_provider_load_from_string(provider, css_content);

  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_free(css_content);
  g_object_unref(provider);
  g_message("CSS stylesheet loaded successfully");
}

AppView *app_view_new(GtkApplication *app) {
  AppView *view = g_malloc(sizeof(AppView));
  GtkBuilder *builder = gtk_builder_new();
  GError *error = NULL;

  // Load CSS stylesheet
  app_view_load_css();

  if (!gtk_builder_add_from_file(builder, "ui/main_window.ui", &error)) {
    g_warning("Failed to load UI file: %s", error->message);
    g_error_free(error);
    g_free(view);
    return NULL;
  }

  view->main_window =
      GTK_APPLICATION_WINDOW(gtk_builder_get_object(builder, "main_window"));
  view->name_entry = GTK_ENTRY(gtk_builder_get_object(builder, "name_entry"));
  view->email_entry = GTK_ENTRY(gtk_builder_get_object(builder, "email_entry"));
  view->add_button = GTK_BUTTON(gtk_builder_get_object(builder, "add_button"));
  view->users_list =
      GTK_LIST_BOX(gtk_builder_get_object(builder, "users_list"));

  gtk_window_set_application(GTK_WINDOW(view->main_window), app);

  g_object_unref(builder);
  return view;
}

void app_view_free(AppView *view) {
  if (view) {
    g_free(view);
  }
}

void app_view_display_users(AppView *view, GPtrArray *users) {
  if (!view || !users) {
    return;
  }

  while (TRUE) {
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(view->users_list, 0);
    if (!row) {
      break;
    }
    gtk_list_box_remove(view->users_list, GTK_WIDGET(row));
  }

  for (guint i = 0; i < users->len; i++) {
    User *user = g_ptr_array_index(users, i);
    app_view_add_user_row(view, user);
  }
}

void app_view_add_user_row(AppView *view, User *user) {
  if (!view || !user) {
    return;
  }

  GtkBox *row_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10));
  gtk_widget_set_margin_top(GTK_WIDGET(row_box), 5);
  gtk_widget_set_margin_bottom(GTK_WIDGET(row_box), 5);
  gtk_widget_set_margin_start(GTK_WIDGET(row_box), 10);
  gtk_widget_set_margin_end(GTK_WIDGET(row_box), 10);

  GtkLabel *name_label = GTK_LABEL(gtk_label_new(user->name));
  gtk_label_set_xalign(name_label, 0);
  gtk_widget_set_hexpand(GTK_WIDGET(name_label), TRUE);

  GtkLabel *email_label = GTK_LABEL(gtk_label_new(user->email));
  gtk_label_set_xalign(email_label, 0);

  gtk_box_append(row_box, GTK_WIDGET(name_label));
  gtk_box_append(row_box, GTK_WIDGET(email_label));

  gtk_list_box_append(view->users_list, GTK_WIDGET(row_box));
}

void app_view_clear_inputs(AppView *view) {
  if (!view) {
    return;
  }

  gtk_editable_set_text(GTK_EDITABLE(view->name_entry), "");
  gtk_editable_set_text(GTK_EDITABLE(view->email_entry), "");
}

gchar *app_view_get_name_input(AppView *view) {
  if (!view) {
    return NULL;
  }
  return (gchar *)gtk_editable_get_text(GTK_EDITABLE(view->name_entry));
}

gchar *app_view_get_email_input(AppView *view) {
  if (!view) {
    return NULL;
  }
  return (gchar *)gtk_editable_get_text(GTK_EDITABLE(view->email_entry));
}

GtkApplicationWindow *app_view_get_window(AppView *view) {
  if (!view) {
    return NULL;
  }
  return view->main_window;
}
