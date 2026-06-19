#include "alarm_common.h"
#include <windows.h>
#include <conio.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: trigger.exe <alarm_id>\n");
        return 1;
    }
    int id = atoi(argv[1]);

    FILE *log = fopen("trigger_log.txt", "a");
    if (log) {
        time_t now = time(NULL);
        fprintf(log, "Trigger started with ID %d at %s", id, ctime(&now));
        fclose(log);
    }

    sqlite3 *db = db_open();
    if (!db) {
        fprintf(stderr, "Cannot open DB\n");
        return 1;
    }
    Alarm a;
    if (!alarm_get(db, id, &a)) {
        printf("Alarm with ID %d not found.\n", id);
        db_close(db);
        return 1;
    }

    int is_recurring = a.is_recurring;
    char task_name[64];
    strcpy(task_name, a.task_name);
    db_close(db); 

    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);

    char msg[300];
    snprintf(msg, sizeof(msg), "Alarm! Note: %s", a.note);
    MessageBox(NULL, msg, "Alarm", MB_OK | MB_ICONEXCLAMATION);

    printf("\n\n********** ALARM! **********\n");
    printf("Note: %s\n", a.note);
    printf("Enter 'stop' to exit...\n");

    char input[10];
    int stop = 0;
    while (!stop) {
        Beep(1000, 200);
        Sleep(300);
        if (_kbhit()) {
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;
            if (strcmp(input, "stop") == 0) {
                stop = 1;
            }
        }
    }

    if (!is_recurring) {
        db = db_open();
        if (db) {
            sqlite3_stmt *stmt;
            const char *sql = "DELETE FROM alarms WHERE id = ?";
            int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            if (rc == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, id);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
            db_close(db);
            scheduler_delete_task(task_name);
            printf("One-time alarm (ID %d) deleted.\n", id);
        }
    }

    FreeConsole();
    return 0;
}