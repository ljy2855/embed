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
    int index;
    char filename[20];
    struct storage_meta *prev;
    struct storage_meta *next;
} storage_meta;

typedef struct storage_list
{
    int index;
    storage_meta *head;
    storage_meta *tail;
} storage_list;

typedef struct merge_result
{
    int cnt;
    char filename[20];
} merge_result;

/**
 * initalize key value store and load exist storage table
 */
void init_store();
/**
 * search key value pair with key by recently
 */
table get_pair(int key);
/**
 * merge storage table
 */
merge_result merge();
/**
 * export memory table to storage table
 */
void flush();
/**
 * insert new key value pair in memory table
 */
int put_pair(int key, char *value);
/**
 * get current storage table count
 */
int storage_cnt();
/**
 * print storage tables meta data list
 */
void print_list();