# Felipe Muggiati Feldman - hw07
# Makefile for server.c, client.c

CC = gcc
FLAGS = -std=c11 -Wall -pthread

all: server client

server: server.c data_structures.c data_structures.h
	${CC} ${FLAGS} -o server server.c data_structures.c

client: client.c
	${CC} ${FLAGS} -o client client.c

clean:
	rm -f server client
