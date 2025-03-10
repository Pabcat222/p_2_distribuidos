#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>


#define DB_NAME "database.db"

// Estructura Coord
typedef struct Coord {
    int x;
    int y;
} Coord;

// Ejecutar consultas SQL generales
int ejecutarSQL(sqlite3 *db, const char *sql) {
    char *error_msg = NULL;
    int result = sqlite3_exec(db, sql, NULL, 0, &error_msg);
    if (result != SQLITE_OK) {
        printf("Error SQL: %s\n", error_msg);
        sqlite3_free(error_msg);
        return 1;
    }
    return 0;
}

// Crear la tabla adaptada
void crearBaseDatos() {
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("Error al abrir la base de datos\n");
        return;
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS datos ("
                      "key INTEGER PRIMARY KEY, "
                      "value1 TEXT NOT NULL, "
                      "value2 BLOB NOT NULL, "
                      "value2_len INTEGER NOT NULL, "
                      "value3_x INTEGER NOT NULL, "
                      "value3_y INTEGER NOT NULL);";

    if (ejecutarSQL(db, sql) == 0)
        printf("Tabla 'datos' lista.\n");

    sqlite3_close(db);
}

// Insertar tupla en la tabla
void insertarTupla() {
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("Error al abrir la base de datos\n");
        return;
    }

    int key, n;
    char value1[256];
    double value2[32];
    Coord value3;

    printf("Ingrese la clave (entero): ");
    scanf("%d", &key);

    printf("Ingrese la cadena (max 255 chars): ");
    scanf(" %255[^\n]", value1);

    do {
        printf("Ingrese tamaño del vector (1-32): ");
        scanf("%d", &n);
    } while (n < 1 || n > 32);

    printf("Ingrese los %d elementos del vector (dobles):\n", n);
    for (int i = 0; i < n; i++) {
        scanf("%lf", &value2[i]);
    }

    printf("Ingrese las coordenadas x y: ");
    scanf("%d %d", &value3.x, &value3.y);

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO datos (key, value1, value2, value2_len, value3_x, value3_y) VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, key);
    sqlite3_bind_text(stmt, 2, value1, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 3, value2, sizeof(double) * n, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, n);
    sqlite3_bind_int(stmt, 5, value3.x);
    sqlite3_bind_int(stmt, 6, value3.y);

    if (sqlite3_step(stmt) == SQLITE_DONE)
        printf("Tupla insertada correctamente.\n");
    else
        printf("Error al insertar tupla.\n");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// Menú adaptado
void menu() {
    int opcion;

    do {
        printf("\n===== Menú Base de Datos Adaptado =====\n");
        printf("1. Insertar tupla\n");
        printf("2. Salir\n");
        printf("Seleccione una opción: ");
        scanf("%d", &opcion);

        switch (opcion) {
            case 1:
                insertarTupla();
                break;
            case 2:
                printf("Saliendo...\n");
                break;
            default:
                printf("Opción no válida.\n");
        }
    } while (opcion != 2);
}

int main() {
    crearBaseDatos();
    menu();
    return 0;
}