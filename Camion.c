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

// Fonction permettant de créer un camion
void *camion(void *arg) {
    key_t cle;
    int shmid_mutex, shmid_camions, random, msgid_camions[2],
        msgid_superviseur, msgid_portiques, conteneur, msgid_camions_creation,
        voie_portique, emplacement_portique, msgid_camions_attente, num_attente,
        msgid_camions_com;
    stockage_camion *stock_camion;
    message_camion msg_camion, msg_camion_envoi;
    message_portique msg_portique;
    message_fin_ordre_superviseur msg_fin_ordre;
    message_creation_camion msg_creation_camion;
    message_retour msg_creation_retour;
    message_attente_camion msg_attente_camion, msg_retour;
    msg_creation_retour.type = 2;
    msg_fin_ordre.type = 1;
    msg_fin_ordre.type_transport = CONTENEUR_POUR_CAMION;

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

    // Récupération de la file de messages pour l'attente des camions
    if ((cle = ftok(FICHIER_CAMION, 5)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions_attente = msgget(cle, 0)) == -1)
    {
        printf("Erreur création de la file de messages de l'attente des camions\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages pour la communication entre les camions
    if ((cle = ftok(FICHIER_CAMION, 6)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions_com = msgget(cle, 0)) == -1)
    {
        printf("Erreur récupération de la file de messages de communications entre les camions\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages du superviseur
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

    /* 
    Le camion attend un message du main pour savoir si le camion se met à un emplacement où un portique peut le traiter 
    ou si le camion attend dans la file d'attente des camions
    */
    msgrcv(msgid_camions_creation, &msg_creation_camion, sizeof(message_creation_camion) - sizeof(long), 1, 0);
    // Envoie d'un message pour confirmer la réception du message précédent
    msgsnd(msgid_camions_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 0);
    // Si le camion se place dans la file d'attente, il attend la réception d'un message pour poursuivre son exécution 
    if(msg_creation_camion.attente == TRUE) {
        num_attente = msg_creation_camion.num_attente;
        msgrcv(msgid_camions_attente, &msg_attente_camion, sizeof(message_attente_camion) - sizeof(long), num_attente, 0);
        voie_portique = msg_attente_camion.voie_portique;
        emplacement_portique = msg_attente_camion.emplacement_portique;
        // On vérifie si le camion indique à un autre camion qu'il doit se mettre dans la file d'attente
        if(msg_attente_camion.endormir_camion==TRUE) {
            msg_camion_envoi.type = emplacement_portique + 1;
            msg_camion_envoi.attente = TRUE;
            msg_camion_envoi.num_attente = num_attente;
            msg_camion_envoi.voie_portique = voie_portique;
            /* 
            Envoie d'un message au camion sur la voie du portique voie_portique à l'emplacement 
            emplacement_portique quil doit attendre dans la file d'attente avec le numéro num_attente
            */
            msgsnd(msgid_camions[voie_portique], &msg_camion_envoi, sizeof(message_camion) - sizeof(long), 0);
            // Réception du message envoyé précédemment
            msgrcv(msgid_camions_com, &msg_retour, sizeof(message_retour) - sizeof(long), voie_portique + 1, 0);
            // Remplacement du conteneur sur la voie du portique voie_portique à l'emplacement emplacement_portique
            pthread_mutex_lock(&stock_camion->mutex);
            stock_camion->espace_portique[voie_portique][emplacement_portique] = conteneur;
            pthread_mutex_unlock(&stock_camion->mutex);
            msg_fin_ordre.plein_conteneurs = FALSE;
            // Envoie d'un message pour confirmer la fin de l'ordre du superviseur pour la voie du portique voie_portique
            msgsnd(msgid_superviseur, &msg_fin_ordre, sizeof(message_fin_ordre_superviseur) - sizeof(long), 0);
        }else {
            // Remplacement du conteneur sur la voie du portique voie_portique à l'emplacement emplacement_portique
            pthread_mutex_lock(&stock_camion->mutex);
            stock_camion->espace_portique[voie_portique][emplacement_portique] = conteneur;
            pthread_mutex_unlock(&stock_camion->mutex);
            msg_retour.type = 3;
            // Confirmation du camion qu'il a finit de s'installer sur la voie d'un portique
            msgsnd(msgid_camions_creation, &msg_retour, sizeof(message_retour) - sizeof(long), NULL);
        }

    }else {
        
        voie_portique = msg_creation_camion.voie_portique;
        emplacement_portique = msg_creation_camion.emplacement_portique;
        // Le camion place son conteneur sur la voie voie_portique à l'emplacement emplacement_portique
        pthread_mutex_lock(&stock_camion->mutex);
        stock_camion->espace_portique[voie_portique][emplacement_portique] = conteneur;
        pthread_mutex_unlock(&stock_camion->mutex);
    }

    // Boucle infini de réception et d'envoi de messages
    while (1)
    {
        // Réception d'un message sur la voie voie_portique à l'emplacement emplacement_portique +1
        msgrcv(msgid_camions[voie_portique], &msg_camion, sizeof(message_camion) - sizeof(long), emplacement_portique + 1, 0);
        // Si le camion se met en attente, il se place dans la file d'attente à l'emplacement num_attente
        if(msg_camion.attente==TRUE) {
            num_attente = msg_camion.num_attente;
            msg_retour.type = voie_portique+1;
            // Envoi d'un message de réception
            msgsnd(msgid_camions_com, &msg_retour, sizeof(message_retour) - sizeof(long), 0);
            // Attente d'un message pour se placer sur une voie de portique
            msgrcv(msgid_camions_attente, &msg_attente_camion, sizeof(message_attente_camion) - sizeof(long), num_attente, 0);
            voie_portique = msg_attente_camion.voie_portique;
            emplacement_portique = msg_attente_camion.emplacement_portique;
            // On vérifie si le camion indique à un autre camion qu'il doit se mettre dans la file d'attente
            if (msg_attente_camion.endormir_camion == TRUE) {
                emplacement_portique = msg_attente_camion.emplacement_portique;
                msg_camion_envoi.type = emplacement_portique + 1;
                msg_camion_envoi.attente = TRUE;
                msg_camion_envoi.num_attente = num_attente;
                msg_camion_envoi.voie_portique = voie_portique;
                /* 
                Envoie d'un message au camion sur la voie du portique voie_portique à l'emplacement 
                emplacement_portique quil doit attendre dans la file d'attente avec le numéro num_attente
                */
                msgsnd(msgid_camions[voie_portique], &msg_camion_envoi, sizeof(message_camion) - sizeof(long), 0);
                // Réception du message envoyé précédemment
                msgrcv(msgid_camions_com, &msg_retour, sizeof(message_retour) - sizeof(long), voie_portique+1, 0);
                // Remplacement du conteneur sur la voie du portique voie_portique à l'emplacement emplacement_portique
                pthread_mutex_lock(&stock_camion->mutex);
                stock_camion->espace_portique[voie_portique][emplacement_portique] = conteneur;
                pthread_mutex_unlock(&stock_camion->mutex);
                msg_fin_ordre.plein_conteneurs = FALSE;
                // Envoie d'un message pour confirmer la fin de l'ordre du superviseur pour la voie du portique voie_portique
                msgsnd(msgid_superviseur, &msg_fin_ordre, sizeof(message_fin_ordre_superviseur) - sizeof(long), 0);
            }else {
                // Remplacement du conteneur sur la voie du portique voie_portique à l'emplacement emplacement_portique
                pthread_mutex_lock(&stock_camion->mutex);
                stock_camion->espace_portique[voie_portique][emplacement_portique] = conteneur;
                pthread_mutex_unlock(&stock_camion->mutex);
                msg_retour.type = 3;
                // Confirmation du camion qu'il a finit de s'installer sur la voie d'un portique
                msgsnd(msgid_camions_creation, &msg_retour, sizeof(message_retour) - sizeof(long), NULL);
            }
        }else {
            // Le camion regarde s'il donne son conteneur au portique ou s'il reçoit un conteneur
            if (msg_camion.envoie_conteneur == TRUE)
            {
                // On vide le camion
                pthread_mutex_lock(&stock_camion->mutex);
                stock_camion->espace_portique[voie_portique][emplacement_portique] = ESPACE_CONTENEUR_VIDE;
                conteneur = ESPACE_CONTENEUR_VIDE;
                pthread_mutex_unlock(&stock_camion->mutex);
                printf("Le camion de la voie du portique %d à l'emplacement %d donne son conteneur au portique\n", voie_portique, emplacement_portique);
                // Le camion précise au portique à qui il envoie son conteneur
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
                msg_portique.type = voie_portique + 1;
                // Le camion donne son conteneur au portique de sa voie
                msgsnd(msgid_portiques, &msg_portique, sizeof(message_portique) - sizeof(long), 0);
            }
            else
            {
                // Le camion prend le conteneur puis il quitte le terminal de transport
                pthread_mutex_lock(&stock_camion->mutex);
                printf("Le camion de la voie du portique %d à l'emplacement %d a reçu un conteneur\n", voie_portique, emplacement_portique);
                stock_camion->espace_portique[voie_portique][emplacement_portique] = CONTENEUR_POUR_CAMION;
                pthread_mutex_unlock(&stock_camion->mutex);
                printf("Le camion de la voie du portique %d à l'emplacement %d quitte le terminal de transport\n", voie_portique, emplacement_portique);
                msg_fin_ordre.camion_emplacement = emplacement_portique;
                msg_fin_ordre.camion_voie_portique = voie_portique;
                msg_fin_ordre.plein_conteneurs = TRUE;
                // Le camion quitte le terminal
                msgsnd(msgid_superviseur, &msg_fin_ordre, sizeof(message_fin_ordre_superviseur) - sizeof(long), 0);
                pthread_exit(NULL);
            }
        }
    }
}
