#include <stdio.h>
#include <sqlite3.h>
#include "claves.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define DB_NAME "/tmp/database-5764-5879.db"
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

void crearBaseDatos() {
    pthread_mutex_lock(&db_mutex);
    //Se abre la bae de datos "datos" o se crea en caso de que no exista
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        pthread_mutex_unlock(&db_mutex);
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
    pthread_mutex_unlock(&db_mutex);
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    crearBaseDatos();

    sqlite3 *db;
    sqlite3_stmt *stmt;
    //Abrimos base de datos
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al abrir la base de datos\n");
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    printf("[BASE_DATOS]Abierta la base de datos correctamente.\n");

    //Insertamos valores en la tabla "datos"
    const char *sql = "INSERT INTO datos (key, value1, value2, value2_len, value3_x, value3_y) VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al preparar la consulta: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
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
    sqlite3_bind_text(stmt, 3, buffer, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, N_value2);
    sqlite3_bind_int(stmt, 5, value3.x);
    sqlite3_bind_int(stmt, 6, value3.y);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("[BASE_DATOS]Error al insertar en la base de datos: %s\n", sqlite3_errmsg(db));
        pthread_mutex_unlock(&db_mutex);
        return -1;
    } else {
        printf("[BASE_DATOS]Inserción realizada con éxito.\n");
    }
    //Cerramos la tabla modificada
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return 0;
}

int destroy(void){
    sqlite3 *db;
    char *err_msg = NULL;
    //Abrimos la tabla
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al abrir la base de datos: %s\n", sqlite3_errmsg(db));
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    // Eliminar todas las filas
    const char *sql = "DELETE FROM datos;";

    if (sqlite3_exec(db, sql, NULL, NULL, &err_msg) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al eliminar datos: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    printf("[BASE_DATOS]Eliminando la base de datos...\nTodas las filas de la base de datos han sido eliminadas correctamente.\n");
    //Cerramos la tabla
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return 0;
}

int delete_key(int key) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    
    //Abrimos la tabla
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al abrir la base de datos: %s\n", sqlite3_errmsg(db));
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }
    //Borramos la key seleccionada
    const char *sql = "DELETE FROM datos WHERE key = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al preparar la consulta: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, key);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        if (sqlite3_changes(db) == 0) {
            printf("[BASE_DATOS]La clave %d no existe en la base de datos.\n", key);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            pthread_mutex_unlock(&db_mutex);
            return -1;
        }
        printf("[BASE_DATOS]La clave %d ha sido eliminada correctamente.\n", key);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return 0;
    } else {
        printf("[BASE_DATOS]Error al eliminar la clave %d: %s\n", key, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }
}


int exist(int key) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    //Abrimos base de datos
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al abrir la base de datos: %s\n", sqlite3_errmsg(db));
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    //Buscamos el valor de la key
    const char *sql = "SELECT 1 FROM datos WHERE key = ? LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al preparar la consulta: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, key);

    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        printf("[BASE_DATOS]La clave %d existe en la base de datos.\n", key);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return 1; // La clave existe
    } else if (result == SQLITE_DONE) {
        printf("[BASE_DATOS]La clave %d no existe en la base de datos.\n", key);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return 0; // La clave no existe
    } else {
        printf("[BASE_DATOS]Error al comprobar la clave %d: %s\n", key, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1; // Error al ejecutar la consulta
    }
}


int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    
    sqlite3 *db;
    sqlite3_stmt *stmt;

    //Abrimos tabla
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al abrir la base de datos: %s\n", sqlite3_errmsg(db));
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    // Verificar si la clave existe
    const char *check_sql = "SELECT 1 FROM datos WHERE key = ? LIMIT 1;";
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al preparar la consulta de verificación: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, key);
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (result != SQLITE_ROW) {
        printf("[BASE_DATOS]La clave %d no existe en la base de datos.\n", key);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    // Convertir el array de doubles a una cadena
    char buffer[256]; // Un buffer suficientemente grande
    int offset = 0;
    for (int i = 0; i < N_value2; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%.2f, ", V_value2[i]);
    }

    // Preparar la consulta de actualización
    const char *update_sql = "UPDATE datos SET value1 = ?, value2 = ?, value2_len = ?, value3_x = ?, value3_y = ? WHERE key = ?;";
    if (sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al preparar la consulta de actualización: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    // Vincular los valores a la consulta
    sqlite3_bind_text(stmt, 1, value1, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, buffer, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, N_value2);
    sqlite3_bind_int(stmt, 4, value3.x);
    sqlite3_bind_int(stmt, 5, value3.y);
    sqlite3_bind_int(stmt, 6, key);

    // Ejecutar la consulta
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("[BASE_DATOS]Error al actualizar la clave %d: %s\n", key, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    printf("[BASE_DATOS]La clave %d ha sido actualizada correctamente.\n", key);

    //Cerrar tabla
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return 0;
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    // Abrimos la tabla
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al abrir la base de datos: %s\n", sqlite3_errmsg(db));
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    // Consulta SQL para obtener los valores asociados a la clave
    const char *sql = "SELECT value1, value2, value2_len, value3_x, value3_y FROM datos WHERE key = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[BASE_DATOS]Error al preparar la consulta: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    // Vincular la clave a la consulta
    sqlite3_bind_int(stmt, 1, key);

    // Ejecutar la consulta
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // Obtener value1
        strncpy(value1, (const char *)sqlite3_column_text(stmt, 0), 256);
        value1[255] = '\0'; // Asegurar que la cadena esté terminada en null

        // Obtener value2 (convertir de cadena a array de doubles)
        const char *value2_str = (const char *)sqlite3_column_text(stmt, 1);
        *N_value2 = sqlite3_column_int(stmt, 2);

        char *token = strtok((char *)value2_str, ", ");
        int i = 0;
        while (token != NULL && i < *N_value2) {
            V_value2[i++] = atof(token);
            token = strtok(NULL, ", ");
        }

        // Obtener value3
        value3->x = sqlite3_column_int(stmt, 3);
        value3->y = sqlite3_column_int(stmt, 4);

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return 0; // Éxito
    } else {
        printf("[BASE_DATOS]La clave %d no existe en la base de datos.\n", key);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
        return -1; // Clave no encontrada
    }
}