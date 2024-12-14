#ifndef CLIENT_TRANSMIT_H
#define CLIENT_TRANSMIT_H

// Déclaration de la structure Info_FIFO_Transaction
struct Info_FIFO_Transaction {
    int pid_client;
    char transaction[200];
};

// Déclarations des fonctions
void* envoyer_transactions(void* arg);
void transmit_request(WINDOW* transmission_win);
void* recevoir_reponses(void* arg);

#endif // CLIENT_TRANSMIT_H
