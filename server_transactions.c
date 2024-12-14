#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "gestionListeChaineeVMS.h"
#include "server_admin.h"
#include "server_fifo.h"

int admin_pid = -1;  // PID du client ADMIN actuel
pthread_mutex_t admin_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex pour protéger admin_pid

void redirectAndExecute(const char* command, int client_fifo_fd);
//void removeVM(int noVM);
void* traiter_transaction(void* arg) {
    struct Info_FIFO_Transaction* trans = (struct Info_FIFO_Transaction*)arg;
    char* transaction = trans->transaction;
    int pid_client = trans->pid_client;
    printf("Transaction brute reçue : '%s' (PID client : %d)\n", trans->transaction, trans->pid_client);


    //Validation de la transaction
    // if (transaction == NULL || strlen(transaction) == 0) {
    //     printf("Erreur : Transaction vide ou invalide.\n");
    //     free(trans);
    //     return NULL;
    // }

    char* tok;
    char* sp;
    tok = strtok_r(transaction, " ", &sp);

    pthread_mutex_lock(&admin_mutex);
    if ((tok[0] == 'L' || tok[0] == 'P' || tok[0] == 'K' || tok[0] == 'E') && admin_pid != -1 && admin_pid != pid_client) {
        printf("Erreur : Un autre ADMIN est déjà connecté (PID %d).\n", admin_pid);
        pthread_mutex_unlock(&admin_mutex);
        free(trans);
        return NULL;
    }

    if ((tok[0] == 'L' || tok[0] == 'P' || tok[0] == 'K' || tok[0] == 'E') && admin_pid == -1) {
        admin_pid = pid_client;
        printf("Client ADMIN connecté : PID %d\n", admin_pid);
    }
    pthread_mutex_unlock(&admin_mutex);

    tok[0] = 'A';

    // Traitement des transactions
    switch (tok[0]) {
        case 'A': {
            printf('On est bon');
        }
        case 'B': {
            listOlcFile();	// Afficher la liste de fichiers binaires .olc		
			break;
        }
        case 'P': {  // Liste des processus
            char client_fifo[100];
            sprintf(client_fifo, "FIFO%d", trans->pid_client);
            int client_fifo_fd = open(client_fifo, O_WRONLY);
            if (client_fifo_fd == -1) {
                perror("Erreur d'ouverture de la FIFO client");
                break;
            }
            const char* command = (tok[0] == 'B') ? "ls -l *.olc3" : "ps -e -o pid,tid,cmd";
            redirectAndExecute(command, client_fifo_fd);
            close(client_fifo_fd);
            break;
        }

        case 'L': {  // Liste des VMs
            if (is_admin_connected()) {
                char* start_str = strtok_r(NULL, "-", &sp);
                char* end_str = strtok_r(NULL, " ", &sp);
                if (start_str == NULL || end_str == NULL) {
                    printf("Erreur : Paramètres manquants pour L.\n");
                    break;
                }
                int nstart = atoi(start_str);
                int nend = atoi(end_str);

                char client_fifo[100];
                sprintf(client_fifo, "FIFO%d", trans->pid_client);
                int client_fifo_fd = open(client_fifo, O_WRONLY);
                if (client_fifo_fd == -1) {
                    perror("Erreur d'ouverture de la FIFO client");
                    break;
                }

                struct noeudVM* current = head;
                while (current != NULL) {
                    if (current->VM.noVM >= nstart && current->VM.noVM <= nend) {
                        char buffer[400];
                        serializeInfoVM(buffer, &current->VM);
                        write(client_fifo_fd, buffer, strlen(buffer) + 1);
                    }
                    current = current->suivant;
                }
                close(client_fifo_fd);
            } else {
                printf("Erreur : Transaction L réservée aux ADMIN.\n");
            }
            break;
        }

        case 'E': {  // Supprimer une VM
            if (is_admin_connected()) {
                int noVM = atoi(strtok_r(NULL, " ", &sp));
                printf("Suppression de la VM");
                //removeVM(noVM);
            } else {
                printf("Erreur : Transaction E réservée aux ADMIN.\n");
            }
            break;
        }

        case 'K': {  // Tuer un thread
            if (is_admin_connected()) {
                int tid = atoi(strtok_r(NULL, " ", &sp));
                killThread(tid);
            } else {
                printf("Erreur : Transaction K réservée aux ADMIN.\n");
            }
            break;
        }

        default:
            printf("Erreur : Type de transaction inconnu (%s).\n", tok);
            break;
    }

    // Répondre au client
    char client_fifo[100];
    sprintf(client_fifo, "FIFO%d", trans->pid_client);
    int client_fifo_fd = open(client_fifo, O_WRONLY);
    if (client_fifo_fd != -1) {
        char response[256];
        snprintf(response, sizeof(response), "Transaction %s traitée avec succès", tok);
        write(client_fifo_fd, response, strlen(response) + 1);
        close(client_fifo_fd);
    } else {
        printf("Erreur : Impossible d'envoyer une réponse au client (PID %d).\n", pid_client);
    }

    free(trans);
    return NULL;
}



void readTrans(int fifo_fd) {
    int admin_pid = -1;  // Stocker le PID du client ADMIN connecté
    while (1) {
        struct Info_FIFO_Transaction* trans = malloc(sizeof(struct Info_FIFO_Transaction));
        printf("Mémoire allouée pour transaction : %p\n", (void*)trans);
        if (trans == NULL) {
            perror("Erreur d'allocation de mémoire pour Info_FIFO_Transaction");
            continue;
        }

        ssize_t bytes_read = read(fifo_fd, trans, sizeof(*trans));
        if (bytes_read > 0) {
            char* tok = strtok(trans->transaction, " ");
            if (tok != NULL) {
                switch (tok[0]) {
                    case 'L':
                    case 'P':
                    case 'K':
                    case 'E':
                        if (admin_pid == -1) {
                            admin_pid = trans->pid_client;  // Déterminer le client ADMIN
                            printf("Client ADMIN connecté : PID %d\n", admin_pid);
                        } else if (admin_pid != trans->pid_client) {
                            printf("Erreur : Un autre ADMIN est déjà connecté (PID %d).\n", admin_pid);
                            free(trans);
                            continue;  // Ignorer la transaction
                        }
                        break;
                    default:
                        // Pas une transaction ADMIN, continuer normalement
                        break;
                }
            }

            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, traiter_transaction, trans->transaction) != 0) {
                perror("Erreur de création du thread");
                free(trans);
            } else {
                pthread_detach(thread_id);
            }
        } 
        // else if (bytes_read == 0) {
        //     printf("FIFO fermée par le client.\n");
        //     free(trans);
        //     break;  // Quitter la boucle si le FIFO est fermé
        // }
        //  else {
        //     perror("Erreur de lecture depuis le FIFO");
        //     free(trans);
        // }
    }
}

void redirectAndExecute(const char* command, int client_fifo_fd) {
    int sfd = dup(STDOUT_FILENO);  // Dupliquer `STDOUT`
    if (sfd == -1) {
        perror("Erreur de duplication de STDOUT");
        return;
    }

    // Rediriger `STDOUT` vers la FIFO client
    if (dup2(client_fifo_fd, STDOUT_FILENO) == -1) {
        perror("Erreur de redirection de STDOUT vers la FIFO client");
        close(sfd);
        return;
    }

    // Exécuter la commande
    system(command);

    // Restaurer `STDOUT`
    if (dup2(sfd, STDOUT_FILENO) == -1) {
        perror("Erreur lors de la restauration de STDOUT");
    }
    close(sfd);
}

