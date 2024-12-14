#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "server_fifo.h"

void create_fifo() {
    if (access(FIFO_PATH, F_OK) == 0) {
        unlink(FIFO_PATH);
        printf("FIFO_TRANSACTIONS existante supprimée.\n");
    }
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        perror("Erreur de création de FIFO_TRANSACTIONS");
        exit(1);
    }
    printf("FIFO_TRANSACTIONS créée avec succès.\n");
}

int open_fifo_for_reading() {
    int fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Erreur d'ouverture de FIFO_TRANSACTIONS");
        exit(1);
    }
    printf("FIFO_TRANSACTIONS ouverte pour lecture.\n");
    return fifo_fd;
}
