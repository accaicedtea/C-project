#include "controller/app_controller.h"
#include <gtk/gtk.h>

static void on_activate(GtkApplication *app, gpointer user_data G_GNUC_UNUSED) {
  AppController *controller = app_controller_new(app);

  if (!controller || !controller->view) {
    g_error("Failed to create application controller");
    return;
  }

  app_controller_refresh_ui(controller);

  GtkApplicationWindow *window = app_view_get_window(controller->view);
  gtk_window_present(GTK_WINDOW(window));

  g_object_set_data(G_OBJECT(app), "controller", controller);
}

static void on_shutdown(GtkApplication *app, gpointer user_data G_GNUC_UNUSED) {
  AppController *controller = g_object_get_data(G_OBJECT(app), "controller");
  if (controller) {
    app_controller_free(controller);
  }
}

int main(int argc, char *argv[]) {
  GtkApplication *app =
      gtk_application_new("org.example.gtkapp", G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  g_signal_connect(app, "shutdown", G_CALLBACK(on_shutdown), NULL);

  int status = g_application_run(G_APPLICATION(app), argc, argv);

  g_object_unref(app);
  return status;
}
