#include "utils/db_connection.h"
#include <stdio.h>
#include <string.h>

DBConnection *db_connection_new(const gchar *host, const gchar *user,
                                const gchar *password, const gchar *dbname) {
  DBConnection *db = g_malloc(sizeof(DBConnection));

  db->connection_string =
      g_strdup_printf("host=%s user=%s password=%s dbname=%s",
                      host ? host : "localhost", user ? user : "postgres",
                      password ? password : "", dbname ? dbname : "gtkapp");

  db->conn = NULL;
  return db;
}

void db_connection_free(DBConnection *db) {
  if (db) {
    if (db->conn) {
      PQfinish(db->conn);
    }
    g_free(db->connection_string);
    g_free(db);
  }
}

gboolean db_connection_connect(DBConnection *db) {
  if (!db) {
    return FALSE;
  }

  db->conn = PQconnectdb(db->connection_string);

  if (PQstatus(db->conn) != CONNECTION_OK) {
    g_warning("Connection to PostgreSQL failed: %s", PQerrorMessage(db->conn));
    PQfinish(db->conn);
    db->conn = NULL;
    return FALSE;
  }

  g_message("Connected to PostgreSQL database");
  return TRUE;
}

void db_connection_disconnect(DBConnection *db) {
  if (db && db->conn) {
    PQfinish(db->conn);
    db->conn = NULL;
    g_message("Disconnected from PostgreSQL database");
  }
}

PGresult *db_connection_execute_query(DBConnection *db, const gchar *query) {
  if (!db || !db->conn || !query) {
    return NULL;
  }

  PGresult *res = PQexec(db->conn, query);

  if (PQstatus(db->conn) == CONNECTION_BAD) {
    g_warning("Database connection lost");
    return NULL;
  }

  if (PQresultStatus(res) != PGRES_TUPLES_OK &&
      PQresultStatus(res) != PGRES_COMMAND_OK) {
    g_warning("Query failed: %s", PQerrorMessage(db->conn));
  }

  return res;
}

gboolean db_connection_is_connected(DBConnection *db) {
  if (!db || !db->conn) {
    return FALSE;
  }
  return PQstatus(db->conn) == CONNECTION_OK;
}
