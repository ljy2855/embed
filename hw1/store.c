#include "store.h"

static table memory_table[MEMORY_TABLE_SIZE];
static int table_index;
static int memory_index;
static storage_list meta_list = {NULL, NULL};

static void load_storage();
static table search_storage(FILE *fp, const int search_key);
int compare_key(const void *a, const void *b);
static void remove_duplicates(table *temp, int *table_cnt);
static void sort_by_key(table *temp, int table_cnt);
static void reindex(table *temp, int table_cnt);

static void insert_node(storage_list *list, FILE *fp, int index, char *filename);
static int list_length(storage_list *list);
static void popleft(storage_list *list);

// KVS interface
void init()
{
    load_storage();
    memory_index = 0;
    table_index = 1;
}

void put_pair(int key, char *value)
{
    assert(memory_index < 3);

    memory_table[memory_index].index = table_index++;
    memory_table[memory_index].key = key;
    strncpy(memory_table[memory_index].value, value, VALUE_BUFFER_SIZE);

    memory_index++;

    if (memory_index == MEMORY_TABLE_SIZE)
    {
        flush();
    }
}

void flush()
{
    FILE *fp;
    int i;
    char filename[20];

    int storage_index = 1;

    if (meta_list.tail)
        storage_index = meta_list.tail->index + 1;
    sprintf(filename, "%d.sst", storage_index);
    fp = fopen(filename, "w+");
    assert(fp != NULL);

    for (i = 0; i < MEMORY_TABLE_SIZE; i++)
    {
        fprintf(fp, "%d %d %s\n", memory_table[i].index, memory_table[i].key, memory_table[i].value);
    }
    memory_index = 0;
    insert_node(&meta_list, fp, storage_index, filename);
}

table get_pair(int key)
{
    int i;
    char filename[20];
    table target;
    storage_meta *cur;

    // search in memory table
    for (i = memory_index - 1; i >= 0; i--)
    {
        if (memory_table[i].key == key)
        {
            target.index = memory_table[i].index;
            target.key = memory_table[i].key;
            strncpy(target.value, memory_table[i].value, VALUE_BUFFER_SIZE);
            return target;
        }
    }

    // search in storage table
    for (cur = meta_list.tail; cur != NULL; cur = cur->prev)
    {
        target = search_storage(cur->fp, key);
        if (target.index != NOT_FOUND)
            return target;
    }
    return target;
}

// TODO background running job
void merge()
{
    table temp[MAX_SIZE];
    storage_meta *cur;
    int storage_cnt = 0;
    int table_cnt = 0;
    int storage_index = 1;
    char new_filename[20];
    FILE *fp;
    int i;

    // get table from storage
    for (cur = meta_list.head; cur != NULL && storage_cnt != MERGE_TABLE_SIZE; cur = cur->next, storage_cnt++)
    {
        fsetpos(cur->fp, 0);
        while (fscanf(cur->fp, "%d %d %s", &temp[table_cnt].index, &temp[table_cnt].key, temp[table_cnt].value) == 3)
        {
            table_cnt++;
        }
    }

    // process merge
    sort_by_key(temp, table_cnt);
    remove_duplicates(temp, &table_cnt);
    reindex(temp, table_cnt);

    // append new storage table
    if (meta_list.tail)
        storage_index = meta_list.tail->index + 1;
    sprintf(new_filename, "%d.sst", storage_index);
    fp = fopen(new_filename, "w+");
    assert(fp != NULL);

    for (i = 0; i < table_cnt; i++)
    {
        fprintf(fp, "%d %d %s\n", temp[i].index, temp[i].key, temp[i].value);
    }
    insert_node(&meta_list, fp, storage_index, new_filename);

    // remove merged storage table from meta_list
    while (storage_cnt--)
        popleft(&meta_list);
}

// helpers
static void load_storage()
{
    DIR *dir;
    struct dirent *entry;
    int index;

    dir = opendir(".");
    assert(dir != NULL);

    while ((entry = readdir(dir)) != NULL)
    {
        if (strstr(entry->d_name, ".sst") != NULL)
        {
            sscanf(entry->d_name, "%d", &index);
            insert_node(&meta_list, fopen(entry->d_name, "r+"), index, entry->d_name);
            printf("Found .sst file: %s\n", entry->d_name);
        }
    }

    closedir(dir);
}

static table search_storage(FILE *fp, const int search_key)
{

    char buffer[1024];
    long pos;
    char *token;
    table result = {NOT_FOUND, 0, ""};

    fseek(fp, 0, SEEK_END);
    pos = ftell(fp);

    while (pos > 0)
    {
        long lineStart = --pos;

        while (pos > 0)
        {
            fseek(fp, --pos, SEEK_SET);
            if (fgetc(fp) == '\n')
            {
                lineStart = pos + 1;
                break;
            }
        }

        fseek(fp, lineStart, SEEK_SET);
        if (fgets(buffer, sizeof(buffer), fp) == NULL)
            break;

        token = strtok(buffer, " ");
        if (token != NULL)
        {
            int index = atoi(token);
            token = strtok(NULL, " ");
            int key = atoi(token);
            if (key == search_key)
            {
                result.index = index;
                result.key = key;
                strncpy(result.value, strtok(NULL, " "), VALUE_BUFFER_SIZE);
                break;
            }
        }

        fseek(fp, lineStart - 1, SEEK_SET);
    }

    return result;
}

int compare_key(const void *a, const void *b)
{
    table *tableA = (table *)a;
    table *tableB = (table *)b;
    return (tableA->key - tableB->key);
}

static void remove_duplicates(table *temp, int *table_cnt)
{
    int write_pos = 0;
    int i;
    for (i = 0; i < *table_cnt; ++i)
    {

        if (i + 1 < *table_cnt && temp[i].key == temp[i + 1].key)
        {
            continue;
        }
        temp[write_pos++] = temp[i];
    }
    *table_cnt = write_pos;
}

static void sort_by_key(table *temp, int table_cnt)
{
    qsort(temp, table_cnt, sizeof(table), compare_key);
}

static void reindex(table *temp, int table_cnt)
{
    int i;
    for (i = 0; i < table_cnt; i++)
    {
        temp[i].index = i + 1;
    }
}

// List structure method

static void insert_node(storage_list *list, FILE *fp, int index, char *filename)
{
    storage_meta *newNode = (storage_meta *)malloc(sizeof(storage_meta));
    newNode->fp = fp;
    newNode->index = index;
    newNode->next = NULL;
    strcpy(newNode->filename, filename);

    if (list->tail == NULL)
    {
        newNode->prev = NULL;
        list->head = newNode;
        list->tail = newNode;
    }
    else
    {
        list->tail->next = newNode;
        newNode->prev = list->tail;
        list->tail = newNode;
    }
}

static int list_length(storage_list *list)
{
    storage_meta *temp = list->head;
    int cnt = 0;

    while (temp != NULL)
    {
        temp = temp->next;
        cnt++;
    }
    return cnt;
}

static void popleft(storage_list *list)
{
    if (list->head == NULL)
    {
        return;
    }

    storage_meta *temp = list->head;

    list->head = list->head->next;

    if (list->head)
    {

        list->head->prev = NULL;
    }
    else
    {

        list->tail = NULL;
    }

    if (temp->fp)
    {
        fclose(temp->fp);
        remove(temp->filename);
    }
    free(temp);
}
