#define MEMORY_TABLE_SIZE 3
#define VALUE_BUFFER_SIZE 6
#define NOT_FOUND -1
#define MAX_SIZE 1000
#define MERGE_TABLE_SIZE 2

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

typedef struct table
{
    int index;
    int key;
    char value[VALUE_BUFFER_SIZE];

} table;

typedef struct storage_meta
{
    FILE *fp;
    int index;
    char filename[20];
    struct storage_meta *prev;
    struct storage_meta *next;
} storage_meta;

typedef struct storage_list
{
    storage_meta *head;
    storage_meta *tail;
} storage_list;

void init_store();
table get_pair(int key);
void merge();
void flush();
void put_pair(int key, char *value);