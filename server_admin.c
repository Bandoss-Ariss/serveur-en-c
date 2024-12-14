#include <pthread.h>
#include <stdbool.h>
#include <pthread.h>

static int admin_pid = -1; // PID de l'ADMIN actuel (-1 = aucun ADMIN)
static pthread_mutex_t admin_mutex = PTHREAD_MUTEX_INITIALIZER;

bool is_admin_connected() {
    pthread_mutex_lock(&admin_mutex);
    bool connected = (admin_pid != -1);
    pthread_mutex_unlock(&admin_mutex);
    return connected;
}

void set_admin(int pid) {
    pthread_mutex_lock(&admin_mutex);
    admin_pid = pid;
    pthread_mutex_unlock(&admin_mutex);
}

void clear_admin() {
    pthread_mutex_lock(&admin_mutex);
    admin_pid = -1;
    pthread_mutex_unlock(&admin_mutex);
}

int get_admin_pid() {
    pthread_mutex_lock(&admin_mutex);
    int pid = admin_pid;
    pthread_mutex_unlock(&admin_mutex);
    return pid;
}
