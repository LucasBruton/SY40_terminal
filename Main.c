#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "Terminal.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/types.h>

pthread_t id_bateau, id_portique[2], id_camion[5], id_train;
int shmid_camions, shmid_mutex, shmid_bateaux, shmid_trains;

void arret(int signo) {
    // Libérations des objets IPC
    if (shmctl(shmid_camions, IPC_RMID, 0) == -1) {
        printf("Erreur fermeture du segment de mémoire pour les camions\n");
    }
    if (shmctl(shmid_mutex, IPC_RMID, 0) == -1) {
        printf("Erreur fermeture du segment de mémoire pour les mutex\n");
    }
    if (shmctl(shmid_bateaux, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture du segment de mémoire pour les bateaux\n");
    }
    if (shmctl(shmid_trains, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture du segment de mémoire pour les bateaux\n");
    }
    printf("Fin de l'exécution du terminal...\n");
    exit(0);
}

int main(int argc, char const *argv[]) {
    pthread_t num_thread;
    int nb_camion_par_portique, nb_conteneurs_par_partie_du_bateau, nb_wagon_par_partie_du_train, nb_conteneurs_par_wagon;
    key_t cle;
    stockage_camion * stock_camion;
    args_camions a_camion;
    struct_mutexs * mutexs;
    stockage_bateau * stock_bateau; 
    stockage_train * stock_train;

    // Vérification du nombre d'arguments passés
    if(argc < 5) {
        printf("Usage: %s <nbCamionParPortique> <nbConteneursParPartieDuBateau> <nbWagonsParPartieDuTrain> <nbConteneursParWagon>\n", argv[0]);
        return EXIT_FAILURE;
    }
    signal(SIGINT, arret);

    nb_camion_par_portique = atoi(argv[1]);
    nb_conteneurs_par_partie_du_bateau = atoi(argv[2]);
    nb_wagon_par_partie_du_train = atoi(argv[3]);
    nb_conteneurs_par_wagon = atoi(argv[4]);

    // Vérification des valeurs données en arguments
    if(nb_camion_par_portique <= 0 || nb_camion_par_portique > MAX_CAMION_PORTIQUE) {
        printf("Erreur: l'argument <nbCamionParPortique> doit être compris entre 1 et %d\n", MAX_CAMION_PORTIQUE);
        return EXIT_FAILURE;
    }
    if (nb_conteneurs_par_partie_du_bateau <= 0 || nb_conteneurs_par_partie_du_bateau > MAX_BATEAU_PORTIQUE)
    {
        printf("Erreur: l'argument <nbConteneursParPartieDuBateau> doit être compris entre 1 et %d\n", MAX_BATEAU_PORTIQUE);
        return EXIT_FAILURE;
    }
    if (nb_wagon_par_partie_du_train <= 0 || nb_wagon_par_partie_du_train > MAX_WAGON_TRAIN)
    {
        printf("Erreur: l'argument <nbWagonsParPartieDuTrain> doit être compris entre 1 et %d\n", MAX_WAGON_TRAIN);
        return EXIT_FAILURE;
    }
    if (nb_conteneurs_par_wagon <= 0 || nb_conteneurs_par_wagon > MAX_CONTENEURS_WAGON)
    {
        printf("Erreur: l'argument <nbConteneursParWagon> doit être compris entre 1 et %d\n", MAX_CONTENEURS_WAGON);
        return EXIT_FAILURE;
    }

    // Creation du segment de mémoire pour les mutex
    if ((cle = ftok(FICHIER_MUTEX, 1)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((shmid_mutex = shmget(cle, sizeof(struct_mutexs), IPC_CREAT | 0600)) == -1) {
        printf("Erreur création segment de mémoire pour les camions\n");
        return EXIT_FAILURE;
    }
    // Initialisation des mutex utilisés à travers le programme
    if ((mutexs = (struct_mutexs *)shmat(shmid_mutex, NULL, 0)) == -1)
    {
        printf("Erreur attachement mémoire partagée pour la structure des mutexs\n");
        pthread_exit(NULL);
    }
    pthread_mutex_init(&mutexs->mutex_stockage_camion, NULL);
    pthread_mutex_init(&mutexs->mutex_stockage_bateau, NULL);
    pthread_mutex_init(&mutexs->mutex_stockage_train, NULL);

    // Creation du segment de mémoire pour les camions
    if ((cle = ftok(FICHIER_CAMION, 1)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((shmid_camions = shmget(cle, sizeof(stockage_camion), IPC_CREAT | 0666))==-1) {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Initialisation du segment de mémoire pour les camions
    stock_camion = (stockage_camion *)shmat(shmid_camions, NULL, 0);
    stock_camion->nb_camion_par_portique = nb_camion_par_portique;
    for(int i = 0; i<nb_camion_par_portique; ++i) {
        stock_camion->espace_portique[0][i] = 0;
        stock_camion->espace_portique[1][i] = 0;
    }


    // Creation du segment de mémoire pour les bateaux
    if ((cle = ftok(FICHIER_BATEAU, 1)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((shmid_bateaux = shmget(cle, sizeof(stockage_bateau), IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Initialisation du segment de mémoire pour les bateaux
    stock_bateau = (stockage_bateau *)shmat(shmid_bateaux, NULL, 0);
    stock_bateau->nb_conteneurs_par_partie_du_bateau = nb_camion_par_portique;
    for (int i = 0; i < nb_conteneurs_par_partie_du_bateau; ++i)
    {
        stock_bateau->espace_portique[0][i] = 0;
        stock_bateau->espace_portique[1][i] = 0;
    }

    // Creation du segment de mémoire pour les trains
    if ((cle = ftok(FICHIER_TRAIN, 1)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((shmid_trains = shmget(cle, sizeof(stockage_train), IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Initialisation du segment de mémoire pour les trains
    stock_train = (stockage_train *)shmat(shmid_trains, NULL, 0);
    stock_train->nb_conteneurs_par_wagon = nb_conteneurs_par_wagon;
    stock_train->nb_wagon_par_partie_du_train = nb_wagon_par_partie_du_train;
    for (int i = 0; i < nb_wagon_par_partie_du_train; ++i)
    {
        for (int j = 0; j < nb_conteneurs_par_wagon; ++j) {
            stock_train->espace_portique[0][i][j] = 0;
            stock_train->espace_portique[1][i][j] = 0;
        }
    }

    pthread_create(&id_bateau, NULL, bateau, NULL);
    pthread_create(&id_portique[0], NULL, portique, NULL);
    pthread_create(&id_portique[1], NULL, portique, NULL);

    // Création des threads camions
    for(int j = 1;  j <=2; ++j) {
        a_camion.numVoiePortique = j;
        for (int i = 0; i < nb_camion_par_portique; ++i) {
            a_camion.numEspacePortique = i;
            pthread_create(&id_camion[i], NULL, camion, (void*)&a_camion);
        }
    }


    pthread_create(&id_train, NULL, train, NULL);
    pause();

    return EXIT_SUCCESS;
}
