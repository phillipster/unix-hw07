/*
 *  ChatFeliPThread - data_structures.c
 *  Felipe Muggiati Feldman
 */

#include "data_structures.h"
#include <stdio.h>
#include <stdlib.h>

// initialize queue
void queue_init(Queue* q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->if_empty, NULL);
}  // queue_init

// add message to queue
void queue_put(Queue* q, Message msg) {
    pthread_mutex_lock(&q->lock);

    Node* new_tail = malloc(sizeof(Node));
    if (new_tail == NULL) {
        perror("queue::malloc");
        exit(1);
    }
    new_tail->message = msg;
    new_tail->next = NULL;
    if (q->size == 0) {
        q->head = new_tail;
        q->tail = new_tail;
    } else {
        q->tail->next = new_tail;  // add new node to end
        q->tail = q->tail->next;  // bump
    }
    ++q->size;

    pthread_cond_signal(&q->if_empty);
    pthread_mutex_unlock(&q->lock);
}  // queue_put

Message queue_get(Queue* q) {
    pthread_mutex_lock(&q->lock);

    while (q->size == 0) {
        pthread_cond_wait(&q->if_empty, &q->lock);
    }
    Message ret_val = q->head->message;
    Node* to_delete = q->head;
    q->head = q->head->next;
    if (q->size == 1) {
        q->tail = NULL;
    }
    free(to_delete);
    --q->size;

    pthread_mutex_unlock(&q->lock);
    return ret_val;
}

void queue_cleanup(Queue* q) {
    Node* cursor = q->head;
    while (cursor) {
        Node* next = cursor->next;
        free(cursor);
        cursor = next;
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->if_empty);
}

void queue_print(Queue* q) {
    pthread_mutex_lock(&q->lock);

    if (q->size == 0) {
        pthread_mutex_unlock(&q->lock);
        return;
    }

    Node* cursor = q->head;
    putchar('(');
    while (cursor->next != NULL) {
        printf("%s -> ", cursor->message.text);
        cursor = cursor->next;
    }
    printf("%s)", cursor->message.text);

    pthread_mutex_unlock(&q->lock);
}

void vector_init(Vector* v) {
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
    pthread_mutex_init(&v->lock, NULL);
    pthread_cond_init(&v->if_empty, NULL);
}

void vector_push_back(Vector* v, User* user) {
    pthread_mutex_lock(&v->lock);

    if (v->size == v->cap) {
        const size_t new_cap = (v->cap == 0) ? 1 : v->cap * 2;
        User** temp = realloc(v->data, sizeof(User*) * new_cap);
        if (!temp) {
            perror("Vector::realloc");
            exit(1);
        }
        v->data = temp;
        v->cap = new_cap;
    }
    v->data[v->size++] = user;

    pthread_mutex_unlock(&v->lock);
}

void vector_remove(Vector* v, User* u) {
    pthread_mutex_lock(&v->lock);

    for (size_t i = 0; i < v->size; ++i) {
        if (v->data[i] == u) {
            for (size_t j = i; j < v->size-1; ++j) {
                v->data[j] = v->data[j+1];
            }
            v->data[--v->size] = NULL;
            pthread_mutex_unlock(&v->lock);
            return;
        }
    }

    pthread_mutex_unlock(&v->lock);
}

void vector_cleanup(Vector* v) {
    free(v->data);
    v->size = 0;
    v->cap = 0;
    pthread_mutex_destroy(&v->lock);
    pthread_cond_destroy(&v->if_empty);
}

void vector_display(Vector* v) {
    pthread_mutex_lock(&v->lock);

    putchar('[');
    if (v->size >= 1) {
        printf("%s", v->data[0]->name);
    }
    for (size_t i = 1; i < v->size; ++i) {
        printf(" %s", v->data[i]->name);
    }
    puts("]");

    pthread_mutex_unlock(&v->lock);
}
