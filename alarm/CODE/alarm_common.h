#ifndef ALARM_COMMON_H
#define ALARM_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include <windows.h>   

typedef struct {
    int id;
    char time[6];           
    char note[256];
    int is_recurring;      
    char task_name[64];    
    int enabled;
} Alarm;


sqlite3* db_open(void);

// Закрыть БД
void db_close(sqlite3 *db);


int alarm_add(sqlite3 *db, const char *time, const char *note, int is_recurring);


int alarm_get(sqlite3 *db, int id, Alarm *a);

Alarm* alarm_get_all(sqlite3 *db, int *count);


int alarm_update(sqlite3 *db, int id, const char *time, const char *note, int is_recurring);

int alarm_delete(sqlite3 *db, int id);


int scheduler_create_task(const Alarm *a);

int scheduler_delete_task(const char *task_name);

int scheduler_update_task(const Alarm *a);


void adjust_date_for_today(char *date_str, const char *time_str);

const char* get_db_path(void);

#endif