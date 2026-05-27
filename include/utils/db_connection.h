#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#include <glib.h>

#ifdef HAVE_LIBPQ
#include <libpq-fe.h>
#else
typedef void PGconn;
typedef void PGresult;
#endif

typedef struct {
  PGconn *conn;
  gchar *connection_string;
} DBConnection;

DBConnection *db_connection_new(const gchar *host, const gchar *user,
                                const gchar *password, const gchar *dbname);
void db_connection_free(DBConnection *db);

gboolean db_connection_connect(DBConnection *db);
void db_connection_disconnect(DBConnection *db);

PGresult *db_connection_execute_query(DBConnection *db, const gchar *query);
gboolean db_connection_is_connected(DBConnection *db);

#endif
