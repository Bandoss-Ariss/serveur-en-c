#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

#include <stdint.h>
#include <signal.h>
/* unix */
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>
#include <sys/resource.h>


struct infoVM{						
	int		noVM;
	unsigned char 	busy;
	int  	tid;
	int		DimRam;
	int		DimRamUsed;
	uint16_t * 	ptrDebutVM;	
	uint16_t   	offsetDebutCode; // region memoire ReadOnly
	uint16_t   	offsetFinCode;
	};								 

struct noeudVM{			
	struct infoVM	VM;		
	struct noeudVM		*suivant;	
	};	
	
void cls(void);
void error(const int exitcode, const char * message);

// gestionListeChaineeVMS.h
#ifndef GESTIONLISTECHAINEEVMS_H
#define GESTIONLISTECHAINEEVMS_H

#include "gestionVMS.h"

extern struct noeudVM* head;  // Pointeur de tête de la liste
extern struct noeudVM* queue; // Pointeur de queue de la liste
extern int nbVM;              // Nombre de VMs actives



struct noeudVM * findItem(const int no);
struct noeudVM * findFreeVM();
struct noeudVM * findPrev(const int no);

struct noeudVM * addItem();
void listOlcFile(); // fichier binaire
void removeItem(const int noVM);
void listItems(const int start, const int end);
void saveItems(const char* sourcefname);
//int executeFile(int noVM, char* sourcefname);
int executeFile(int p, char* sourcefname);
void killThread(int tid);

void serializeInfoVM(char* buffer, struct infoVM* vm);

void* readTransd(char* nomFichier);

#endif
