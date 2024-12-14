#ifndef SERVER_ADMIN_H
#define SERVER_ADMIN_H

#include <stdbool.h>

bool is_admin_connected();
void set_admin(int pid);
void clear_admin();
int get_admin_pid();  // Déclarez une fonction pour récupérer le PID de l'ADMIN.

#endif
