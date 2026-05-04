/*
 *  ChatFeliPThread - client.c
 *  Felipe Muggiati Feldman
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#define	MAX_LINE	4096
#define MAX_USERNAME  64

#define	BUFFSIZE 8192
#define	LISTENQ	1024
#define PORT_NUM 13000
#define MAX_PORT_NUM 65535  // maximum acceptable user port

int parse_user_port(const char* input);
void chat(int sockfd);
int receive_message(int sockfd);
int send_message(int sockfd);

// global consts and function prototypes
static const char LOCALHOST[] = "localhost";
void handle_args(int argc, char** argv,
    const char** username, int* port_num, const char** hostname);
struct in_addr read_hostname(const char* hostname);

int main(int argc, char** argv) {
    // set up vars for the various calls
    int sockfd, port_num;
    const char *username, *hostname;
    struct sockaddr_in servaddr;

    // parse args
    handle_args(argc, argv, &username, &port_num, &hostname);
    // open socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket error");
        exit(1);
    }
    // oh boy, ANOTHER struct!
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port_num);
    servaddr.sin_addr = read_hostname(hostname);
    // if connection fails, give up
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        close(sockfd);
        exit(1);
    }
    // send client's name
    if (write(sockfd, username, strlen(username) + 1) < 0) {
        perror("write error");
        close(sockfd);
        exit(1);
    }
    printf("Connected to server %s:%d\n", hostname, port_num);
    puts("(Type Ctrl + D (EOF) to quit, or Ctrl + C)");
    fflush(stdout);

    chat(sockfd);  // loop until server closes or EOF read
    close(sockfd);  // close socket FD
}

// extremely boring function to handle user-provided arguments and flags
void handle_args(int argc, char** argv,
        const char** username, int* port_num, const char** hostname) {
    if (argc < 2) {
        fputs("usage: client <username> [-p port] [-a address]\n", stderr);
        exit(1);
    }
    if (argc == 2) {
        *port_num = PORT_NUM;
        *hostname = LOCALHOST;
    } else if (argc == 4) {
        if (strcmp(argv[2], "-p") == 0) {
            *port_num = parse_user_port(argv[3]);
            *hostname = LOCALHOST;
        } else if (strcmp("-a", argv[2]) == 0) {
            *port_num = PORT_NUM;
            *hostname = argv[3];
        } else {
            *port_num = parse_user_port(argv[2]);
            *hostname = argv[3];
        }
    } else if (argc == 6) {
        if (strcmp(argv[2], "-p") == 0) {
            *port_num = parse_user_port(argv[3]);
            *hostname = argv[5];
        } else {
            *port_num = parse_user_port(argv[5]);
            *hostname = argv[3];
        }
    } else {
        fputs("usage: client <username> [-p port] [-a address]\n", stderr);
        exit(1);
    }
    if (*port_num < 0) {
        fputs("usage: client <username> [-p port] [-a address]\n", stderr);
        exit(1);
    }
    if (strlen(argv[1]) > MAX_USERNAME) {
        fputs("username must not exceed 64 characters!\n"
              ": client <username> [-p port] [-a address]\n", stderr);
        exit(1);
    }
    *username = argv[1];
}  // handle_args

// function to convert hostname string into a usable IP address
struct in_addr read_hostname(const char* hostname) {
    // yet more insufferable struct manipulation
    struct addrinfo hint;
    struct addrinfo* results;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    // perform getaddrinfo to interpret hostname
    if (getaddrinfo(hostname, NULL, &hint, &results) != 0) {
        fprintf(stderr, "getaddrinfo error for: %s\n", hostname);
        exit(1);
    }

    // extract the in_addr to return
    struct in_addr out = ((struct sockaddr_in*) results->ai_addr)->sin_addr;
    freeaddrinfo(results);  // it allocated a linked list on the heap... >:(
    return out;
}  // read_hostname

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

void chat(int sockfd) {
    for (;;) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        FD_SET(sockfd, &set);
        int nfds = ((sockfd) > (STDIN_FILENO)) ? (sockfd) : (STDIN_FILENO);
        int count = select(nfds + 1, &set, NULL, NULL, NULL);
        if (count < 0) {
            perror("Select failed");
            return;
        }
        if (count == 0) {
            fprintf(stderr, "Timeout\n");
            break;
        }
        if (FD_ISSET(sockfd, &set)) {  // read from socket to stdout FIRST
            if (receive_message(sockfd) < 0) {
                puts("Lost connection to server.");
                return;
            }
        }
        if (FD_ISSET(STDIN_FILENO, &set)) {  // write to socket
            if (send_message(sockfd) < 0) return;
        }
    }
}  // chat

int receive_message(int sockfd) {
    char buffer[MAX_LINE];
    ssize_t n = read(sockfd, buffer, MAX_LINE);
    if (n < 0) {
        perror("read error");
        return -1;
    }
    if (n == 0) {  // reached EOF
        return -1;
    }
    buffer[n] = '\0';
    n = write(STDOUT_FILENO, buffer, n);
    if (n < 0) {
        perror("write error");
        return -1;
    }
    return 0;
}  // receive_message

int send_message(int sockfd) {
    char buffer[MAX_LINE];
    ssize_t n = read(STDIN_FILENO, buffer, MAX_LINE);
    if (n < 0) {
        perror("read error");
        return -1;
    }
    if (n == 0) {  // reached EOF
        return -1;
    }
    ssize_t written = write(sockfd, buffer, n);
    if (written < 0) {
        perror("write error");
        return -1;
    }
    if (written != n) {
        fprintf(stderr, "partial write to socket\n");
        return -1;
    }
    return 0;
}  // send_message
