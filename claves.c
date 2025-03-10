#include <stdio.h>
#include <sqlite3.h>
#include "claves.h"

#define DB_NAME "database.db"

void crearBaseDatos() {
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        return;
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS datos ("
                      "key INTEGER PRIMARY KEY, "
                      "value1 TEXT NOT NULL, "
                      "value2 BLOB NOT NULL, "
                      "value2_len INTEGER NOT NULL, "
                      "value3_x INTEGER NOT NULL, "
                      "value3_y INTEGER NOT NULL);";

    sqlite3_exec(db, sql, NULL, NULL, NULL);
    sqlite3_close(db);
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    crearBaseDatos();
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("database.db", &db) != SQLITE_OK) {
        printf("Error al abrir la base de datos\n");
        return -1;
    }

    printf("Abierta la base de datos correctamente.\n");

    const char *sql = "INSERT INTO datos (key, value1, value2, value2_len, value3_x, value3_y) VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Error al preparar la consulta: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    //Pasamos de vector a texto para guardarlo en la base de datos
    char buffer[100];  // Un buffer suficientemente grande
    int offset = 0;

    for (int i = 0; i < N_value2; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%.2f, ", V_value2[i]);
    }

    sqlite3_bind_int(stmt, 1, key);
    sqlite3_bind_text(stmt, 2, value1, -1, SQLITE_STATIC);
    //sqlite3_bind_blob(stmt, 3, V_value2, N_value2 * sizeof(double), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, buffer, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, N_value2);
    sqlite3_bind_int(stmt, 5, value3.x);
    sqlite3_bind_int(stmt, 6, value3.y);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Error al insertar en la base de datos: %s\n", sqlite3_errmsg(db));
    } else {
        printf("Inserción realizada con éxito.\n");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}
