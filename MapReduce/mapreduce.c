#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "mapreduce.h"

//The node store values of a specific key
typedef struct value_node{
    char* val;
    struct value_node* next;
}value_node;

//The node of linked list for storing data
typedef struct Node {
    char* key;
    struct value_node* value;
    struct Node* next;
} Node;

//The node of the first node of each combine/partition
typedef struct NodeWrapper{
    struct Node* element;
}NodeWrapper;

//The node of free list to store values
typedef struct free_str{
    struct Node *value;
    struct free_str *next;
}free_str;

//global fields for future use
Node* header_of_cur_mapper;
Node* header_of_reduce;
pthread_t* cur_map_tid;
pthread_t* tid_of_cur_reducer;

Partitioner num_of_partitions;
int number_of_mappers;
int number_of_reducer;
Mapper mapper_thread;
Combiner combine_thread;
Reducer reduce_thread;
char** argument_threads;
int argument_num;

sem_t* sem_mutex;
NodeWrapper free_list_wrapper;
free_str free_list_value;
sem_t* locker_for_value;
sem_t locker_for_wrapper;

//comparator function for sort linked list use
int comparator(const void *p, const void *q)
{
    const struct value_node * a = (const struct value_node *) p;
    const struct value_node * b = (const struct value_node *) q;
    if(strcmp(a -> val, b -> val) < 0) {
      return -1;
    } else if(strcmp(a -> val, b -> val) > 0) {
      return 1;
    } else {
      return 0;
    }
}

//the function is to build a free list for value nodes.
void build_free_list(char* value) {
    NodeWrapper* header = &free_list_wrapper;
    Node *head = header->element;
    Node* temp = malloc(sizeof(Node));
    temp->key = value;
    temp->next = head->next;
    temp->value = NULL;
    head->next = temp;
}

//this is the function to clear the built value free_list
void clear_free_list() {
    Node* head = free_list_wrapper.element->next;
    Node* temp;
    while (head) {
        temp = head;
        head = head->next;
        free(temp->key);
        free(temp);
    }
    free(free_list_wrapper.element);
}

//this is the function to build the key free_list
void build_head_free_list(Node* value) {
    free_str* head = &free_list_value;
    free_str* temp = malloc(sizeof(free_str));
    //printf("ggggggggg\n");
    temp->next = head->next;
    temp->value = value;
    head->next = temp;
}

//this is the function to clear the built head free-list
void clear_head_free_list() {
    free_str* head = free_list_value.next;
    free_str* temp;
    while (head) {
        temp = head;
        head = head->next;
        free(temp->value);
        free(temp);
    }
}

//insert node into the linked list
void ll_insert(Node* head, char* key, char* value) {
     if (head == NULL) {
        return;
     }
     value_node* temp = malloc(sizeof(value_node));
     //printf("%s\n", temp->key);
     char* tmp_val = strdup(value);
     temp->val = tmp_val;
     temp->next = NULL;
     Node* search_node = head->next;
     while (search_node && strcmp(search_node->key, key) != 0) {
       search_node = search_node->next;
     }
     if (search_node == NULL) {

      Node* newNode = malloc(sizeof(Node));
      newNode->next = head->next;
      newNode->key = strdup(key);
      newNode->value = temp;
      //printf("77777777777777");
      head->next = newNode;
      return;
     }

     temp->next = search_node->value;
     search_node->value = temp;
}

void free_cur_node(Node* head) {
    Node* cur = head;
    cur = cur->next;
    Node* temp2;
    while(cur != NULL){
        value_node *curValue = cur->value;
        value_node *temp;
        while(curValue != NULL){
            temp = curValue;
            curValue = curValue->next;
            free(temp);
        }
        temp2 = cur;
        cur = cur->next;
        if(temp2->key != NULL){
            free(temp2->key);
        }
        free(temp2);
    }
}

pthread_t map_thread_id(int check) {
    pthread_t pid = pthread_self();
    int index = 0;
    if(check){
        while (cur_map_tid[index] != pid && index <= number_of_mappers) {
          index++;
        }
    }
    if (index > number_of_mappers) {
        return -1;
    }else{
      return index;
    }
}

char* get_next_for_combine(char* key) {
    int hash_check = (97 % number_of_mappers) + 1;
    int index = map_thread_id(hash_check);

    Node* head = header_of_cur_mapper + index;
    Node* head_r = head;
    head = head->next;
    while (head && head->key && strcmp(head->key, key) != 0) head = head->next;
    if (head != NULL && head->value != NULL) {
        value_node* temp = head->value;
        char* val = temp->val;
        head->value = head->value->next;
        sem_wait(locker_for_value);
        build_free_list(val);
        sem_post(locker_for_value);
        free(temp);
        if (head->value == NULL) {
            // to remove
            while (head_r && head_r->next && strcmp(head_r->next->key, key) != 0)
                head_r = head_r->next;

            head = head_r->next;
            sem_wait(locker_for_value);
            build_free_list(head->key);
            sem_post(locker_for_value);
            head_r->next = head_r->next->next;
            build_head_free_list(head);
        }
        return val;
    }
    return NULL;
}

char* get_next_for_reducer(char* key, int partition_number) {
    Node *head = header_of_reduce + partition_number;
    Node* head_r = head;
    head = head->next;
    while (head && head->key && strcmp(head->key, key) != 0) head = head->next;

    if (head && head->value) {
        value_node* temp = head->value;
        char* val = temp->val;
        head->value = head->value->next;
        sem_wait(locker_for_value);
        build_free_list(val);
        sem_post(locker_for_value);
        free(temp);

        if (!head->value) {
            // to remove
            while (head_r && head_r->next && strcmp(head_r->next->key, key) != 0)
                head_r = head_r->next;

            head = head_r->next;
            sem_wait(locker_for_value);
            build_free_list(head->key);
            sem_post(locker_for_value);
            head_r->next = head_r->next->next;
            build_head_free_list(head);
        }
        return val;
    }
    return NULL;
}

void MR_EmitToCombiner(char *key, char *value) {
    int hash_check = (97 % number_of_mappers) + 1;
    int index = map_thread_id(hash_check);
    Node* head = header_of_cur_mapper + index;
    if (!head)
        return;
    value_node* temp = malloc(sizeof(value_node));
    temp->val = strdup(value);
    temp->next = NULL;

    Node* search_node = head->next;
    while (search_node && strcmp(search_node->key, key) != 0) search_node = search_node->next;

    if (search_node == NULL) {
        Node* newNode = malloc(sizeof(Node));
        newNode->next = head->next;
        newNode->key = strdup(key);
        newNode->value = temp;
        head->next = newNode;
        return;
    }

    temp->next = search_node->value;
    search_node->value = temp;
}

void MR_EmitToReducer(char *key, char *value) {
    int index = num_of_partitions(key, number_of_reducer);
    sem_wait(sem_mutex + index);
    ll_insert(header_of_reduce + index, key, value);
    sem_post(sem_mutex + index);
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

void reduce_wrapper(int* index) {
    int loca = *index;
    sem_post(&locker_for_wrapper);
    Node* head = header_of_reduce[loca].next;
    while (head) {
        reduce_thread(head->key, NULL, get_next_for_reducer, loca);
        head = head->next;
    }
}

void mapper_wrapper(int* index) {
    // This is the function called by the map threads
    // We will invoke the Mapper here
    //
    // After Mapper is done invoke the Combiner etc.
    int loca = *index;
    int loca_r = loca;
    sem_post(&locker_for_wrapper);
    while (loca < argument_num) {
        mapper_thread(argument_threads[loca]);
        loca += number_of_mappers;
    }
    if (combine_thread != NULL) {
        Node* head = header_of_cur_mapper[loca_r].next;
        while (loca_r < argument_num && head) {
            combine_thread(head->key, get_next_for_combine);
            head = head->next;
        }
    }
}

void task_division(int *tasks_per_wrapper,int num_wrapper, int file_num) {
    for (int i = 0; i < num_wrapper; ++i) {
        tasks_per_wrapper[i] = file_num / num_wrapper;
        if (i < file_num % num_wrapper) {
            tasks_per_wrapper[i]++;
        }
    }
}

void MR_Run(int argc, char *argv[],
    Mapper map, int num_mappers,
    Reducer reduce, int num_reducers,
    Combiner combine,
    Partitioner partition) {
    //step1: Initialize fields
    locker_for_value = malloc(sizeof(sem_t));
    sem_init(locker_for_value, 0, 1);

    free_list_wrapper.element = malloc(sizeof(Node));
    free_list_wrapper.element->next = NULL;
    free_list_wrapper.element->key = NULL;
    free_list_wrapper.element->value = NULL;
    free_list_value.value = NULL;
    free_list_value.next = NULL;

    num_of_partitions = partition;
    number_of_mappers = num_mappers;
    number_of_reducer = num_reducers;
    reduce_thread = reduce;
    mapper_thread = map;
    combine_thread = combine;
    argument_threads = argv;
    argument_num = argc;
    //malloc the firds for future use
    cur_map_tid = malloc((num_mappers + 6) * sizeof(pthread_t));
    tid_of_cur_reducer = malloc((num_reducers + 6) * sizeof(pthread_t));
    header_of_cur_mapper = malloc((num_mappers + 6) * sizeof(Node));
    header_of_reduce = malloc((num_reducers + 6) * sizeof(Node));
    sem_mutex = malloc((number_of_reducer + 6) * sizeof(sem_t));

    for (int i = 0; i <= number_of_mappers; i++) {
        header_of_cur_mapper[i].next = NULL;
        header_of_cur_mapper[i].key = NULL;
        header_of_cur_mapper[i].value = NULL;
    }

    //printf("aaaaaaaaaaaaaaaa");
    //printf("%d\n", cur_map_tid[i]);
    for (int i = 0; i <= number_of_mappers; i++) {
        cur_map_tid[i] = 0;
    }
    for (int i = 0; i <= number_of_reducer; i++) {
        header_of_reduce[i].next = NULL;
        header_of_reduce[i].key = NULL;
        header_of_reduce[i].value = NULL;
        tid_of_cur_reducer[i] = 0;
        if(i < number_of_reducer){
            sem_init(sem_mutex + i, 0, 1);
        }
    }

    // step2: Map
    int num_files_per_mapper[num_mappers];
    task_division(num_files_per_mapper, num_mappers, argc - 1);
    int* file_per_map = malloc((num_mappers + 5) * sizeof(int));

    for (int i = 0; i <= num_mappers; i++) {
        file_per_map[i] = i;
    }
    //printf("77777777777777");
    sem_init(&locker_for_wrapper, 0, 1);
    for (int i = 1; i <= num_mappers; i++) {
        sem_wait(&locker_for_wrapper);
        pthread_create(&cur_map_tid[i], NULL, (void*)mapper_wrapper, (void*)(file_per_map + i));
    }
    //printf("6666666666666");
    for (int i = 1; i <= num_mappers; i++) {
        pthread_join(cur_map_tid[i], NULL);
    }
    //printf("done here");
    for (int i = 1; i <= num_mappers; i++) {
        free_cur_node(header_of_cur_mapper + i);
    }
    int* keys = malloc(num_reducers * sizeof(int));

    for (int i = 0; i < number_of_reducer; i++) {
        keys[i] = i;
    }

    // step3: Reduce
    for (int i = 0; i < num_reducers; i++) {
        sem_wait(&locker_for_wrapper);
        pthread_create(tid_of_cur_reducer + i, NULL, (void*)reduce_wrapper, (void*)(keys + i));
    }

    for (int i = 0; i < num_reducers; i++) {
        pthread_join(tid_of_cur_reducer[i], NULL);
    }
    for (int i = 0; i < num_reducers; i++) {
        free_cur_node(header_of_reduce + i);
    }

    //step4: free
    free(file_per_map);
    free(cur_map_tid);
    free(header_of_reduce);
    clear_free_list();
    clear_head_free_list();
    free(sem_mutex);
    free(locker_for_value);
    free(header_of_cur_mapper);
    free(keys);
    free(tid_of_cur_reducer);
}
//finish
