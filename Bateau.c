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

void *bateau(void *arg)
{
    key_t cle;
    int shmid_bateaux, shmid_depart, random, msgid_bateaux, msgid_portiques,
        conteneurs_bateau = 0, bateau_rempli, msgid_superviseur;
    stockage_bateau *stock_bateaux;
    debut_superviseur *d_superviseur;
    message_bateau msg_bateau;
    message_fin_ordre_superviseur msg_fin_ordre;
    message_portique msg_portique;
    msg_fin_ordre.type = 1;
    msg_fin_ordre.type_transport = CONTENEUR_POUR_BATEAU;
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

    // Récupération du segment de mémoire pour les bateaux
    if ((cle = ftok(FICHIER_BATEAU, 1)) == -1)
    {
        printf("Erreur ftok\n");
        kill(getpid(), SIGINT);
    }
    if ((shmid_bateaux = shmget(cle, 0, 0)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }
    if ((stock_bateaux = (stockage_bateau *)shmat(shmid_bateaux, NULL, 0)) == -1)
    {
        printf("Erreur attachement mémoire partagée des conteneurs des camions\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages des bateaux
    if ((cle = ftok(FICHIER_BATEAU, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_bateaux = msgget(cle, 0)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages pour le superviseur
    if ((cle = ftok(FICHIER, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_superviseur = msgget(cle, 0)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
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

    // Initialisation des conteneurs du bateau
    pthread_mutex_lock(&stock_bateaux->mutex);
    bateau_rempli = 2 * stock_bateaux->nb_conteneurs_par_partie_du_bateau;
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
    pthread_mutex_unlock(&stock_bateaux->mutex);

    // Ajout d'un bateau aux nombre de bateaux qui sont prets
    pthread_mutex_lock(&d_superviseur->mutex);
    d_superviseur->nb_bateaux++;
    pthread_cond_signal(&d_superviseur->attente_vehicules);
    pthread_mutex_unlock(&d_superviseur->mutex);

    while(1) {
        msgrcv(msgid_bateaux, &msg_bateau, sizeof(message_bateau) - sizeof(long), 1, 0);
        if(msg_bateau.envoie_conteneur == TRUE) {
            pthread_mutex_lock(&stock_bateaux->mutex);
            stock_bateaux->espace_portique[msg_bateau.voie_portique][msg_bateau.emplacement_conteneur] = ESPACE_CONTENEUR_VIDE;
            pthread_mutex_unlock(&stock_bateaux->mutex);
            printf("Le bateau donne son conteneur de la voie %d à l'emplacement %d\n", msg_bateau.voie_portique, msg_bateau.emplacement_conteneur);
            msg_portique.destinataire = msg_bateau.destinataire;
            msg_portique.type = msg_bateau.voie_portique + 1;
            if(msg_bateau.destinataire == CONTENEUR_POUR_CAMION) {
                msg_portique.camion_destinataire = msg_bateau.camion_destinataire;
            }else {
                msg_portique.train_wagon = msg_bateau.train_wagon;
                msg_portique.wagon_emplacement = msg_bateau.wagon_emplacement;
            }
            msgsnd(msgid_portiques, &msg_portique, sizeof(message_portique) - sizeof(long), 0);
        }else {
            pthread_mutex_lock(&stock_bateaux->mutex);
            printf("Le bateau a reçu un conteneur pour la voie du portique %d à l'emplacement %d\n", msg_bateau.voie_portique, msg_bateau.envoie_conteneur);
            conteneurs_bateau++;
            stock_bateaux->espace_portique[msg_bateau.voie_portique][msg_bateau.emplacement_conteneur] = CONTENEUR_POUR_BATEAU;
            if(conteneurs_bateau == bateau_rempli) {
                msg_fin_ordre.plein_conteneurs = TRUE;
                printf("Le bateau quitte le terminal de transport\n");
                pthread_mutex_lock(&d_superviseur->mutex);
                d_superviseur->nb_bateaux--;
                pthread_cond_signal(&d_superviseur->attente_vehicules);
                pthread_mutex_unlock(&d_superviseur->mutex);
            }else {
                msg_fin_ordre.plein_conteneurs = FALSE;
            }
            msgsnd(msgid_superviseur, &msg_fin_ordre, sizeof(message_fin_ordre_superviseur) - sizeof(long), 0); 
            if (conteneurs_bateau == bateau_rempli)
            {
                pthread_mutex_unlock(&stock_bateaux->mutex);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&stock_bateaux->mutex);
        }
    }

}
