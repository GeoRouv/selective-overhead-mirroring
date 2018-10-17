#Project 2
CC = gcc 
FLAGS = -g3 -Wall -lpthread

all: clean MirrorInitiator MirrorServer ContentServer

MirrorInitiator: 
	$(CC) -o MirrorInitiator MirrorInitiator.c $(FLAGS) 

MirrorServer: 
	$(CC) -o MirrorServer MirrorServer.c  $(FLAGS) 

ContentServer:
	$(CC) -o ContentServer ContentServer.c $(FLAGS)	


clean:
	rm -rf *o ContentServer
	rm -rf *o MirrorServer
	rm -rf *o MirrorInitiator

.PHONY: all clean

