#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "model/app_model.h"
#include "utils/db_connection.h"
#include "view/app_view.h"
#include <gtk/gtk.h>

typedef struct {
  AppModel *model;
  AppView *view;
  DBConnection *db;
} AppController;

AppController *app_controller_new(GtkApplication *app);
void app_controller_free(AppController *controller);

void app_controller_add_user(AppController *controller);
void app_controller_load_users(AppController *controller);
void app_controller_refresh_ui(AppController *controller);

#endif
