#include "client_windows.h"
#include <unistd.h>  // Pour getpid et unlink
#include "client_transmit.h"

// Définitions des fenêtres globales
WINDOW* transmission_win;
WINDOW* reception_win;

void init_windows() {
    initscr();            // Initialiser ncurses
    start_color();        // Activer les couleurs
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Initialisation des paires de couleurs
    init_pair(1, COLOR_RED, COLOR_BLACK);   // Couleur pour listage
    init_pair(2, COLOR_GREEN, COLOR_BLACK); // Couleur pour messages généraux

    // Création des fenêtres
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    transmission_win = newwin(rows / 2, cols, 0, 0);
    box(transmission_win, 0, 0);
    wattron(transmission_win, COLOR_PAIR(2));
    mvwprintw(transmission_win, 1, 1, "Fenêtre TRANSMISSION");
    wattroff(transmission_win, COLOR_PAIR(2));
    wrefresh(transmission_win);

    reception_win = newwin(rows / 2, cols, rows / 2, 0);
    box(reception_win, 0, 0);
    wattron(reception_win, COLOR_PAIR(1));
    mvwprintw(reception_win, 1, 1, "Fenêtre RECEPTION");
    wattroff(reception_win, COLOR_PAIR(1));
    wrefresh(reception_win);
}

void close_client() {
    char client_fifo[100];
    snprintf(client_fifo, sizeof(client_fifo), "FIFO%d", getpid());

    // Supprimer la FIFO client
    if (unlink(client_fifo) == 0) {
        mvwprintw(transmission_win, 6, 1, "FIFO client supprimée avec succès.");
    } else {
        mvwprintw(transmission_win, 6, 1, "Erreur : Impossible de supprimer la FIFO client.");
    }
    wrefresh(transmission_win);

    // Nettoyer les fenêtres
    delwin(transmission_win);
    delwin(reception_win);
    endwin();
}
