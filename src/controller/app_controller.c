#include "controller/app_controller.h"
#include <stdlib.h>

// Forward declaration
static void on_add_button_clicked(GtkButton *button, gpointer user_data);

AppController *app_controller_new(GtkApplication *app) {
    AppController *controller = g_malloc(sizeof(AppController));

    controller->model = app_model_new();
    controller->view = app_view_new(app);
    controller->db = db_connection_new("localhost", "postgres", "", "gtkapp");

    if (!controller->view) {
        g_warning("Failed to initialize view");
        app_controller_free(controller);
        return NULL;
    }

    g_signal_connect(controller->view->add_button, "clicked", G_CALLBACK(on_add_button_clicked),
                     controller);

    return controller;
}

void app_controller_free(AppController *controller) {
    if (controller) {
        if (controller->model) {
            app_model_free(controller->model);
        }
        if (controller->view) {
            app_view_free(controller->view);
        }
        if (controller->db) {
            db_connection_free(controller->db);
        }
        g_free(controller);
    }
}

static void on_add_button_clicked(GtkButton *button G_GNUC_UNUSED, gpointer user_data) {
    AppController *controller = (AppController *)user_data;

    const gchar *name = app_view_get_name_input(controller->view);
    const gchar *email = app_view_get_email_input(controller->view);

    if (!name || strlen(name) == 0 || !email || strlen(email) == 0) {
        g_warning("Name and email cannot be empty");
        return;
    }

    app_model_add_user(controller->model, name, email);
    app_view_clear_inputs(controller->view);
    app_controller_refresh_ui(controller);
}

void app_controller_add_user(AppController *controller) {
    if (!controller) {
        return;
    }

    const gchar *name = app_view_get_name_input(controller->view);
    const gchar *email = app_view_get_email_input(controller->view);

    app_model_add_user(controller->model, name, email);
    app_view_clear_inputs(controller->view);
}

void app_controller_load_users(AppController *controller) {
    if (!controller) {
        return;
    }

#ifdef HAVE_LIBPQ
    if (db_connection_is_connected(controller->db)) {
        const gchar *query = "SELECT id, name, email FROM users;";
        PGresult *res = db_connection_execute_query(controller->db, query);

        if (res && PQresultStatus(res) == PGRES_TUPLES_OK) {
            int rows = PQntuples(res);
            for (int i = 0; i < rows; i++) {
                const char *name = PQgetvalue(res, i, 1);
                const char *email = PQgetvalue(res, i, 2);

                app_model_add_user(controller->model, name, email);
            }
            PQclear(res);
        }
    }
#endif
}

void app_controller_refresh_ui(AppController *controller) {
    if (!controller) {
        return;
    }

    GPtrArray *users = app_model_get_users(controller->model);
    app_view_display_users(controller->view, users);
}
