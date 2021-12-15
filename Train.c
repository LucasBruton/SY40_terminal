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

void *train(void *arg)
{
    printf("Je suis un train\n");
    key_t cle;
    int shmid_mutex, shmid_trains, shmid_depart, random, msgid;
    stockage_train *stock_train;
    struct_mutexs *mutexs;
    debut_superviseur *d_superviseur;
    srand(time(NULL));

    // Récupération du segment de mémoire utilisé pour les mutexs du programme
    if ((cle = ftok(FICHIER, 1)) == -1)
    {
        printf("Erreur ftok\n");
        kill(getpid(), SIGINT);
    }
    if ((shmid_mutex = shmget(cle, 0, 0)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }

    if ((mutexs = (struct_mutexs *)shmat(shmid_mutex, NULL, 0)) == -1)
    {
        printf("Erreur attachement mémoire partagée pour la structure des mutexs\n");
        kill(getpid(), SIGINT);
    }

    // Récupération du segment de mémoire utilisé pour synchronisation du superviseur avec les autres véhicules
    if ((cle = ftok(FICHIER, 2)) == -1)
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

    // Récupération du segment de mémoire pour les trains
    if ((cle = ftok(FICHIER_TRAIN, 1)) == -1)
    {
        printf("Erreur ftok\n");
        kill(getpid(), SIGINT);
    }
    if ((shmid_trains = shmget(cle, 0, 0)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }
    if ((stock_train = (stockage_train *)shmat(shmid_trains, NULL, 0)) == -1)
    {
        printf("Erreur attachement mémoire partagée des conteneurs des camions\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages des trains
    if ((cle = ftok(FICHIER_TRAIN, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid = msgget(cle, 0)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

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
    pthread_mutex_unlock(&mutexs->mutex_stockage_train);

    pthread_mutex_lock(&d_superviseur->mutex);
    d_superviseur->nb_trains++;
    pthread_cond_signal(&d_superviseur->attente_vehicules);
    pthread_mutex_unlock(&d_superviseur->mutex);
    printf("Fin train\n");
}
