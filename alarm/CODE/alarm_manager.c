#include "alarm_common.h"
#include <windows.h>
#include <conio.h>

void read_line(char *buffer, int size) {
    fgets(buffer, size, stdin);
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] != '\n') {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
    } else if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
}

void print_alarm(const Alarm *a) {
    printf("ID: %d | %s | %s | %s | %s\n",
           a->id, a->time, a->note,
           a->is_recurring ? "Daily" : "Once",
           a->enabled ? "On" : "Off");
}

int main() {
    sqlite3 *db = db_open();
    if (!db) return 1;
    int choice;
    do {
        printf("\n=== Alarm Manager ===\n");
        printf("1. Add alarm\n");
        printf("2. List alarms\n");
        printf("3. Delete alarm\n");
        printf("4. Start timer (separate process)\n");
        printf("5. Exit\n");
        printf("Choose: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1: {
                char time[6], note[256];
                int recurring;
                printf("Time (HH:MM): ");
                read_line(time, sizeof(time));
                printf("Note: ");
                read_line(note, sizeof(note));
                printf("Type (0 - once, 1 - daily): ");
                scanf("%d", &recurring);
                getchar();
                int id = alarm_add(db, time, note, recurring);
                if (id < 0) {
                    printf("Error adding to DB.\n");
                } else {
                    Alarm a;
                    alarm_get(db, id, &a);
                    if (scheduler_create_task(&a) == 0) {
                        printf("Alarm added (ID=%d).\n", id);
                    } else {
                        alarm_delete(db, id);
                        printf("Error creating scheduled task, alarm removed.\n");
                    }
                }
                break;
            }
            case 2: {
                int count;
                Alarm *list = alarm_get_all(db, &count);
                if (count == 0) {
                    printf("No alarms.\n");
                } else {
                    for (int i = 0; i < count; i++) {
                        print_alarm(&list[i]);
                    }
                    free(list);
                }
                break;
            }
            case 3: {
                int id;
                printf("Enter alarm ID to delete: ");
                scanf("%d", &id);
                getchar();
                if (alarm_delete(db, id)) {
                    printf("Alarm deleted.\n");
                } else {
                    printf("Not found.\n");
                }
                break;
            }
            case 4: {
                char time_str[10];
                printf("Enter timer duration (HH:MM or minutes): ");
                read_line(time_str, sizeof(time_str));
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "timer.exe %s", time_str);
                STARTUPINFO si = { sizeof(si) };
                PROCESS_INFORMATION pi;
                if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                    printf("Timer started (PID=%lu).\n", (unsigned long)pi.dwProcessId);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                } else {
                    printf("Failed to start timer.exe.\n");
                }
                break;
            }
            case 5:
                printf("Exit.\n");
                break;
            default:
                printf("Invalid choice.\n");
        }
    } while (choice != 5);
    db_close(db);
    return 0;
}