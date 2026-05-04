/*
*  ChatFeliPThread - data_structures.h
*  Felipe Muggiati Feldman
 */

#ifndef HW07_DATA_STRUCTURES_H
#define HW07_DATA_STRUCTURES_H

#include <pthread.h>

typedef struct user User;
typedef struct vector Vector;
typedef struct queue Queue;
typedef struct message Message;
typedef struct node Node;

struct user {
    int user_fd;
    pthread_t tid;
    char* name;
    Vector* v;
    Queue* q;
};

struct message {
    User* user;
    char* text;
    size_t size;
};

struct node {
    Message message;
    struct node* next;
};

struct queue {
    Node* head;
    Node* tail;
    size_t size;
    pthread_mutex_t lock;
    pthread_cond_t if_empty;
};

struct vector {
    User** data;
    size_t size;
    size_t cap;
    pthread_mutex_t lock;
    pthread_cond_t if_empty;
};

typedef struct {
    Queue* q;
    Vector* v;
} MessageSenderArgs;


void queue_init(Queue* q);
void queue_put(Queue* q, Message msg);
Message queue_get(Queue* q);
void queue_cleanup(Queue* q);
void queue_print(Queue* q);

void vector_init(Vector* v);
void vector_push_back(Vector* v, User* user);
void vector_remove(Vector* v, User* u);
void vector_cleanup(Vector* v);
void vector_display(Vector* v);

#endif //HW07_DATA_STRUCTURES_H