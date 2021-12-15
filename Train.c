#include <stdlib.h>
#include <stdio.h>
#include "Terminal.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>

void *train(void *arg)
{
    printf("Je suis un train\n");
    key_t cle;
    int shmid_mutex, shmid_trains, random;
    stockage_train *stock_train;
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

    // Récupération du segment de mémoire pour les trains
    if ((cle = ftok(FICHIER_TRAIN, 1)) == -1)
    {
        printf("Erreur ftok\n");
        pthread_exit(NULL);
    }
    if ((shmid_trains = shmget(cle, 0, 0)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        pthread_exit(NULL);
    }

    stock_train = (stockage_train *)shmat(shmid_trains, NULL, 0);

    // Initalisation des conteneurs du train
    pthread_mutex_lock(&mutexs->mutex_stockage_train);
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < stock_train->nb_wagon_par_partie_du_train; ++j)
        {
            for (int k = 0; k < stock_train->nb_conteneurs_par_wagon; ++k)
            {
                random = rand() % 4;

                if (random < 2)
                {
                    stock_train->espace_portique[i][j][k] = ESPACE_CONTENEUR_VIDE;
                }
                else if (random == 2)
                {
                    stock_train->espace_portique[i][j][k] = CONTENEUR_POUR_CAMION;
                }
                else
                {
                    stock_train->espace_portique[i][j][k] = CONTENEUR_POUR_BATEAU;
                }

            }
        }
    }
    pthread_mutex_unlock(&mutexs->mutex_stockage_bateau);
}
