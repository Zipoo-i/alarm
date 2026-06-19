#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>

void show_countdown(int total_seconds) {
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = total_seconds % 60;
    while (total_seconds > 0) {
        printf("\rRemaining: %02d:%02d:%02d  ", hours, minutes, seconds);
        fflush(stdout);
        Sleep(1000);
        total_seconds--;
        hours = total_seconds / 3600;
        minutes = (total_seconds % 3600) / 60;
        seconds = total_seconds % 60;
    }
    printf("\rTimer finished!            \n");
}

void wait_for_stop_with_beep() {
    printf("\n********** TIMER FINISHED! **********\n");
    printf("Enter 'stop' to exit...\n");
    char input[10];
    int stop = 0;
    while (!stop) {
        MessageBeep(MB_ICONEXCLAMATION);
        Sleep(500);
        if (_kbhit()) {
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;
            if (strcmp(input, "stop") == 0) {
                stop = 1;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);

    int total_seconds = 0;
    if (argc >= 2) {
        char *arg = argv[1];
        if (strchr(arg, ':') != NULL) {
            int h, m;
            if (sscanf(arg, "%d:%d", &h, &m) == 2) {
                total_seconds = h * 3600 + m * 60;
            }
        } else {
            int mins = atoi(arg);
            total_seconds = mins * 60;
        }
    }

    if (total_seconds <= 0) {
        printf("Enter duration (minutes or HH:MM): ");
        char input[20];
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        if (strchr(input, ':') != NULL) {
            int h, m;
            if (sscanf(input, "%d:%d", &h, &m) == 2) {
                total_seconds = h * 3600 + m * 60;
            }
        } else {
            int mins = atoi(input);
            total_seconds = mins * 60;
        }
        if (total_seconds <= 0) {
            printf("Invalid input. Using default 10 minutes.\n");
            total_seconds = 600;
        }
    }

    printf("Timer started for %d second(s).\n", total_seconds);
    show_countdown(total_seconds);
    MessageBox(NULL, "Timer is up!", "Timer", MB_OK | MB_ICONEXCLAMATION);
    wait_for_stop_with_beep();

    FreeConsole();
    return 0;
}