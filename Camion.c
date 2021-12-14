#include <stdlib.h>
#include <stdio.h>
#include "Terminal.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>

void *camion(void *arg) {
    printf("Je suis un camion\n");
    args_camions *a_camions = (args_camions*)arg;
    key_t cle;
    int shmid_mutex, shmid_camions, random;
    stockage_camion *stock_camion;
    struct_mutexs* mutexs;
    srand(time(NULL));

    if ((cle = ftok(FICHIER_MUTEX, 1)) == -1)
    {
        printf("Erreur ftok\n");
        pthread_exit(NULL);
    }
    if ((shmid_mutex = shmget(cle, 0, 0)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        pthread_exit(NULL);
    }

    if((mutexs = (struct_mutexs*)shmat(shmid_mutex, NULL, 0)) == -1) {
        printf("Erreur attachement mémoire partagée pour la structure des mutexs\n");
        pthread_exit(NULL);
    }

    // Récupération du segment de mémoire pour les camions
    if ((cle = ftok(FICHIER_CAMION, 1)) == -1)
    {
        printf("Erreur ftok\n");
        pthread_exit(NULL);
    }
    if ((shmid_camions = shmget(cle, 0, 0)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        pthread_exit(NULL);
    }

    stock_camion = (stockage_camion *)shmat(shmid_camions, NULL, 0);
    random = rand()%4;

    pthread_mutex_lock(&mutexs->mutex_stockage_camion);
    if(random < 2) {
        stock_camion->espace_portique[a_camions->numVoiePortique][a_camions->numEspacePortique] = ESPACE_CONTENEUR_VIDE;
    }else if(random == 2) {
        stock_camion->espace_portique[a_camions->numVoiePortique][a_camions->numEspacePortique] = CONTENEUR_POUR_BATEAU;
    }else {
        stock_camion->espace_portique[a_camions->numVoiePortique][a_camions->numEspacePortique] = CONTENEUR_POUR_TRAIN;
    }
    pthread_mutex_unlock(&mutexs->mutex_stockage_camion);
    
}
