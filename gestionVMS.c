//#########################################################
//#
//# Titre : 	Utilitaires CVS LINUX Automne 24
//#				SIF-1015 - Système d'exploitation
//#				Université du Québec à Trois-Rivières
//#
//# Auteur : 	Francois Meunier
//#	Date :		Septembre 2024
//#
//# Langage : 	ANSI C on LINUX 
//#
//#######################################

#include "gestionListeChaineeVMS.h"
#include "gestionVMS.h"


//#######################################
//#
//# fonction utilisée pour le traitement  des transactions
//# ENTREE: Nom de fichier de transactions 
//# SORTIE: 
void* readTransd(char* nomFichier){
	FILE *f;
	char buffer[100];
	char *tok, *sp;

	//Ouverture du fichier en mode "r" (equiv. "rt") : [r]ead [t]ext
	f = fopen(nomFichier, "rt");
	if (f==NULL)
		error(2, "readTrans: Erreur lors de l'ouverture du fichier.");

	//Lecture (tentative) d'une ligne de texte
	fgets(buffer, 100, f);

	//Pour chacune des lignes lues
	while(!feof(f)){

		//Extraction du type de transaction
		tok = strtok_r(buffer, " ", &sp);

		//Branchement selon le type de transaction
		switch(tok[0]){
			case 'E':
			case 'e':{
				//Extraction du paramètre
				int noVM = atoi(strtok_r(NULL, " ", &sp));
				//Appel de la fonction associée
				removeItem(noVM); // Eliminer une VM
				break;
				}
			case 'L':
			case 'l':{
				//Extraction des paramètres
				int nstart = atoi(strtok_r(NULL, "-", &sp));
				int nend = atoi(strtok_r(NULL, " ", &sp));
				//Appel de la fonction associée
				listItems(nstart, nend); // Lister les VM
				break;
			}
			case 'B': // affiche liste des fichiers .olc
			case 'b':{					
				listOlcFile();	// Afficher la liste de fichiers binaires .olc		
				break;
			}
			case 'P':
			case 'p':{
				processList(); // Afficher la liste des process
				break;
			}
			case 'K':
			case 'k':{
				//Extraction du paramètre
				int tid = atoi(strtok_r(NULL, " ", &sp));
				//Appel de la fonction associée
				killThread(tid); // Kill d'un thread qui s'execute sur une VM
				break;
				}
			case 'X':
			case 'x':{
				//Appel de la fonction associée
				int p = atoi(strtok_r(NULL, " ", &sp));
				char *nomfich = strtok_r(NULL, "\n", &sp);
				executeFile(p, nomfich); // Executer le code binaire du fichier nomFich 
				break;
				}
		}
		//Lecture (tentative) de la prochaine ligne de texte
		fgets(buffer, 100, f);
	}
	//Fermeture du fichier
	fclose(f);
	//Retour
	return NULL;
}


