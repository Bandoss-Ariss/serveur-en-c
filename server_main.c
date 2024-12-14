#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "server_fifo.h"
#include "server_admin.h"
#include "server_transactions.h"

int main() {
    // Créer la FIFO de transmission
    create_fifo();

    // Ouvrir la FIFO en lecture (bloquante)
    int fifo_fd = open_fifo_for_reading();

    // Boucle de traitement des transactions
    readTrans(fifo_fd);

    // Nettoyage à la fin
    close(fifo_fd);
    unlink(FIFO_PATH); // Supprimer la FIFO
    return 0;
}
