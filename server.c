/*
 *  ChatFeliPThread - server.c
 *  a semi-sophisticated server for a multi-threaded chat room
 *  by Felipe Muggiati Feldman
 */
#include "data_structures.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#define	MAX_LINE	  4096
#define MAX_USERNAME    64
#define PORT_NUM     13000
#define MAX_PORT_NUM 65535  // maximum acceptable user port
#define	LISTENQ       1024

// prototypes for parser functions (unchanged from previous assignment)
void parse_args(int argc, char** argv, int* port);
int parse_user_port(const char* input);

// new functions to handle multithreading
void* client_service(void* arg);
void message_collector(User* u);
void* message_sender_service(void* arg);
User* create_user(void);
int set_user_name(User*);
void announce_arrival(Queue* q, User* u);
void display_others(Vector* v, User* u);
void user_cleanup(User* u);

int main(int argc, char** argv) {
    // set up vars for the various calls
    int	listenfd, connfd, port_num;
    struct sockaddr_in servaddr;

    Queue message_queue;
    Vector connected_users;
    queue_init(&message_queue);
    vector_init(&connected_users);
    MessageSenderArgs msa = {&message_queue, &connected_users};
    pthread_t msg_buffer;

    if (pthread_create(&msg_buffer, NULL, message_sender_service, &msa) != 0) {
        perror("pthread_create: messages buffer");
        exit(1);
    }

    // parse args
    parse_args(argc, argv, &port_num);
    // open socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // consider the code below...unfixed!

    /*
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                                        &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        close(listenfd);
        exit(1);
    }
    */



    // deal with insufferable struct
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_num);
    // error checking for bind
    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(listenfd);
        exit(1);
    }
    // error checking for listen
    if (listen(listenfd, LISTENQ) < 0) {
        perror("listen");
        close(listenfd);
        exit(1);
    }

    printf("Server is listening on port %d\n", port_num);
    fflush(stdout);

    for (;;) {  // loop until interrupted/killed
        connfd = accept(listenfd, NULL, NULL);  // block until client connects
        if (connfd == -1) {
            perror("accept");
            break;
        }
        User* u = create_user();
        u->user_fd = connfd;
        u->q = &message_queue;
        u->v = &connected_users;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_service, u) != 0) {
            perror("pthread_create: new connection");
            break;
        }
        pthread_detach(tid);
    }
}  // main

// heap-allocates a new User; does NOT leak memory, as users are destroyed
// in the user_cleanup function
User* create_user(void) {
    User* new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        perror("malloc");
        return NULL;
    }
    return new_user;
}  // create_user

// free's a user's name, removes the user from the Vector of users
void user_cleanup(User* u) {
    free(u->name);
    vector_remove(u->v, u);
    free(u);
}  //

void* message_sender_service(void* arg) {
    MessageSenderArgs* msa = arg;
    Queue* q = msa->q;
    Vector* v = msa->v;

    for (;;) {
        Message next = queue_get(q);

        pthread_mutex_lock(&v->lock);

        for (size_t i = 0; i < v->size; ++i) {
            if (v->data[i] != next.user) {
                if (write(v->data[i]->user_fd, next.text, next.size) < 0) {
                    perror("msg_send_serv");
                    break;
                }
            }
        }

        pthread_mutex_unlock(&v->lock);
        free(next.text);
    }
}

void* client_service(void* arg) {
    User* u = arg;
    Vector* v = u->v;
    Queue* q = u->q;

    u->tid = pthread_self();
    if (set_user_name(u) == -1) {
        pthread_exit(NULL);
    }
    printf("User \"%s\" has connected\n", u->name);
    fflush(stdout);
    vector_push_back(v, u);
    announce_arrival(q, u);
    display_others(v, u);
    message_collector(u);

    return NULL;
}

int set_user_name(User* u) {
    char user_buffer[MAX_USERNAME + 1] = {0};
    ssize_t n_read = read(u->user_fd, user_buffer, MAX_USERNAME);
    if (n_read > 0) {
        u->name = strdup(user_buffer);
        if (u->name == NULL) {
            perror("set_user_name: strdup");
            return -1;
        }
    } else {
        perror("set_user_name::socket read");
        return -1;
    }
    return 0;
}

void announce_arrival(Queue* q, User* u) {
    Message m = {u};
    ssize_t n_write = snprintf(NULL, 0, "%s has entered the chat!\n", u->name);
    m.text = malloc(n_write+1);
    if (m.text == NULL) {
        perror("announce_arrival::malloc");
    }
    m.size = n_write;
    snprintf(m.text, n_write+1, "%s has entered the chat!\n", u->name);
    queue_put(q, m);
}

void display_others(Vector* v, User* u) {
    pthread_mutex_lock(&v->lock);

    char* out = malloc(MAX_LINE);
    if (out == NULL) {
        perror("display_others::malloc");
        pthread_mutex_unlock(&v->lock);
        return;
    }
    ssize_t total = 0;
    if (v->size <= 1) {
        total = snprintf(out, MAX_LINE, "No other connected users.\n");
    } else {
        total = snprintf(out, MAX_LINE, "Others present: ");
        for (size_t i = 0; i < v->size-1; ++i) {
            if (v->data[i] == u) {
                continue;
            }
            ssize_t written = snprintf(out + total, MAX_LINE - total,
                                                    "%s, ", v->data[i]->name);
            if (written > 0) {
                total += written;
            }
            if (total >= MAX_LINE - 1) break;
        }

        if (total > 2 && out[total - 2] == ',') {
            out[total - 2] = '\n';
            out[total - 1] = '\0';
            total--;
        }
    }
    if (write(u->user_fd, out, (size_t)total) < 0) {
        perror("display_others::write");
    }

    pthread_mutex_unlock(&v->lock);
    free(out);
}

void message_collector(User* u) {
    Queue* q = u->q;
    ssize_t n_read;
    char input_buffer[MAX_LINE+1];
    Message m = {u};

    while ((n_read = read(u->user_fd, input_buffer, MAX_LINE)) > 0) {
        input_buffer[n_read] = '\0';
        ssize_t n_write = snprintf(NULL, 0, "%s> %s", u->name, input_buffer);
        m.text = malloc(n_write+1);
        if (m.text == NULL) {
            perror("client_service::malloc");
            user_cleanup(u);
        }
        m.size = n_write;
        snprintf(m.text, n_write+1, "%s> %s", u->name, input_buffer);
        queue_put(q, m);
    }

    if (n_read <= 0) {  // disconnected + socket read error (same cleanup)
        ssize_t n_write = snprintf(NULL, 0, "%s has left the chat.\n", u->name);
        m.text = malloc(n_write+1);
        if (m.text == NULL) {
            perror("client_service::disconnected::malloc");
            user_cleanup(u);
        }
        m.size = n_write;
        snprintf(m.text, n_write+1, "%s has left the chat.\n", u->name);
        queue_put(q, m);
        printf("User \"%s\" has disconnected\n", u->name);
        fflush(stdout);
        user_cleanup(u);
        pthread_exit(NULL);
    }
}


// argument parsers (unchanged)
void parse_args(int argc, char** argv, int* port) {
    const char invalid_arg_error[] = "usage: server [port number]\n";
    if (argc > 2) {
        fputs(invalid_arg_error, stderr);
        exit(1);
    }
    if (argc == 2) {
        *port = parse_user_port(argv[1]);
        if (*port < 0) {
            fputs(invalid_arg_error, stderr);
            exit(1);
        }
    } else {
        *port = PORT_NUM;
    }
}  // handle_args

int parse_user_port(const char* input) {  // a whole lotta error checking
    char* end_ptr;
    errno = 0;
    int port = (int) strtol(input, &end_ptr, 10);  // god I hate strtol
    if (errno != 0) {
        perror("error: out of range");
        return -1;
    }
    if (end_ptr == input || *end_ptr != '\0') {
        fputs("arg parse error: invalid port (must be an integer)\n", stderr);
        return -1;
    }
    if (port > MAX_PORT_NUM) {
        fputs("arg parse error: port must be between 0 and 65535\n", stderr);
        return -1;
    }
    return port;
}  // parse_user_port
