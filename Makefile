# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS_NCURSES = -lncurses

SERVER = server
CLIENT = client

SERVER_SRC = server_socket.c
CLIENT_SRC = client_socket.c

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT) $(LDFLAGS_NCURSES)

clean:
	rm -f $(SERVER) $(CLIENT)
