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
#include <string.h>

// Fonction permettant de créer un bateau
void *bateau(void *arg)
{
    key_t cle;
    int shmid_bateaux, random, msgid_bateaux, msgid_portiques,
        conteneurs_bateau = 0, bateau_rempli, msgid_superviseur,
        msgid_bateaux_creation;
    stockage_bateau *stock_bateaux;
    message_bateau msg_bateau;
    message_fin_ordre_superviseur msg_fin_ordre;
    message_portique msg_portique;
    message_retour msg_creation_retour;
    msg_fin_ordre.type = 1;
    msg_fin_ordre.type_transport = CONTENEUR_POUR_BATEAU;
    msg_creation_retour.type = 2;
    srand(time(NULL));

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
        printf("Erreur récupération de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages pour la création des bateaux
    if ((cle = ftok(FICHIER_BATEAU, 3)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_bateaux_creation = msgget(cle, 0)) == -1)
    {
        printf("Erreur récupération de la file de messages de la création des bateaux\n");
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
    bateau_rempli = 2 * stock_bateaux->nb_conteneurs_par_partie_du_bateau;
    pthread_mutex_lock(&stock_bateaux->mutex);
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

    // Envoi d'un mmessage pour confirmer l'initialisation du bateau
    msgsnd(msgid_bateaux_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 0);

    // Boucle infini de réception et d'envoi de messages
    while(1) {
        // Réception d'un message qui indique si le bateau doit donner un conteneur à un portique ou s'il reçoit un conteneur
        msgrcv(msgid_bateaux, &msg_bateau, sizeof(message_bateau) - sizeof(long), 1, 0);
        // Si le bateau donne un conteneur à un portique, il précise au portique où il doit donner le conteneur
        if(msg_bateau.envoie_conteneur == TRUE) {
            // En donnant un conteneur à un portique, l'emplacement du conteneur devient vide
            pthread_mutex_lock(&stock_bateaux->mutex);
            stock_bateaux->espace_portique[msg_bateau.voie_portique][msg_bateau.emplacement_conteneur] = ESPACE_CONTENEUR_VIDE;
            pthread_mutex_unlock(&stock_bateaux->mutex);
            printf("Le bateau donne son conteneur de la voie %d à l'emplacement %d\n", msg_bateau.voie_portique, msg_bateau.emplacement_conteneur);
            // On donne les informations nécessaires pour que le portique puisse donner le conteneur à la bonne destination
            msg_portique.destinataire = msg_bateau.destinataire;
            msg_portique.type = msg_bateau.voie_portique + 1;
            if(msg_bateau.destinataire == CONTENEUR_POUR_CAMION) {
                msg_portique.camion_destinataire = msg_bateau.camion_destinataire;
            }else {
                msg_portique.train_wagon = msg_bateau.train_wagon;
                msg_portique.wagon_emplacement = msg_bateau.wagon_emplacement;
            }
            // On envoie le conteneur à un portique
            msgsnd(msgid_portiques, &msg_portique, sizeof(message_portique) - sizeof(long), 0);
        }else {
            // Réception d'un conteneur
            pthread_mutex_lock(&stock_bateaux->mutex);
            printf("Le bateau a reçu un conteneur pour la voie du portique %d à l'emplacement %d\n", msg_bateau.voie_portique, msg_bateau.envoie_conteneur);
            // On compte le nombre de conteneurs à destination du bateau que le bateau possède 
            conteneurs_bateau++;
            // On place le conteneur à un emplacement précisé par le portique
            stock_bateaux->espace_portique[msg_bateau.voie_portique][msg_bateau.emplacement_conteneur] = CONTENEUR_POUR_BATEAU;
            /* 
            On regarde si le bateau est plein de conteneurs à destination du bateau
            Si le bateau est plein, il quitte le terminal de transport
            */
            if(conteneurs_bateau == bateau_rempli) {
                msg_fin_ordre.plein_conteneurs = TRUE;
                printf("Le bateau quitte le terminal de transport\n");
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
