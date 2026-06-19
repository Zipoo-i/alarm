#include "alarm_common.h"
#include <windows.h>

#define SCHTASKS_CMD "schtasks"

static char db_path_buffer[MAX_PATH] = {0};

const char* get_db_path(void) {
    if (db_path_buffer[0] == 0) {
        char exe_path[MAX_PATH];
        GetModuleFileName(NULL, exe_path, MAX_PATH);
        char *last_slash = strrchr(exe_path, '\\');
        if (last_slash) *(last_slash + 1) = '\0';
        snprintf(db_path_buffer, sizeof(db_path_buffer), "%salarms.db", exe_path);
    }
    return db_path_buffer;
}

sqlite3* db_open(void) {
    sqlite3 *db;
    const char *db_path = get_db_path();
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    const char *sql = "CREATE TABLE IF NOT EXISTS alarms ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "time TEXT NOT NULL, "
                      "note TEXT, "
                      "is_recurring INTEGER DEFAULT 0, "
                      "task_name TEXT UNIQUE, "
                      "enabled INTEGER DEFAULT 1)";
    char *err = NULL;
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
        sqlite3_close(db);
        return NULL;
    }
    return db;
}

void db_close(sqlite3 *db) {
    if (db) sqlite3_close(db);
}

int alarm_add(sqlite3 *db, const char *time, const char *note, int is_recurring) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO alarms (time, note, is_recurring, task_name) VALUES (?, ?, ?, ?)";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, time, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, note, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, is_recurring);
    sqlite3_bind_text(stmt, 4, "", -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return -1;
    int id = (int)sqlite3_last_insert_rowid(db);
    char task_name[64];
    snprintf(task_name, sizeof(task_name), "AlarmTask_%d", id);
    sql = "UPDATE alarms SET task_name = ? WHERE id = ?";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, task_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return -1;
    return id;
}

int alarm_get(sqlite3 *db, int id, Alarm *a) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, time, note, is_recurring, task_name, enabled FROM alarms WHERE id = ?";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        a->id = sqlite3_column_int(stmt, 0);
        strcpy(a->time, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(a->note, (const char*)sqlite3_column_text(stmt, 2));
        a->is_recurring = sqlite3_column_int(stmt, 3);
        strcpy(a->task_name, (const char*)sqlite3_column_text(stmt, 4));
        a->enabled = sqlite3_column_int(stmt, 5);
        sqlite3_finalize(stmt);
        return 1;
    }
    sqlite3_finalize(stmt);
    return 0;
}

Alarm* alarm_get_all(sqlite3 *db, int *count) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, time, note, is_recurring, task_name, enabled FROM alarms ORDER BY id";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *count = 0;
        return NULL;
    }
    Alarm *list = NULL;
    int n = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Alarm a;
        a.id = sqlite3_column_int(stmt, 0);
        strcpy(a.time, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(a.note, (const char*)sqlite3_column_text(stmt, 2));
        a.is_recurring = sqlite3_column_int(stmt, 3);
        strcpy(a.task_name, (const char*)sqlite3_column_text(stmt, 4));
        a.enabled = sqlite3_column_int(stmt, 5);
        list = realloc(list, (n+1)*sizeof(Alarm));
        if (!list) {
            sqlite3_finalize(stmt);
            *count = 0;
            return NULL;
        }
        list[n++] = a;
    }
    sqlite3_finalize(stmt);
    *count = n;
    return list;
}

int alarm_update(sqlite3 *db, int id, const char *time, const char *note, int is_recurring) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE alarms SET time = ?, note = ?, is_recurring = ? WHERE id = ?";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, time, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, note, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, is_recurring);
    sqlite3_bind_int(stmt, 4, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 1 : 0;
}

int alarm_delete(sqlite3 *db, int id) {
    Alarm a;
    if (!alarm_get(db, id, &a)) return 0;
    scheduler_delete_task(a.task_name);
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM alarms WHERE id = ?";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 1 : 0;
}

void adjust_date_for_today(char *date_str, const char *time_str) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    int h, m;
    sscanf(time_str, "%d:%d", &h, &m);

    struct tm tm_today = *tm_now;
    tm_today.tm_hour = h;
    tm_today.tm_min = m;
    tm_today.tm_sec = 0;
    time_t t_today = mktime(&tm_today);

    if (t_today <= now) {
        tm_now->tm_mday += 1;
        mktime(tm_now);
    }

    // Формат DD.MM.YYYY (принимается русской локалью)
    strftime(date_str, 12, "%d.%m.%Y", tm_now);
}

int scheduler_create_task(const Alarm *a) {
    char cmd[2048];
    char date_str[12]; // достаточно для "MM/DD/YYYY\0"
    char exe_path[MAX_PATH];
    char trigger_full_path[MAX_PATH + 20];

    GetModuleFileName(NULL, exe_path, MAX_PATH);
    char *last_slash = strrchr(exe_path, '\\');
    if (last_slash) *(last_slash + 1) = '\0';

    // Весь путь с аргументом в одних кавычках
    snprintf(trigger_full_path, sizeof(trigger_full_path), "\"%strigger.exe %d\"", exe_path, a->id);

    if (a->is_recurring) {
        snprintf(cmd, sizeof(cmd),
                 "%s /create /tn \"%s\" /tr %s /sc daily /st %s /f",
                 SCHTASKS_CMD, a->task_name, trigger_full_path, a->time);
    } else {
        adjust_date_for_today(date_str, a->time);
        snprintf(cmd, sizeof(cmd),
                 "%s /create /tn \"%s\" /tr %s /sc once /st %s /sd %s /f",
                 SCHTASKS_CMD, a->task_name, trigger_full_path, a->time, date_str);
    }
    printf("Executing: %s\n", cmd); // для отладки
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Failed to create scheduled task (return code %d): %s\n", ret, cmd);
    }
    return ret;
}

int scheduler_delete_task(const char *task_name) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s /delete /tn \"%s\" /f", SCHTASKS_CMD, task_name);
    return system(cmd);
}

int scheduler_update_task(const Alarm *a) {
    scheduler_delete_task(a->task_name);
    return scheduler_create_task(a);
}