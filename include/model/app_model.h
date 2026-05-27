#ifndef APP_MODEL_H
#define APP_MODEL_H

#include <glib.h>

typedef struct {
  gint id;
  gchar *name;
  gchar *email;
} User;

typedef struct {
  GPtrArray *users;
} AppModel;

AppModel *app_model_new(void);
void app_model_free(AppModel *model);

void app_model_add_user(AppModel *model, const gchar *name, const gchar *email);
GPtrArray *app_model_get_users(AppModel *model);
void app_model_remove_user(AppModel *model, gint id);
User *app_model_get_user(AppModel *model, gint id);

User *user_new(gint id, const gchar *name, const gchar *email);
void user_free(User *user);

#endif
