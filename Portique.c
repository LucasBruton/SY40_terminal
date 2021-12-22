#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "Terminal.h"
#include <signal.h>
#include <sys/shm.h>

void * portique(void *arg) {
    key_t cle;
    int num_portique = (int)arg, msgid_trains, msgid_bateaux, msgid_camions[2],
        msgid_portiques, msgid_portiques_creation;
    message_portique msg_portique;
    message_bateau msg_bateau;
    message_camion msg_camion;
    message_train msg_train;
    message_retour msg_creation_retour;
    // Creation de la file de messages pour les trains
    if ((cle = ftok(FICHIER_TRAIN, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_trains = msgget(cle, 0)) == -1)
    {
        printf("Erreur récuparation de la file de messages pour les trains\n");
        kill(getpid(), SIGINT);
    }

    if ((cle = ftok(FICHIER_BATEAU, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_bateaux = msgget(cle, 0)) == -1)
    {
        printf("Erreur récuparation de la file de messages pour les bateaux\n");
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

    if ((cle = ftok(FICHIER_PORTIQUE, 1)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_portiques = msgget(cle, 0)) == -1)
    {
        printf("Erreur récuparation de la file de messages pour les portiques\n");
        kill(getpid(), SIGINT);
    }

    // Récupération de la file de messages pour la création des camions
    if ((cle = ftok(FICHIER_PORTIQUE, 2)) == -1)
    {
        printf("Erreur ftok\n");
        kill(getpid(), SIGINT);
    }
    if ((msgid_portiques_creation = msgget(cle, 0)) == -1)
    {
        printf("Erreur récupération de la file de messages de la création des portiques\n");
        kill(getpid(), SIGINT);
    }

    msg_bateau.voie_portique = num_portique;
    msg_bateau.envoie_conteneur = FALSE;
    msg_bateau.type = 1;
    msg_camion.envoie_conteneur = FALSE;
    msg_camion.attente = FALSE;
    msg_train.envoie_conteneur = FALSE;
    msg_train.type = 1;
    msg_train.voie_portique = num_portique;
    msg_creation_retour.type = 2;

    msgsnd(msgid_portiques_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 0);

    while(1) {
        msgrcv(msgid_portiques, &msg_portique, sizeof(message_portique) - sizeof(long), num_portique + 1, 0);
        printf("Portique %d: destination: %d\n", num_portique, msg_portique.destinataire);
        if(msg_portique.destinataire == CONTENEUR_POUR_BATEAU) {
            msg_bateau.emplacement_conteneur = msg_portique.bateau_emplacement;
            printf("Portique %d a récupéré un portique à destination du bateau\n", num_portique);
            msgsnd(msgid_bateaux, &msg_bateau, sizeof(message_bateau)-sizeof(long), 0);
        }else if(msg_portique.destinataire == CONTENEUR_POUR_TRAIN) {
            printf("Portique %d a récupéré un portique à destination du train\n", num_portique);
            msg_train.wagon_emplacement = msg_portique.wagon_emplacement;
            msg_train.wagon = msg_portique.train_wagon;
            msgsnd(msgid_trains, &msg_train, sizeof(message_train)-sizeof(long), 0);
        }else if (msg_portique.destinataire == CONTENEUR_POUR_CAMION) {
            printf("Portique %d a récupéré un portique à destination d'un camion\n", num_portique);
            msg_camion.type = msg_portique.camion_destinataire;
            msgsnd(msgid_camions[num_portique], &msg_camion, sizeof(message_camion) - sizeof(long), 0);
        }else {
            printf("Erreur portique %d: destination du conteneur\n", num_portique);
            kill(getpid(), SIGINT);
        }
    }
}
