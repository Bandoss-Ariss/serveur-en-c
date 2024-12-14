// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>

#define PORT 8080
#define BUFFER_SIZE 1024
struct Info_FIFO_Transaction info;

struct paramF {
    int connfd;       // Socket de communication
    pid_t tid_client; // PID du client associé
};
struct Info_FIFO_Transaction {
    int pid_client;           // PID du client
    char transaction[200];    // Contenu de la transaction
};

void list_files(struct paramF* param) {
    pthread_detach(pthread_self()); // Detach the thread
    int connfd = param->connfd;
    int tid_client = param->tid_client;
    free(param); // Free the memory allocated for param

    int sfd = dup(STDOUT_FILENO); // Duplicate STDOUT
    /* Redirect STDOUT on connfd */
    if (-1 == dup2(connfd, STDOUT_FILENO)) {
        printf("\ncannot redirect STDOUT on connfd error\n");
        pthread_exit(NULL);
    }

    char command[100];
    // List all .c files in the current directory
    sprintf(command, "ls -l *.c");
    system(command); // Execute and redirect output to connfd

    /* Restore stdout */
    if (-1 != dup2(sfd, STDOUT_FILENO)) {
        printf("\nrecover STDOUT\n");
    }

    // Close the duplicated file descriptor
    close(sfd);
}

void listFileTable(struct paramF* param) {
    pthread_detach(pthread_self()); // Detach the thread
    int connfd = param->connfd;
    int pid = param->tid_client;
    free(param); // Free the memory allocated for param

    pid = getpid(); // Get the current process ID

    int sfd = dup(1); // Duplicate STDOUT
    /* Redirect STDOUT on connfd */
    if (-1 == dup2(connfd, 1)) {
        printf("\nCannot redirect STDOUT on connfd error\n");
        pthread_exit(NULL);
    }

    char command[100];
    // Command to list open files for the current process
    sprintf(command, "ls -l /proc/%d/fd", pid);
    system(command); // Execute the command and redirect output to connfd

    /* Restore stdout */
    if (-1 != dup2(sfd, 1)) {
        printf("\nrecover STDOUT\n");
    }

    // Close the duplicated file descriptor
    close(sfd);
}

void list_open_files(int client_socket) {
    pid_t pid = getpid();
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "ls -l /proc/%d/fd", pid);

    FILE *fp = popen(command, "r");
    if (!fp) {
        snprintf(command, sizeof(command), "Erreur d'exécution de la commande\n");
        send(client_socket, command, strlen(command), 0);
        return;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), fp) != NULL) {
        send(client_socket, line, strlen(line), 0);
    }

    pclose(fp);
}

void handle_file_stat(int client_socket, char *filename) {
    struct stat file_stat;
    char buffer[BUFFER_SIZE];

    if (stat(filename, &file_stat) == -1) {
        perror("stat");
        snprintf(buffer, sizeof(buffer), "Erreur de récupération des info sur le fichier %s\n", filename);
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }

    snprintf(buffer, sizeof(buffer),
             "Fichier: %s\nTaill: %ld bytes\nPermissions: %o\nDernières modifications: %ld\n",
             filename, file_stat.st_size, file_stat.st_mode & 0777, file_stat.st_mtime);
    send(client_socket, buffer, strlen(buffer), 0);
}

void listMemoryMap(struct paramF* param) {
    pthread_detach(pthread_self()); // Detach the thread
    int connfd = param->connfd;
    int pid = param->tid_client;
    free(param); // Free the memory allocated for param

    int sfd = dup(STDOUT_FILENO); // Duplicate STDOUT
    /* Redirect STDOUT on connfd */
    if (-1 == dup2(connfd, STDOUT_FILENO)) {
        printf("cannot redirect STDOUT on connfd error\n");
        pthread_exit(NULL);
    }

    char command[100];
    // List memory mappings for the current process
    sprintf(command, "cat /proc/%d/maps", pid);
    system(command); // Execute the command and redirect output to connfd

    /* Restore stdout */
    if (-1 != dup2(sfd, STDOUT_FILENO)) {
        printf("recover STDOUT\n");
    }

    // Close the duplicated file descriptor
    close(sfd);
}

//#######################################
//#
//# Affiche les items dont le numéro séquentiel est compris dans une plage
//#
void listItems(int client_socket, int start, int end) {

    typedef struct VM {
    int id;
    char name[50];
    char status[20]; // "active", "inactive", etc.
    } VM;

    // Exemple d'une liste de VMs
    VM vm_list[] = {
        {1, "VM1", "active"},
        {2, "VM2", "inactive"},
        {3, "VM3", "active"},
        {4, "VM4", "active"},
        {5, "VM5", "inactive"},
        {6, "VM6", "active"}
    };

    int vm_list_size = sizeof(vm_list) / sizeof(VM);
    char buffer[1024];
    int found = 0;

    // Validation des plages
    if (start < 1 || end > vm_list_size || start > end) {
        snprintf(buffer, sizeof(buffer), "Erreur : Plage invalide [%d - %d].\n", start, end);
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }

    // Parcourir la liste des VMs dans la plage spécifiée
    for (int i = start - 1; i < end; i++) {
        snprintf(buffer, sizeof(buffer), "VM ID: %d, Name: %s, Status: %s\n",
                 vm_list[i].id, vm_list[i].name, vm_list[i].status);
        send(client_socket, buffer, strlen(buffer), 0);
        found = 1;
    }

    // Si aucune VM trouvée dans la plage
    if (!found) {
        snprintf(buffer, sizeof(buffer), "Aucune VM trouvée dans la plage [%d - %d].\n", start, end);
        send(client_socket, buffer, strlen(buffer), 0);
    } else {
        snprintf(buffer, sizeof(buffer), "Fin de la liste des VMs dans la plage [%d - %d].\n", start, end);
        send(client_socket, buffer, strlen(buffer), 0);
    }
}

//#######################################
//#
void listOlcFile(int client_socket) {
    DIR *dir;
    struct dirent *entry;
    char buffer[1024];

    // Ouvrir le répertoire courant
    dir = opendir(".");
    if (dir == NULL) {
        snprintf(buffer, sizeof(buffer), "Erreur : Impossible d'ouvrir le répertoire courant.\n");
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }

    // Parcourir les fichiers du répertoire
    while ((entry = readdir(dir)) != NULL) {
        // Vérifier si l'extension est ".olc3"
        if (strstr(entry->d_name, ".olc3")) {
            snprintf(buffer, sizeof(buffer), "%s\n", entry->d_name);
            send(client_socket, buffer, strlen(buffer), 0); // Envoyer au client
        }
    }

    closedir(dir);

    // Indiquer la fin de la liste
    snprintf(buffer, sizeof(buffer), "Fin de la liste des fichiers .olc3\n");
    send(client_socket, buffer, strlen(buffer), 0);
}

void processList(struct paramF* param) {
    pthread_detach(pthread_self()); // Detach the thread
    int connfd = param->connfd;
    int pid = param->tid_client;
    free(param); // Free the memory allocated for param

    int sfd = dup(STDOUT_FILENO); // Duplicate STDOUT
    /* Redirect STDOUT on connfd */
    if (-1 == dup2(connfd, STDOUT_FILENO)) {
        printf("cannot redirect STDOUT on connfd error\n");
        pthread_exit(NULL);
    }

    char command[100];
    // Command to list active processes/threads
    sprintf(command, "ps -T -p %d", pid);
    system(command); // Execute the command and redirect output to connfd

    /* Restore stdout */
    if (-1 != dup2(sfd, STDOUT_FILENO)) {
        printf("recover STDOUT\n");
    }

    // Close the duplicated file descriptor
    close(sfd);
}

// Function to check if the file exists and is executable
int is_executable(const char *filename) {
    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {
        return (file_stat.st_mode & S_IXUSR); // Check if the user has execute permissions
    }
    return 0;
}

// Function to execute a `.olc3` file
void executeFile(int client_socket, int priority, const char *filename) {
    char buffer[1024];

    // Validate file extension
    if (!strstr(filename, ".olc3")) {
        snprintf(buffer, sizeof(buffer), "Erreur : Le fichier %s n'est pas un fichier .olc3.\n", filename);
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }

    // Check if the file exists and is executable
    if (!is_executable(filename)) {
        snprintf(buffer, sizeof(buffer), "Erreur : Le fichier %s est introuvable ou non exécutable.\n", filename);
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }

    // Log the execution attempt
    snprintf(buffer, sizeof(buffer), "Exécution du fichier %s avec priorité %d...\n", filename, priority);
    send(client_socket, buffer, strlen(buffer), 0);

    // Execute the file (use system() or an exec function)
    int ret = system(filename);

    // Send the execution result to the client
    if (ret == -1) {
        snprintf(buffer, sizeof(buffer), "Erreur : Impossible d'exécuter le fichier %s.\n", filename);
    } else {
        snprintf(buffer, sizeof(buffer), "Le fichier %s a été exécuté avec succès.\n", filename);
    }
    send(client_socket, buffer, strlen(buffer), 0);
}
void *readTrans(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int is_admin = 1;
    while ((bytes_received = recv(client_socket, &info, sizeof(info), 0)) > 0) {
    // Vérifier que les données reçues correspondent bien à la taille de la structure
    if (bytes_received != sizeof(info)) {
        char error_msg[] = "Erreur : Données reçues invalides.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        continue;
    }

    // Afficher les informations reçues pour débogage
    printf("Reçu : PID = %d, Transaction = %s\n", info.pid_client, info.transaction);

    // Traitement de la transaction (par exemple, en utilisant info.transaction)
    char *transaction = info.transaction;

    switch (transaction[0]) {
        case 'B':
        case 'b':
            listOlcFile(client_socket);
            break;
        case 'L':
        case 'l':
            if (is_admin) {
                // int nstart = atoi(strtok_r(transaction + 2, "-", NULL));
                // int nend = atoi(strtok_r(NULL, " ", NULL));
                listItems(client_socket, 1, 5);
            } else {
                send(client_socket, "Erreur : Commande réservée aux admin.\n", 38, 0);
            }
            break;
        case 'F':
        case 'f': {
            struct paramF *ptr = (struct paramF *)malloc(sizeof(struct paramF));
            ptr->connfd = client_socket;
            ptr->tid_client = getpid();
            pthread_t tid;
            pthread_create(&tid, NULL, listFileTable, ptr); // Creer un thread listFileTable()
            break;
        }

            case 'I':
            case 'i': {
                // Transaction I : Liste les fichiers d'un répertoire (utilisé dans l'image)
                struct paramF *ptr = (struct paramF *)malloc(sizeof(struct paramF));
                ptr->connfd = client_socket;
                ptr->tid_client = getpid(); 
                pthread_t tid;
                pthread_create(&tid, NULL, list_files, ptr); 
                pthread_detach(tid); 
                break;
            }

            case 'P':
            case 'p': {
                // Transaction P : Liste les threads actifs
                struct paramF *ptr = (struct paramF *)malloc(sizeof(struct paramF));
                ptr->connfd = client_socket;
                ptr->tid_client = getpid();
                pthread_t tid;
                pthread_create(&tid, NULL, processList, ptr); // Crée un thread pour processList
                pthread_detach(tid);
                break;
            }

            case 'M':
            case 'm': {
                // Transaction M : Affiche le mapping mémoire
                struct paramF *ptr = (struct paramF *)malloc(sizeof(struct paramF));
                ptr->connfd = client_socket;
                ptr->tid_client = getpid();
                pthread_t tid;
                pthread_create(&tid, NULL, listMemoryMap, ptr); // Crée un thread pour listMemoryMap
                pthread_detach(tid);
                break;
            }

            default: {
                snprintf(buffer, sizeof(buffer), "Erreur : Type de transaction inconnu.\n");
                send(client_socket, buffer, strlen(buffer), 0);
                break;
            }
        }
    }

    close(client_socket);
    free(arg);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) == -1) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Serveur en écoute sur le port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_socket;

        pthread_t thread;
        if (pthread_create(&thread, NULL, readTrans, client_sock_ptr) != 0) {
            perror("pthread_create");
            close(client_socket);
            free(client_sock_ptr);
        }

        pthread_detach(thread);
    }

    close(server_socket);
    return 0;
}