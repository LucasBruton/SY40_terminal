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
    key_t cle;
    int shmid_mutex, shmid_camions, random, msgid_camions[2],
        msgid_superviseur, msgid_portiques, conteneur, msgid_camions_creation, 
        voie_portique, emplacement_portique;
    stockage_camion *stock_camion;
    message_camion msg_camion;
    message_portique msg_portique;
    message_fin_ordre_superviseur msg_fin_ordre;
    message_creation_camion msg_creation_camion;
    message_creation_retour msg_creation_retour;
    msg_creation_retour.type = 2;
    msg_fin_ordre.type = 1;
    msg_fin_ordre.type_transport = CONTENEUR_POUR_CAMION;
    msg_fin_ordre.plein_conteneurs = TRUE;

    srand(time(NULL));

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

    // Récupération de la file de messages pour la création des camions
    if ((cle = ftok(FICHIER_CAMION, 4)) == -1)
    {
        printf("Erreur ftok\n");
        kill(getpid(), SIGINT);
    }
    if ((msgid_camions_creation = msgget(cle, 0)) == -1)
    {
        printf("Erreur récupération de la file de messages de la création des camions\n");
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
    if ((msgid_portiques = msgget(cle, 0)) == -1)
    {
        printf("Erreur récupération de la file de messages pour les portiques\n");
        kill(getpid(), SIGINT);
    }

    // Initialisation du conteneur du camion
    random = rand() % 4;
    if (random < 2)
    {
        conteneur = ESPACE_CONTENEUR_VIDE;
    }
    else if (random == 2)
    {
        conteneur = CONTENEUR_POUR_BATEAU;
    }
    else
    {
        conteneur = CONTENEUR_POUR_TRAIN;
    }

    msgrcv(msgid_camions_creation, &msg_creation_camion, sizeof(message_creation_camion) - sizeof(long), 1, 0);
    pthread_mutex_lock(&stock_camion->mutex);
    stock_camion->espace_portique[msg_creation_camion.voie_portique][msg_creation_camion.emplacement_portique] = conteneur;
    pthread_mutex_unlock(&stock_camion->mutex);
    msgsnd(msgid_camions_creation, &msg_creation_retour, sizeof(message_creation_retour) - sizeof(long), 0);
    // if(msg_creation_camion.attente == TRUE) {
    //     voie_portique
    // }
    while (1)
    {
        msgrcv(msgid_camions[msg_creation_camion.voie_portique], &msg_camion, sizeof(message_camion) - sizeof(long), msg_creation_camion.emplacement_portique + 1, 0);
        if (msg_camion.envoie_conteneur == TRUE)
        {
            pthread_mutex_lock(&stock_camion->mutex);
            stock_camion->espace_portique[msg_creation_camion.voie_portique][msg_creation_camion.emplacement_portique] = ESPACE_CONTENEUR_VIDE;
            conteneur = ESPACE_CONTENEUR_VIDE;
            pthread_mutex_unlock(&stock_camion->mutex);
            printf("Le camion de la voie du portique %d à l'emplacement %d donne son conteneur au portique\n", msg_creation_camion.voie_portique, msg_creation_camion.emplacement_portique);
            msg_portique.destinataire = msg_camion.desinataire;
            if (msg_camion.desinataire == CONTENEUR_POUR_BATEAU)
            {
                msg_portique.bateau_emplacement = msg_camion.bateau_emplacement;
            }
            else
            {
                msg_portique.train_wagon = msg_camion.train_wagon;
                msg_portique.wagon_emplacement = msg_camion.wagon_emplacement;
            }
            msg_portique.type = msg_creation_camion.voie_portique + 1;
            msgsnd(msgid_portiques, &msg_portique, sizeof(message_portique) - sizeof(long), 0);
        }
        else
        {
            pthread_mutex_lock(&stock_camion->mutex);
            printf("Le camion de la voie du portique %d à l'emplacement %d a reçu un conteneur\n", msg_creation_camion.voie_portique, msg_creation_camion.emplacement_portique);
            stock_camion->espace_portique[msg_creation_camion.voie_portique][msg_creation_camion.emplacement_portique] = CONTENEUR_POUR_CAMION;
            pthread_mutex_unlock(&stock_camion->mutex);
            printf("Le camion de la voie du portique %d à l'emplacement %d quitte le terminal de transport\n", msg_creation_camion.voie_portique, msg_creation_camion.emplacement_portique);
            msg_fin_ordre.camion_emplacement = msg_creation_camion.emplacement_portique;
            msg_fin_ordre.camion_voie_portique = msg_creation_camion.voie_portique;
            msgsnd(msgid_superviseur, &msg_fin_ordre, sizeof(message_fin_ordre_superviseur) - sizeof(long), 0);
            pthread_exit(NULL);
        }
    }
}
