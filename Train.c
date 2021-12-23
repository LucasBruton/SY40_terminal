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

// Fonction permettant de créer un train
void *train(void *arg)
{
    key_t cle;
    int shmid_mutex, shmid_trains, random, msgid_trains, msgid_portiques,
        msgid_superviseur, conteneurs_trains = 0, train_rempli, msgid_trains_creation;
    stockage_train *stock_train;
    message_train msg_train;
    message_fin_ordre_superviseur msg_fin_ordre;
    message_portique msg_portique;
    message_retour msg_creation_retour;
    msg_fin_ordre.type = 1;
    msg_fin_ordre.type_transport = CONTENEUR_POUR_TRAIN;
    msg_creation_retour.type = 2;
    srand(time(NULL));

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
    if ((msgid_trains = msgget(cle, 0)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages pour la création des trains
    if ((cle = ftok(FICHIER_TRAIN, 3)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_trains_creation = msgget(cle, 0)) == -1)
    {
        printf("Erreur récupération de la file de messages de la création des trains\n");
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

    // Initalisation des conteneurs du train
    pthread_mutex_lock(&stock_train->mutex);
    train_rempli = 2 * stock_train->nb_conteneurs_par_wagon * stock_train->nb_wagon_par_partie_du_train;
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
    pthread_mutex_unlock(&stock_train->mutex);

    // Envoie d'un message pour confirmer l'initialisation du train
    msgsnd(msgid_trains_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 0);

    // Boucle infini de réception et d'envoi de messages
    while (1)
    {
        // Réception d'un message qui indique si le train doit donner un conteneur à un portique ou s'il reçoit un conteneur
        msgrcv(msgid_trains, &msg_train, sizeof(message_train) - sizeof(long), 1, 0);
        // Si le train donne un conteneur à un portique, il précise au portique où il doit donner le conteneur
        if (msg_train.envoie_conteneur == TRUE)
        {
            // En donnant un conteneur à un portique, l'emplacement du conteneur devient vide
            pthread_mutex_lock(&stock_train->mutex);
            stock_train->espace_portique[msg_train.voie_portique][msg_train.wagon][msg_train.wagon_emplacement] = ESPACE_CONTENEUR_VIDE;
            pthread_mutex_unlock(&stock_train->mutex);
            printf("Le train donne son conteneur à l'emplacement %d du wagon %d qui se trouve sur la voie %d\n", msg_train.wagon_emplacement, msg_train.wagon, msg_train.voie_portique);
            // On donne les informations nécessaires pour que le portique puisse donner le conteneur à la bonne destination
            msg_portique.destinataire = msg_train.destinataire;
            msg_portique.type = msg_train.voie_portique + 1;
            if (msg_train.destinataire == CONTENEUR_POUR_CAMION)
            {
                msg_portique.camion_destinataire = msg_train.camion_destinataire;
            }
            else
            {
                msg_portique.bateau_emplacement = msg_train.bateau_emplacement;
            }
            // On envoie le conteneur à un portique
            msgsnd(msgid_portiques, &msg_portique, sizeof(message_portique) - sizeof(long), 0);
        }
        else
        {
            // Réception d'un conteneur
            pthread_mutex_lock(&stock_train->mutex);
            printf("Le train a reçu un conteneur pour l'emplacement %d du wagon %d de la voie du portique %d\n", msg_train.wagon_emplacement, msg_train.wagon, msg_train.voie_portique);
            // On compte le nombre de conteneurs à destination du train que le train possède
            conteneurs_trains++;
            // On place le conteneur à un emplacement précisé par le portique
            stock_train->espace_portique[msg_train.voie_portique][msg_train.wagon][msg_train.wagon_emplacement] = CONTENEUR_POUR_TRAIN;
            /* 
            On regarde si le train est plein de conteneurs à destination du train
            Si le train est plein, il quitte le terminal de transport
            */
            if (conteneurs_trains == train_rempli)
            {
                msg_fin_ordre.plein_conteneurs = TRUE;
                printf("Le train quitte le terminal de transport\n");
            }
            else
            {
                msg_fin_ordre.plein_conteneurs = FALSE;
            }
            msgsnd(msgid_superviseur, &msg_fin_ordre, sizeof(message_fin_ordre_superviseur) - sizeof(long), 0);
            if (conteneurs_trains == train_rempli)
            {
                pthread_mutex_unlock(&stock_train->mutex);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&stock_train->mutex);
        }
    }
}
