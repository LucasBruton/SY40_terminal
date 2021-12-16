#include <stdlib.h>
#include <stdio.h>
#include "Terminal.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

void *camion(void *arg) {
    printf("Je suis un camion\n");
    args_camions *a_camions = (args_camions*)arg;
    key_t cle;
    int shmid_mutex, shmid_camions, shmid_depart, random, msgid;
    stockage_camion *stock_camion;
    debut_superviseur *d_superviseur;
    srand(time(NULL));

    // Récupération du segment de mémoire utilisé pour synchronisation du superviseur avec les autres véhicules
    if ((cle = ftok(FICHIER, 1)) == -1)
    {
        printf("Erreur ftok\n");
        kill(getpid(), SIGINT);
    }
    if ((shmid_depart = shmget(cle, 0, 0)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }

    if ((d_superviseur = (debut_superviseur *)shmat(shmid_depart, NULL, 0)) == -1)
    {
        printf("Erreur attachement mémoire partagée pour la structure des mutexs\n");
        kill(getpid(), SIGINT);
    }

    // Récupération du segment de mémoire pour les camions
    if ((cle = ftok(FICHIER_CAMION, 1)) == -1)
    {
        printf("Erreur ftok\n");
        kill(getpid(), SIGINT);
    }
    if ((shmid_camions = shmget(cle, 0, 0)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }

    if((stock_camion = (stockage_camion *)shmat(shmid_camions, NULL, 0))==-1) {
        printf("Erreur attachement mémoire partagée des conteneurs des camions\n");
        kill(getpid(), SIGINT);
    }
    // Récupération de la file de messages des camions
    if ((cle = ftok(FICHIER_BATEAU, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid = msgget(cle, 0)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Initialisation du conteneur du camion
    random = rand()%4;
    pthread_mutex_lock(&stock_camion->mutex);
    if(random < 2) {
        stock_camion->espace_portique[a_camions->numVoiePortique][a_camions->numEspacePortique] = ESPACE_CONTENEUR_VIDE;
    }else if(random == 2) {
        stock_camion->espace_portique[a_camions->numVoiePortique][a_camions->numEspacePortique] = CONTENEUR_POUR_BATEAU;
    }else {
        stock_camion->espace_portique[a_camions->numVoiePortique][a_camions->numEspacePortique] = CONTENEUR_POUR_TRAIN;
    }
    pthread_mutex_unlock(&stock_camion->mutex);

    // Ajout d'un camion aux nombre de camions qui sont prets
    pthread_mutex_lock(&d_superviseur->mutex);
    d_superviseur->nb_camions++;
    pthread_cond_signal(&d_superviseur->attente_vehicules);
    pthread_mutex_unlock(&d_superviseur->mutex);

    printf("Fin camion\n");
}
