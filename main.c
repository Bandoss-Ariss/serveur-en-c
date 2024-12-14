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