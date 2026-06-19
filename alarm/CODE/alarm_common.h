#ifndef ALARM_COMMON_H
#define ALARM_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include <windows.h>   // для MAX_PATH

typedef struct {
    int id;
    char time[6];           // "HH:MM"
    char note[256];
    int is_recurring;       // 0 - once, 1 - daily
    char task_name[64];     // "AlarmTask_<id>"
    int enabled;
} Alarm;

// Открыть БД (создать таблицу при необходимости)
sqlite3* db_open(void);

// Закрыть БД
void db_close(sqlite3 *db);

// Добавить будильник, возвращает ID или -1 при ошибке
int alarm_add(sqlite3 *db, const char *time, const char *note, int is_recurring);

// Получить будильник по ID, возвращает 1 если найден
int alarm_get(sqlite3 *db, int id, Alarm *a);

// Получить все будильники (возвращает массив, нужно освободить)
Alarm* alarm_get_all(sqlite3 *db, int *count);

// Обновить будильник (по ID), возвращает 1 при успехе
int alarm_update(sqlite3 *db, int id, const char *time, const char *note, int is_recurring);

// Удалить будильник по ID
int alarm_delete(sqlite3 *db, int id);

// Создать задачу в планировщике для данного будильника
int scheduler_create_task(const Alarm *a);

// Удалить задачу планировщика по имени
int scheduler_delete_task(const char *task_name);

// Обновить задачу (удалить старую, создать новую)
int scheduler_update_task(const Alarm *a);

// Проверить, нужно ли перенести дату на завтра (если время прошло)
void adjust_date_for_today(char *date_str, const char *time_str);

// Получить абсолютный путь к файлу БД (на основе расположения .exe)
const char* get_db_path(void);

#endif