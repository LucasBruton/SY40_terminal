#include <stdlib.h>
#include <stdio.h>
#include "Terminal.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>

void *bateau(void *arg)
{
    printf("Je suis un bateau\n");
    key_t cle;
    int shmid_mutex, shmid_bateaux, random;
    stockage_bateau *stock_bateaux;
    struct_mutexs *mutexs;
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

    if ((mutexs = (struct_mutexs *)shmat(shmid_mutex, NULL, 0)) == -1)
    {
        printf("Erreur attachement mémoire partagée pour la structure des mutexs\n");
        pthread_exit(NULL);
    }

    // Récupération du segment de mémoire pour les camions
    if ((cle = ftok(FICHIER_BATEAU, 1)) == -1)
    {
        printf("Erreur ftok\n");
        pthread_exit(NULL);
    }
    if ((shmid_bateaux = shmget(cle, 0, 0)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        pthread_exit(NULL);
    }
    stock_bateaux = (stockage_bateau *)shmat(shmid_bateaux, NULL, 0);

    // Initialisation des conteneurs du bateau
    pthread_mutex_lock(&mutexs->mutex_stockage_bateau);
    for(int i = 0; i<2; ++i) {
        for (int j = 0; j < stock_bateaux->nb_conteneurs_par_partie_du_bateau; ++j)
        {
            random = rand()%4;
            if (random < 2)
            {
                stock_bateaux->espace_portique[i][j] = ESPACE_CONTENEUR_VIDE;
            }
            else if (random == 2)
            {
                stock_bateaux->espace_portique[i][j] = CONTENEUR_POUR_TRAIN;
            }
            else
            {
                stock_bateaux->espace_portique[i][j] = CONTENEUR_POUR_CAMION;
            } 
        }
    }
    pthread_mutex_unlock(&mutexs->mutex_stockage_bateau);
}
