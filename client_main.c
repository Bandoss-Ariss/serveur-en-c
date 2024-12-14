#include <ncurses.h>
#include <pthread.h>
#include "client_windows.h"
#include "client_transmit.h"


// Déclaration des fonctions
void init_windows();
void close_client();
void* envoyer_transactions(void* arg);
void* recevoir_reponses(void* arg);

int main() {
    init_windows();

    pthread_t thread_envoi, thread_reception;

    pthread_create(&thread_envoi, NULL, envoyer_transactions, NULL);
    pthread_create(&thread_reception, NULL, recevoir_reponses, NULL);

    pthread_join(thread_envoi, NULL);
    pthread_cancel(thread_reception);

    close_client();
    return 0;
}
