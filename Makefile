CC = gcc
CFLAGS = -Wall -g `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0` -lrt -lpthread

all: chat_app tcp_server

chat_app: main.o gui.o shared_memory_client.o tcp_client.o
	$(CC) -o chat_app main.o gui.o shared_memory_client.o tcp_client.o $(LDFLAGS)

tcp_server: tcp_server.o
	$(CC) -o tcp_server tcp_server.o $(LDFLAGS)

main.o: main.c gui.h client.h
	$(CC) $(CFLAGS) -c main.c

gui.o: gui.c gui.h common.h
	$(CC) $(CFLAGS) -c gui.c

shared_memory_client.o: shared_memory_client.c gui.h common.h client.h
	$(CC) $(CFLAGS) -c shared_memory_client.c

tcp_server.o: tcp_server.c common.h
	$(CC) $(CFLAGS) -c tcp_server.c

tcp_client.o: tcp_client.c gui.h common.h client.h
	$(CC) $(CFLAGS) -c tcp_client.c

clean:
	rm -f *.o chat_app tcp_server
