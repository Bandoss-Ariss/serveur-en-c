//#########################################################
//#
//# Titre : UTILITAIRES (MAIN) TP1 LINUX Automne 24
//#             SIF-1015 - Système d'exploitation
//#             Université du Québec à Trois-Rivières
//#
//# Auteur : Francois Meunier
//# Date : Septembre 2024
//#
//# Langage : ANSI C on LINUX
//#
//#########################################################

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#include <pthread.h>

// Mutex pour protéger l'accès à la liste chaînée et aux pointeurs de tête et queue
pthread_mutex_t mutex_liste = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_tete_queue = PTHREAD_MUTEX_INITIALIZER;

// Mutex pour protéger l'accès à la console
pthread_mutex_t mutex_console = PTHREAD_MUTEX_INITIALIZER;

// Mutex pour l'accès au système de fichiers
pthread_mutex_t mutex_fichier = PTHREAD_MUTEX_INITIALIZER;

pthread_rwlock_t rwlock_liste = PTHREAD_RWLOCK_INITIALIZER;


// Définition des structures infoVM et noeudVM pour la liste chaînée de VMs
struct infoVM {
    int noVM;                     // Numéro de la VM
    unsigned char busy;           // Indicateur d'occupation (0 = libre, 1 = occupé)
    int tid;                      // ID du thread associé
    unsigned short *ptrDebutVM;   // Pointeur vers le début de la mémoire de la VM
    unsigned short *offsetDebutCode; // Début de la région mémoire en lecture seule (code)
    unsigned short *offsetFinCode;   // Fin de la région mémoire en lecture seule (code)
};

struct noeudVM {
    struct infoVM VM;             // Structure contenant les informations de la VM
    struct noeudVM *suivant;      // Pointeur vers le nœud suivant dans la liste chaînée
};
// Prototype des fonctions de gestion de la liste chaînée
struct noeudVM* addVM();
void removeVM(int noVM);
void printVMs();

// Pointeurs pour la liste chaînée de VMs
struct noeudVM* head = NULL;
struct noeudVM* queue = NULL;
int nbVM = 0; // Compteur pour le numéro de VM

// Variable pour le rôle d'utilisateur : true pour ADMIN, false pour non-ADMIN
bool is_admin = false;

void* traiter_transaction(void* arg);

// Fonction principale
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fichier_transactions>\n", argv[0]);
        exit(1);
    }

    char* nomFichier = argv[1];
    FILE* fichier = fopen(nomFichier, "r");
    if (fichier == NULL) {
        perror("Erreur d'ouverture du fichier de transactions");
        exit(1);
    }

    char ligne[100];
    pthread_t thread_id;

    // Lecture de chaque ligne de transaction dans le fichier
    while (fgets(ligne, sizeof(ligne), fichier)) {
        // Suppression du saut de ligne en fin de ligne
        ligne[strcspn(ligne, "\n")] = '\0';

        // Copie de la ligne de transaction pour la passer au thread
        char* transaction = strdup(ligne); 
        if (pthread_create(&thread_id, NULL, traiter_transaction, transaction) != 0) {
            perror("Erreur lors de la création du thread");
            free(transaction);
            continue;
        }

        // Détacher le thread pour qu'il soit libéré automatiquement à la fin
        pthread_detach(thread_id);
    }

    fclose(fichier);
    return 0;
}

// Fonction pour ajouter une VM à la liste chaînée
struct noeudVM* addVM() {
    pthread_rwlock_wrlock(&rwlock_liste); // Verrou d'écriture pour ajouter une VM
    struct noeudVM* nouveau_noeud = (struct noeudVM*)malloc(sizeof(struct noeudVM));
    if (nouveau_noeud == NULL) {
        fprintf(stderr, "Erreur d'allocation de mémoire pour une nouvelle VM.\n");
        pthread_rwlock_unlock(&rwlock_liste); // Déverrouiller avant de sortir
        exit(1);
    }

    // Initialiser les informations de la VM
    nouveau_noeud->VM.noVM = ++nbVM;
    nouveau_noeud->VM.busy = 0;
    nouveau_noeud->VM.tid = 0;
    nouveau_noeud->VM.ptrDebutVM = NULL;
    nouveau_noeud->VM.offsetDebutCode = NULL;
    nouveau_noeud->VM.offsetFinCode = NULL;
    nouveau_noeud->suivant = NULL;

    // Ajout en fin de liste
    if (head == NULL) {
        head = queue = nouveau_noeud;
    } else {
        queue->suivant = nouveau_noeud;
        queue = nouveau_noeud;
    }

    printf("Virtual Machine ajoutée noVM: %d\n", nouveau_noeud->VM.noVM);
    pthread_rwlock_unlock(&rwlock_liste); // Déverrouillage d'écriture
    return nouveau_noeud;
}
// Fonction pour supprimer une VM de la liste chaînée
void removeVM(int noVM) {
    pthread_rwlock_wrlock(&rwlock_liste); // Verrou d'écriture pour supprimer une VM
    struct noeudVM* actuel = head;
    struct noeudVM* precedent = NULL;

    while (actuel != NULL && actuel->VM.noVM != noVM) {
        precedent = actuel;
        actuel = actuel->suivant;
    }

    if (actuel == NULL) {
        printf("VM %d introuvable.\n", noVM);
        pthread_rwlock_unlock(&rwlock_liste);
        return;
    }

    if (precedent == NULL) {
        head = actuel->suivant;
    } else {
        precedent->suivant = actuel->suivant;
    }

    if (actuel == queue) {
        queue = precedent;
    }

    free(actuel);
    printf("VM %d supprimée.\n", noVM);
    pthread_rwlock_unlock(&rwlock_liste); // Déverrouillage d'écriture
}

// Fonction pour afficher toutes les VMs dans la liste chaînée
void printVMs() {
    pthread_rwlock_rdlock(&rwlock_liste); // Verrou de lecture pour lister les VMs
    printf("noVM\tBusy\tTID\tDebutVM\t\tDebutCode\tFinCode\n");
    printf("==========================================================\n");
    struct noeudVM* actuel = head;
    while (actuel != NULL) {
        printf("%d\t%d\t%d\t%p\t%p\t%p\n",
               actuel->VM.noVM,
               actuel->VM.busy,
               actuel->VM.tid,
               (void*)actuel->VM.ptrDebutVM,
               (void*)actuel->VM.offsetDebutCode,
               (void*)actuel->VM.offsetFinCode);
        actuel = actuel->suivant;
    }
    pthread_rwlock_unlock(&rwlock_liste); // Déverrouillage de lecture
}
void* traiter_transaction(void* arg) {
    char* transaction = (char*)arg;

    printf("Traitement de la transaction : %s\n", transaction);

    char* tok;
    char* sp;
    tok = strtok_r(transaction, " ", &sp);

    if (tok == NULL) {
        free(transaction);
        return NULL;
    }

    switch (tok[0]) {
        case 'B':
        case 'b': {
            // Transaction B : Liste les fichiers .olc3 (non-ADMIN uniquement)
          
                listOlcFile(); // Afficher la liste des fichiers .olc3
            
           
            break;
        }

        case 'L':
        case 'l': {
            // Transaction L : Liste les VMs dans une plage (ADMIN uniquement)
            if (is_admin) {
                int nstart = atoi(strtok_r(NULL, "-", &sp));
                int nend = atoi(strtok_r(NULL, " ", &sp));
                //Appel de la fonction associée
				listItems(nstart, nend); // Lister les VM
				break;
            } else {
                printf("Erreur : Transaction L réservée aux ADMIN.\n");
            }
            break;
        }

        case 'X':
        case 'x': {
            // Transaction X : Exécute un fichier .olc3 (ADMIN et non-ADMIN)
            int p = atoi(strtok_r(NULL, " ", &sp));
            char *nomfich = strtok_r(NULL, "\n", &sp);
            executeFile(p, nomfich); // Exécute le fichier spécifié
            break;
        }

        case 'E':
        case 'e': {
            // Transaction E : Supprime une VM (ADMIN uniquement)
            if (is_admin) {
                int noVM = atoi(strtok_r(NULL, " ", &sp));
                removeVM(noVM); // Supprime la VM spécifiée
            } else {
                printf("Erreur : Transaction E réservée aux ADMIN.\n");
            }
            break;
        }

        case 'K':
        case 'k': {
            // Transaction K : Terminer l'exécution d'un fichier olc3 (ADMIN uniquement)
            if (is_admin) {
                int tid = atoi(strtok_r(NULL, " ", &sp));
                killThread(tid); // Tuer le thread spécifié par son TID
            } else {
                printf("Erreur : Transaction K réservée aux ADMIN.\n");
            }
            break;
        }

        default:
            printf("Erreur : Type de transaction inconnu (%s).\n", tok);
            break;
    }

    free(transaction);
    return NULL;
}
