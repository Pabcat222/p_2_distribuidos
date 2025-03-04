#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>

#define DB_NAME "database.db"

// Función para ejecutar consultas sin retorno de datos
int ejecutarSQL(sqlite3 *db, const char *sql) {
    char *error_msg = NULL;
    int result = sqlite3_exec(db, sql, NULL, 0, &error_msg);
    if (result != SQLITE_OK) {
        printf("Error en SQL: %s\n", error_msg);
        sqlite3_free(error_msg);
        return 1;
    }
    return 0;
}

// Función de callback para leer datos
int mostrarUsuarios(void *data, int argc, char **argv, char **col_names) {
    printf("ID: %s | Nombre: %s | Edad: %s\n", argv[0], argv[1], argv[2]);
    return 0;
}

// Crear base de datos y tabla
void crearBaseDatos() {
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("Error al abrir la base de datos\n");
        return;
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS usuarios ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "nombre TEXT NOT NULL, "
                      "edad INTEGER NOT NULL);";

    if (ejecutarSQL(db, sql) == 0) {
        printf("Tabla 'usuarios' lista.\n");
    }

    sqlite3_close(db);
}

// Insertar un usuario
void insertarUsuario() {
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("Error al abrir la base de datos\n");
        return;
    }

    char nombre[50];
    int edad;

    printf("Ingrese el nombre: ");
    scanf("%49s", nombre);
    printf("Ingrese la edad: ");
    scanf("%d", &edad);

    char sql[200];
    sprintf(sql, "INSERT INTO usuarios (nombre, edad) VALUES ('%s', %d);", nombre, edad);

    if (ejecutarSQL(db, sql) == 0) {
        printf("Usuario agregado correctamente.\n");
    }

    sqlite3_close(db);
}

// Leer usuarios
void leerUsuarios() {
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("Error al abrir la base de datos\n");
        return;
    }

    const char *sql = "SELECT * FROM usuarios;";
    printf("\nLista de Usuarios:\n-------------------\n");
    sqlite3_exec(db, sql, mostrarUsuarios, NULL, NULL);

    sqlite3_close(db);
}

// Actualizar un usuario
void actualizarUsuario() {
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("Error al abrir la base de datos\n");
        return;
    }

    int id, edad;
    char nombre[50];

    printf("Ingrese el ID del usuario a modificar: ");
    scanf("%d", &id);
    printf("Ingrese el nuevo nombre: ");
    scanf("%49s", nombre);
    printf("Ingrese la nueva edad: ");
    scanf("%d", &edad);

    char sql[200];
    sprintf(sql, "UPDATE usuarios SET nombre = '%s', edad = %d WHERE id = %d;", nombre, edad, id);

    if (ejecutarSQL(db, sql) == 0) {
        printf("Usuario actualizado correctamente.\n");
    }

    sqlite3_close(db);
}

// Eliminar un usuario
void eliminarUsuario() {
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("Error al abrir la base de datos\n");
        return;
    }

    int id;
    printf("Ingrese el ID del usuario a eliminar: ");
    scanf("%d", &id);

    char sql[100];
    sprintf(sql, "DELETE FROM usuarios WHERE id = %d;", id);

    if (ejecutarSQL(db, sql) == 0) {
        printf("Usuario eliminado correctamente.\n");
    }

    sqlite3_close(db);
}

// Menú principal
void menu() {
    int opcion;

    do {
        printf("\n===== Menú Base de Datos SQLite =====\n");
        printf("1. Insertar usuario\n");
        printf("2. Mostrar usuarios\n");
        printf("3. Actualizar usuario\n");
        printf("4. Eliminar usuario\n");
        printf("5. Salir\n");
        printf("Seleccione una opción: ");
        scanf("%d", &opcion);

        switch (opcion) {
            case 1: insertarUsuario(); break;
            case 2: leerUsuarios(); break;
            case 3: actualizarUsuario(); break;
            case 4: eliminarUsuario(); break;
            case 5: printf("Saliendo...\n"); break;
            default: printf("Opción no válida.\n");
        }
    } while (opcion != 5);
}

int main() {
    crearBaseDatos();  // Crear la base de datos y la tabla si no existen
    menu();            // Ejecutar el menú interactivo
    return 0;
}
