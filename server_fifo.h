#ifndef SERVER_FIFO_H
#define SERVER_FIFO_H

#define FIFO_PATH "FIFO_TRANSACTIONS"

struct Info_FIFO_Transaction {
    int pid_client;
    char transaction[200];
};

void create_fifo();
int open_fifo_for_reading();

#endif
