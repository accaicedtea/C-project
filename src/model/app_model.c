#include "model/app_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gint next_id = 1;

User *user_new(gint id, const gchar *name, const gchar *email) {
  User *user = g_malloc(sizeof(User));
  user->id = id;
  user->name = g_strdup(name);
  user->email = g_strdup(email);
  return user;
}

void user_free(User *user) {
  if (user) {
    g_free(user->name);
    g_free(user->email);
    g_free(user);
  }
}

AppModel *app_model_new(void) {
  AppModel *model = g_malloc(sizeof(AppModel));
  model->users = g_ptr_array_new_with_free_func((GDestroyNotify)user_free);
  return model;
}

void app_model_free(AppModel *model) {
  if (model) {
    g_ptr_array_unref(model->users);
    g_free(model);
  }
}

void app_model_add_user(AppModel *model, const gchar *name,
                        const gchar *email) {
  if (!model || !name || !email) {
    return;
  }

  User *user = user_new(next_id++, name, email);
  g_ptr_array_add(model->users, user);

  g_message("User added: %s (%s)", name, email);
}

GPtrArray *app_model_get_users(AppModel *model) {
  if (!model) {
    return NULL;
  }
  return model->users;
}

void app_model_remove_user(AppModel *model, gint id) {
  if (!model) {
    return;
  }

  for (guint i = 0; i < model->users->len; i++) {
    User *user = g_ptr_array_index(model->users, i);
    if (user->id == id) {
      g_ptr_array_remove_index(model->users, i);
      g_message("User removed with ID: %d", id);
      break;
    }
  }
}

User *app_model_get_user(AppModel *model, gint id) {
  if (!model) {
    return NULL;
  }

  for (guint i = 0; i < model->users->len; i++) {
    User *user = g_ptr_array_index(model->users, i);
    if (user->id == id) {
      return user;
    }
  }

  return NULL;
}
