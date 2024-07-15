#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECORDS 100

typedef struct {
    int id;
    char name[50];
    int age;
} Record;

Record database[MAX_RECORDS];
int record_count = 0;

void insert_record(int id, const char* name, int age);
void update_record(int id, const char* name, int age);
void delete_record(int id);

void insert_record(int id, const char* name, int age) {
    if (record_count < MAX_RECORDS) {
        database[record_count].id = id;
        strncpy(database[record_count].name, name, sizeof(database[record_count].name) - 1);
        database[record_count].name[sizeof(database[record_count].name) - 1] = '\0';
        database[record_count].age = age;
        record_count++;
    }
}

void update_record(int id, const char* name, int age) {
    for (int i = 0; i < record_count; i++) {
        if (database[i].id == id) {
            strncpy(database[i].name, name, sizeof(database[i].name) - 1);
            database[i].name[sizeof(database[i].name) - 1] = '\0';
            database[i].age = age;
            break;
        }
    }
}

void delete_record(int id) {
    for (int i = 0; i < record_count; i++) {
        if (database[i].id == id) {
            for (int j = i; j < record_count - 1; j++) {
                database[j] = database[j + 1];
            }
            record_count--;
            break;
        }
    }
}
