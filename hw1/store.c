#include "store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static table memory_table[MEMORY_TABLE_SIZE];
static int table_index;
static int memory_index;
static storage_list *meta_list;

static void load_storage();
static table search_storage(FILE *fp, const int search_key);
int compare_key(const void *a, const void *b);
static void remove_duplicates(table *temp, int *table_cnt);
static void sort_by_key(table *temp, int table_cnt);
static void reindex(table *temp, int table_cnt);

static void insert_node(storage_list *list, int index, char *filename);
static int list_length(storage_list *list);
static void popleft(storage_list *list);

#define META_LIST_CNT 20
const char *shm_name = "/my_shared_memory";
const int shm_size = sizeof(storage_list) + META_LIST_CNT * sizeof(storage_meta);

storage_list *init_shared_memory()
{
    int shm_fd = shm_open(shm_name, O_RDWR, 0666);

    // 이미 존재하는 공유 메모리 객체가 있다면 삭제
    if (shm_fd != -1)
    {
        // Close and unlink if already exists
        close(shm_fd);
        shm_unlink(shm_name);
    }

    // 공유 메모리 객체 재생성
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, shm_size) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *mapped = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mapped == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // 메모리 초기화
    memset(mapped, 0, shm_size);

    return (storage_list *)mapped;
}

// KVS interface
void init_store()
{
    meta_list = init_shared_memory();
    meta_list->index = 1;
    load_storage();
    memory_index = 0;
    table_index = 1;
    print_list();
}

int put_pair(int key, char *value)
{
    assert(memory_index < 3);
    int index = table_index++;
    memory_table[memory_index].index = index;
    memory_table[memory_index].key = key;
    strncpy(memory_table[memory_index].value, value, VALUE_BUFFER_SIZE);

    memory_index++;

    if (memory_index == MEMORY_TABLE_SIZE)
    {
        flush();
    }
    return index;
}

void flush()
{
    if (memory_index == 0)
        return;
    FILE *fp;
    int i;
    char filename[20];

    int storage_index = meta_list->index++;

    sprintf(filename, "%d.sst", storage_index);
    fp = fopen(filename, "w+");
    assert(fp != NULL);

    for (i = 0; i < memory_index; i++)
    {
        fprintf(fp, "%d %d %s\n", memory_table[i].index, memory_table[i].key, memory_table[i].value);
    }
    memory_index = 0;
    insert_node(meta_list, storage_index, filename);
    fclose(fp);
}

table get_pair(int key)
{
    int i;
    char filename[20];
    table target;
    storage_meta *cur;
    FILE *fp;

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
    for (cur = meta_list->tail; cur != NULL; cur = cur->prev)
    {
        fp = fopen(cur->filename, "r");
        target = search_storage(fp, key);
        fclose(fp);
        if (target.index != NOT_FOUND)
            return target;
    }
    return target;
}

// TODO background running job
merge_result merge()
{
    table temp[MAX_SIZE];
    storage_meta *cur;
    int storage_cnt = 0;
    int table_cnt = 0;
    int storage_index = 1;
    int order;
    char new_filename[20];
    FILE *fp;
    int i;
    merge_result result;

    order = meta_list->head->index;
    // get table from storage
    for (cur = meta_list->head; storage_cnt != MERGE_TABLE_SIZE; storage_cnt++)
    {
        fp = fopen(cur->filename, "r");
        while (fscanf(fp, "%d %d %s", &temp[table_cnt].index, &temp[table_cnt].key, temp[table_cnt].value) == 3)
        {
            printf("%d %d %s\n", temp[table_cnt].index, temp[table_cnt].key, temp[table_cnt].value);
            table_cnt++;
        }
        fclose(fp);
        cur = cur->next;

        popleft(meta_list);
    }

    // process merge
    sort_by_key(temp, table_cnt);
    remove_duplicates(temp, &table_cnt);
    reindex(temp, table_cnt);

    // append new storage table

    storage_index = meta_list->index++;
    sprintf(new_filename, "%d.sst", storage_index);
    fp = fopen(new_filename, "w+");
    assert(fp != NULL);

    for (i = 0; i < table_cnt; i++)
    {
        printf("%d %d %s\n", temp[i].index, temp[i].key, temp[i].value);
        fprintf(fp, "%d %d %s\n", temp[i].index, temp[i].key, temp[i].value);
    }

    // remove merged storage table from meta_list

    insert_node(meta_list, order, new_filename);
    // store result
    result.cnt = table_cnt;
    strcpy(result.filename, new_filename);
    printf("new merge file: %s index: %d\n", new_filename, order);
    print_list();
    fclose(fp);
    return result;
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
            if (index > meta_list->index)
                meta_list->index = index + 1;
            insert_node(meta_list, index, entry->d_name);
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

    // 파일의 끝으로 이동하여 전체 크기를 측정
    fseek(fp, 0, SEEK_END);
    long last_pos = ftell(fp); // 파일 크기를 저장

    // 파일의 끝에서 한 줄씩 역순으로 읽기 시작
    pos = last_pos;
    while (pos > 0)
    {
        long line_start = pos;

        // 줄의 시작 위치를 찾기
        while (pos > 0)
        {
            fseek(fp, --pos, SEEK_SET);
            char c = fgetc(fp);
            if (c == '\n' && pos != last_pos - 1)
            { // 파일 끝 바로 다음 개행 제외
                line_start = pos + 1;
                break;
            }
        }

        // 파일 시작 위치에 도달하면, 줄 시작을 파일 시작으로 설정
        if (pos == 0)
        {
            fseek(fp, 0, SEEK_SET);
        }
        else
        {
            fseek(fp, line_start, SEEK_SET);
        }

        if (fgets(buffer, sizeof(buffer), fp) == NULL)
            break; // 파일 시작에 도달하거나 읽기 오류 발생

        // 버퍼에서 데이터 토큰화
        token = strtok(buffer, " ");
        if (token != NULL)
        {
            int index = atoi(token);
            token = strtok(NULL, " ");
            if (token != NULL)
            {
                int key = atoi(token);
                if (key == search_key)
                {
                    result.index = index;
                    result.key = key;
                    token = strtok(NULL, " ");
                    if (token)
                    {
                        token[strlen(token) - 1] = 0;
                        strncpy(result.value, token, VALUE_BUFFER_SIZE - 1);
                        result.value[VALUE_BUFFER_SIZE - 1] = '\0'; // 널 문자 보장
                    }
                    break;
                }
            }
        }

        // 다음 줄을 위해 위치 갱신
        pos = line_start - 1;
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

static void insert_node(storage_list *list, int index, char *filename)
{
    storage_meta *newNode = (storage_meta *)(list + 1); // 가정: storage_meta 배열이 storage_list 바로 뒤에 위치
    int i;
    for (i = 0; i < META_LIST_CNT; i++)
    {
        if (newNode[i].index == 0)
        { // 미사용 노드 찾기
            newNode[i].index = index;
            strcpy(newNode[i].filename, filename);
            newNode[i].next = newNode[i].prev = NULL;

            if (list->head == NULL)
            {
                list->head = list->tail = &newNode[i];
            }
            else
            {
                storage_meta *current = list->head, *prev = NULL;
                while (current != NULL && current->index < index)
                {
                    prev = current;
                    current = current->next;
                }

                if (current == list->head)
                {
                    newNode[i].next = list->head;
                    list->head->prev = &newNode[i];
                    list->head = &newNode[i];
                }
                else if (current == NULL)
                {
                    list->tail->next = &newNode[i];
                    newNode[i].prev = list->tail;
                    list->tail = &newNode[i];
                }
                else
                {
                    newNode[i].next = current;
                    newNode[i].prev = current->prev;
                    current->prev = &newNode[i];
                    if (prev)
                    {
                        prev->next = &newNode[i];
                    }
                }
            }
            break;
        }
    }
}

int storage_cnt()
{
    storage_meta *temp = meta_list->head;
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
        return; // 리스트가 비어있는 경우
    }

    storage_meta *temp = list->head;
    printf("remove file : %s index: %d\n", temp->filename, temp->index);
    // 리스트의 head를 다음 노드로 이동
    list->head = list->head->next;

    if (list->head)
    {
        list->head->prev = NULL; // 새 head의 prev를 NULL로 설정
    }
    else
    {
        list->tail = NULL; // 리스트가 비었으면 tail도 NULL로 설정
    }

    // 파일 관련 리소스 정리
    remove(temp->filename);

    // 파일 삭제

    // 노드 초기화 및 사용하지 않는 것으로 표시
    temp->index = 0; // index를 0으로 설정하여 미사용으로 표시
    temp->next = NULL;
    temp->prev = NULL;
    memset(temp->filename, 0, sizeof(temp->filename)); // 파일 이름 초기화
}

void print_list()
{
    storage_meta *current = meta_list->head;
    printf("List Contents:\n");
    while (current != NULL)
    {
        printf("Index: %d, Filename: %s\n", current->index, current->filename);
        current = current->next;
    }
}