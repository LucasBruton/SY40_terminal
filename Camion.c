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
#include <sys/msg.h>

void *camion(void *arg) {
    args_camions *a_camions = (args_camions*)arg;
    key_t cle;
    int shmid_mutex, shmid_camions, shmid_depart, random, msgid_camions[2], msgid_superviseur, msgid_portiques;
    stockage_camion *stock_camion;
    debut_superviseur *d_superviseur;
    message_camion msg_camion;
    message_portique msg_portique;
    message_fin_ordre_superviseur msg_fin_ordre;
    msg_fin_ordre.type = 1;
    msg_fin_ordre.type_transport = CONTENEUR_POUR_CAMION;
    msg_fin_ordre.camion_emplacement = a_camions->numEspacePortique;
    msg_fin_ordre.camion_voie_portique = a_camions->numVoiePortique;
    msg_fin_ordre.plein_conteneurs = TRUE;
    msg_portique.type = a_camions->numVoiePortique+1;

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

    // Récupération de la 1e file de messages pour les camions
    if ((cle = ftok(FICHIER_CAMION, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions[0] = msgget(cle, 0)) == -1)
    {
        printf("Erreur récupération de la 1e file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la 2e file de messages pour les camions
    if ((cle = ftok(FICHIER_CAMION, 3)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions[1] = msgget(cle, 0)) == -1)
    {
        printf("Erreur récupération de la 2e file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages du supervisuer
    if ((cle = ftok(FICHIER, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_superviseur = msgget(cle, 0)) == -1)
    {
        printf("Erreur récupération de la file de messages pour le supervisuer\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages pour les portiques
    if ((cle = ftok(FICHIER_PORTIQUE, 1)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_portiques = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur récupération de la file de messages pour les portiques\n");
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

    while(1) {
        msgrcv(msgid_camions[a_camions->numVoiePortique], &msg_camion, sizeof(message_camion) - sizeof(long), a_camions->numEspacePortique+1, 0);
        if(msg_camion.envoie_conteneur == TRUE) {
            pthread_mutex_lock(&stock_camion->mutex);
            stock_camion->espace_portique[a_camions->numVoiePortique][a_camions->numEspacePortique] = ESPACE_CONTENEUR_VIDE;
            pthread_mutex_unlock(&stock_camion->mutex);
            printf("Le camion de la voie du portique %d à l'emplacement %d donne son conteneur au portique\n", a_camions->numVoiePortique, a_camions->numEspacePortique);
            msg_portique.destinataire = msg_camion.desinataire;
            if(msg_camion.desinataire == CONTENEUR_POUR_BATEAU) {
                msg_portique.bateau_emplacement = msg_camion.bateau_emplacement;
            }else {
                msg_portique.train_wagon = msg_camion.train_wagon;
                msg_portique.wagon_emplacement = msg_camion.wagon_emplacement;
            }
            msgsnd(msgid_portiques, &msg_portique, sizeof(message_portique) - sizeof(long), 0);
        }else {
            pthread_mutex_lock(&stock_camion->mutex);
            printf("Le camion de la voie du portique %d à l'emplacement %d a reçu un conteneur\n", a_camions->numVoiePortique, a_camions->numEspacePortique);
            stock_camion->espace_portique[a_camions->numVoiePortique][a_camions->numEspacePortique] = CONTENEUR_POUR_CAMION;
            pthread_mutex_unlock(&stock_camion->mutex);
            printf("Le camion de la voie du portique %d à l'emplacement %d quitte le terminal de transport\n", a_camions->numVoiePortique, a_camions->numEspacePortique);
            pthread_mutex_lock(&d_superviseur->mutex);
            d_superviseur->nb_camions--;
            pthread_cond_signal(&d_superviseur->attente_vehicules);
            pthread_mutex_unlock(&d_superviseur->mutex);
            msgsnd(msgid_superviseur, &msg_fin_ordre, sizeof(message_fin_ordre_superviseur) - sizeof(long), 0);
            pthread_exit(NULL);
        }
    }
}
