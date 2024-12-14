#ifndef CLIENT_WINDOWS_H
#define CLIENT_WINDOWS_H

#include <ncurses.h>

// Déclarations des fenêtres globales
extern WINDOW* transmission_win;
extern WINDOW* reception_win;

// Fonctions
void init_windows();
void close_client();
void* recevoir_reponses(void* arg);  // Ajout de cette ligne

#endif
