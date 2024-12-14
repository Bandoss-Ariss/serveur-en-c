#include <ncurses.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>

#include "client_transmit.h"
#include "client_windows.h" // Pour close_client


void transmit_request(WINDOW* transmission_win) {
    struct Info_FIFO_Transaction request;
    request.pid_client = getpid(); // PID du client
    char input[200];
    char client_fifo[100];

    // Demander à l'utilisateur de saisir une requête
    mvwprintw(transmission_win, 3, 1, "Saisir une requête : ");
    wrefresh(transmission_win);
    wgetstr(transmission_win, input);

    // Copier l'entrée utilisateur dans la structure de requête
    strncpy(request.transaction, input, sizeof(request.transaction) - 1);
    request.transaction[sizeof(request.transaction) - 1] = '\0'; // Assurer la terminaison de la chaîne



    // Créer une FIFO client unique (nommée FIFO<pid>)
    snprintf(client_fifo, sizeof(client_fifo), "FIFO%d", getpid());
    if (mkfifo(client_fifo, 0666) == -1) {
        if (errno != EEXIST) { // Ignorer si la FIFO existe déjà
            mvwprintw(transmission_win, 4, 1, "Erreur : Création de la FIFO client");
            wrefresh(transmission_win);
            return;
        }
    }

    // Ouvrir la FIFO serveur en écriture
    int server_fifo_fd = open("FIFO_TRANSACTIONS", O_WRONLY);
    if (server_fifo_fd == -1) {
        mvwprintw(transmission_win, 4, 1, "Erreur : Impossible de se connecter au serveur");
        wrefresh(transmission_win);
        unlink(client_fifo); // Supprimer la FIFO client
        return;
    }
    
    if (write(server_fifo_fd, &request, sizeof(request)) == -1) {
    perror("Erreur d'envoi au serveur");
    } else {
        printf("Transaction envoyée : %s (PID client : %d)\n", request.transaction, request.pid_client);
    }

    // Écrire la requête dans la FIFO serveur
    if (write(server_fifo_fd, &request, sizeof(request)) == -1) {
        mvwprintw(transmission_win, 5, 1, "Erreur : Échec de l'envoi de la requête");
        wrefresh(transmission_win);
        close(server_fifo_fd);
        unlink(client_fifo); // Supprimer la FIFO client
        return;
    }
    close(server_fifo_fd); // Fermer la FIFO serveur après l'envoi

    // Afficher que la requête a été envoyée
    mvwprintw(transmission_win, 6, 1, "Requête envoyée : %s", input);
    wrefresh(transmission_win);

    // Ouvrir la FIFO client pour lire la réponse
    int client_fifo_fd = open(client_fifo, O_RDONLY);
    if (client_fifo_fd == -1) {
        mvwprintw(transmission_win, 7, 1, "Erreur : Lecture de la FIFO client");
        wrefresh(transmission_win);
        unlink(client_fifo); // Supprimer la FIFO client
        return;
    }

    // Lire la réponse du serveur
    char response[200];
    int bytes_read = read(client_fifo_fd, response, sizeof(response) - 1);
    if (bytes_read > 0) {
        response[bytes_read] = '\0'; // Terminer correctement la chaîne
        mvwprintw(transmission_win, 8, 1, "Réponse reçue : ");
        mvwprintw(transmission_win, 9, 1, "%s", response); // Correction ici
    } else {
        mvwprintw(transmission_win, 8, 1, "Erreur : Pas de réponse du serveur");
    }
    wrefresh(transmission_win);

    // Nettoyer
    close(client_fifo_fd);
    unlink(client_fifo); // Supprimer la FIFO client
}

void* envoyer_transactions(void* arg) {
    while (1) {
        char input[200];
        mvwprintw(transmission_win, 3, 1, "Tapez une transaction (ou 'quit') : ");
        wrefresh(transmission_win);
        wgetstr(transmission_win, input);

        if (strcmp(input, "quit") == 0) {
            close_client();
            pthread_exit(NULL);
        }

        // Envoyer la transaction
        transmit_request(transmission_win);
    }
}

void* recevoir_reponses(void* arg) {
    char client_fifo[100];
    snprintf(client_fifo, sizeof(client_fifo), "FIFO%d", getpid());
    mkfifo(client_fifo, 0666);

    int client_fifo_fd = open(client_fifo, O_RDONLY);
    if (client_fifo_fd == -1) {
        perror("Erreur d'ouverture de la FIFO client");
        pthread_exit(NULL);
    }

    char response[200];
    while (read(client_fifo_fd, response, sizeof(response)) > 0) {
        wattron(reception_win, COLOR_PAIR(1));
        mvwprintw(reception_win, 3, 1, "Réponse du serveur : %s", response);
        wattroff(reception_win, COLOR_PAIR(1));
        wrefresh(reception_win);
    }

    close(client_fifo_fd);
    unlink(client_fifo);
    pthread_exit(NULL);
}
