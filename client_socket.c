// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ncurses.h>

struct Info_FIFO_Transaction {
    int pid_client;
    char transaction[200];
};

#define PORT 8080
#define BUFFER_SIZE 1024

WINDOW *transmission_win, *reception_win;

// Historique des commandes
static char command_history[10][BUFFER_SIZE]; // Stocke jusqu'à 10 commandes récentes
static int history_index = 0;

void *display_handler(void *arg) {
    int socket_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_received] = '\0';
        wprintw(reception_win, "%s", buffer);
        wrefresh(reception_win);
    }

    pthread_exit(NULL);
}

void initialize_windows() {
    initscr();
    cbreak();
    noecho();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    transmission_win = newwin(rows / 2, cols, 0, 0);
    reception_win = newwin(rows / 2, cols, rows / 2, 0);

    box(transmission_win, 0, 0);
    box(reception_win, 0, 0);

    mvwprintw(transmission_win, 0, 1, "Fenêtre de transmission ");
    mvwprintw(reception_win, 0, 1, " Fenêtre de réception ");

    wrefresh(transmission_win);
    wrefresh(reception_win);
}

void cleanup_windows() {
    delwin(transmission_win);
    delwin(reception_win);
    endwin();
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    initialize_windows();

    pthread_t display_thread;
    if (pthread_create(&display_thread, NULL, display_handler, &client_socket) != 0) {
        perror("pthread_create");
        cleanup_windows();
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Effacer la zone de saisie et repositionner le curseur
        wmove(transmission_win, 1, 1);
        wclrtoeol(transmission_win);
        mvwprintw(transmission_win, 1, 1, ""); // Laisser de l'espace pour la saisie
        wrefresh(transmission_win);

        // Lire la commande saisie
        mvwgetnstr(transmission_win, 1, 1, buffer, sizeof(buffer) - 1);

        // Afficher ce qui a été saisi dans la fenêtre
        wmove(transmission_win, 1, 1);
        wprintw(transmission_win, ">> %s", buffer);
        wrefresh(transmission_win);

        // Quitter si l'utilisateur entre 'quit'
        if (strcmp(buffer, "quit") == 0) {
            break;
        }

         // Ajouter la commande à l'historique
        strcpy(command_history[history_index % 10], buffer); // Stocke la commande dans le tableau
        history_index++;

        // Réafficher l'historique complet des commandes dans la fenêtre TRANSMISSION
        for (int i = 0; i < 10 && i < history_index; i++) {
            mvwprintw(transmission_win, i + 1, 1, "%d: %s", i + 1, command_history[i % 10]);
        }
        wrefresh(transmission_win); // Mettre à jour la fenêtre

        struct Info_FIFO_Transaction info;
        info.pid_client = getpid();
        strcpy(info.transaction, buffer);

        // Envoyer la commande au serveur
        //send(client_socket, buffer, strlen(buffer), 0);
        send(client_socket, &info, sizeof(info), 0);
    }



    pthread_cancel(display_thread);
    pthread_join(display_thread, NULL);

    cleanup_windows();
    close(client_socket);

    return 0;
}
