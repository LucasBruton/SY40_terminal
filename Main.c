#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "Terminal.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/msg.h>

int shmid_camions, shmid_bateaux, shmid_trains, shmid_synchronisation, 
msgid_camions, msgid_trains, msgid_bateaux, msgid_superviseur, check_transport[2], check_camion[2];
stockage_bateau *stock_bateau;
stockage_train *stock_train;
stockage_camion *stock_camion;


// Fonction d'arrêt qui est appelé lorsque le programme reçoit un signal SIGINT
void arret(int signo) {
    // Libérations des objets IPC
    if (shmctl(shmid_camions, IPC_RMID, 0) == -1) {
        printf("Erreur fermeture du segment de mémoire pour les camions\n");
    }
    if (shmctl(shmid_bateaux, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture du segment de mémoire pour les bateaux\n");
    }
    if (shmctl(shmid_trains, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture du segment de mémoire pour les bateaux\n");
    }
    if (shmctl(shmid_synchronisation, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture du segment de mémoire pour la synchronisation du superviseur avec les autres véhicules\n");
    }
    if (msgctl(msgid_bateaux, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages des bateaux\n");
    }
    if (msgctl(msgid_camions, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages des camions\n");
    }
    if (msgctl(msgid_trains, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages des trains\n");
    }
    if (msgctl(msgid_superviseur, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages du superviseur\n");
    }
    printf("Fin de l'exécution du terminal...\n");
    exit(0);
}


int deplacerConteneurCamion(int portique) {
    message_camion envoi;
    envoi.envoie_conteneur = TRUE;

    int deplacement_conteneur = 0, conteneur, destination;
    pthread_mutex_lock(&stock_camion->mutex);
    for(int i = 0; i<stock_camion->nb_camion_par_portique && deplacement_conteneur == 0; ++i)
    {
        conteneur = stock_camion->espace_portique[portique][i];
        if (conteneur == CONTENEUR_POUR_BATEAU) {
            pthread_mutex_lock(&stock_bateau->mutex);
            for (int j = 0; j < stock_bateau->nb_conteneurs_par_partie_du_bateau && deplacement_conteneur == 0; ++j)
            {
                printf("Envoie message à camion (bateau)\n");
                destination = stock_bateau->espace_portique[portique][i];
                if (destination == ESPACE_CONTENEUR_VIDE)
                {
                    envoi.type = i*10 + j;
                    envoi.destinataire = CONTENEUR_POUR_BATEAU;
                    msgsnd(msgid_camions, &envoi, sizeof(message_camion) - sizeof(long), 0);
                    deplacement_conteneur = 1;
                }
            }
            pthread_mutex_unlock(&stock_bateau->mutex);
        }
        else if (conteneur == CONTENEUR_POUR_TRAIN)
        {
            pthread_mutex_lock(&stock_train->mutex);
            for (int j = 0; j < stock_bateau->nb_conteneurs_par_partie_du_bateau && deplacement_conteneur == 0; ++j)
            {
                printf("Envoie message à camion (train)\n");
                destination = stock_train->espace_portique[portique][i][j];
                if (destination == ESPACE_CONTENEUR_VIDE)
                {
                    envoi.type = i * 10 + j;
                    envoi.destinataire = CONTENEUR_POUR_TRAIN;
                    msgsnd(msgid_camions, &envoi, sizeof(message_camion) - sizeof(long), 0);
                    deplacement_conteneur = 1;
                }
            }
            pthread_mutex_unlock(&stock_train->mutex);
        }
    }
    pthread_mutex_unlock(&stock_camion->mutex);

    if(deplacement_conteneur==0) {
        return -1;
    }
    return 0;
}

// Correspond à un ordre du spuerviseur
void envoieOrdreVehicule(void * arg) {
    int portique = (int) arg;
    printf("Superviseur: %d\n", portique);
    deplacerConteneurCamion(portique);
}

void printConteneursVehicules() {
    int i, j, k;
    pthread_mutex_lock(&stock_camion->mutex);
    pthread_mutex_lock(&stock_bateau->mutex);
    pthread_mutex_lock(&stock_train->mutex);
    printf("Légende: \nEspace de conteneur vide = %d\nConteneur destiné aux bateaux = %d\nConteneur destiné aux trains = %d\nConteneur destiné aux camions = %d\n", ESPACE_CONTENEUR_VIDE, CONTENEUR_POUR_BATEAU, CONTENEUR_POUR_TRAIN, CONTENEUR_POUR_CAMION);
    printf("\nBateau:\n\n");
    for (i = 0; i < 2; ++i)
    {
        printf("Espace du portique %d\n", i);
        for (j = 0; j < stock_bateau->nb_conteneurs_par_partie_du_bateau; ++j)
        {
            printf("%d ", stock_bateau->espace_portique[i][j]);
        }
        printf("\n");
    }
    printf("\nTrain:\n\n");
    for (i = 0; i < 2; ++i)
    {
        printf("Espace du portique %d\n", i);
        for (j = 0; j < stock_train->nb_wagon_par_partie_du_train; ++j)
        {
            printf("Wagon %d\n", j);
            for (k = 0; k < stock_train->nb_conteneurs_par_wagon; ++k)
            {
                printf("%d ", stock_train->espace_portique[i][j][k]);
            }
            printf("\n");
        }
    }
    printf("\nCamion:\n\n");
    for (i = 0; i < 2; ++i)
    {
        printf("Espace du portique %d\n", i);
        for (j = 0; j < stock_camion->nb_camion_par_portique; ++j)
        {
            printf("%d ", stock_camion->espace_portique[i][j]);
        }
        printf("\n");
    }

    pthread_mutex_unlock(&stock_camion->mutex);
    pthread_mutex_unlock(&stock_bateau->mutex);
    pthread_mutex_unlock(&stock_train->mutex);
}

int main(int argc, char const *argv[]) {
    pthread_t num_thread;
    int nb_camion_par_portique, nb_conteneurs_par_partie_du_bateau, nb_wagon_par_partie_du_train, nb_conteneurs_par_wagon;
    key_t cle;
    args_camions a_camion[2][MAX_CAMION_PORTIQUE];
    debut_superviseur * d_superviseur;
    pthread_condattr_t cattr;
    pthread_mutexattr_t mattr;
    pthread_t id_bateau, id_portique[2], id_camion[5], id_train, id_ordre_superviseur[2];

    pthread_condattr_init(&cattr);
    pthread_mutexattr_init(&mattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

    // Vérification du nombre d'arguments passés
    if (argc < 5)
    {
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
    pthread_mutex_init(&stock_camion->mutex, &mattr);
    for(int i = 0; i<nb_camion_par_portique; ++i) {
        stock_camion->espace_portique[0][i] = 0;
        stock_camion->espace_portique[1][i] = 0;
    }

    // Creation de la file de messages pour les camions
    if ((cle = ftok(FICHIER_CAMION, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
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
    pthread_mutex_init(&stock_bateau->mutex, &mattr);
    for (int i = 0; i < nb_conteneurs_par_partie_du_bateau; ++i)
    {
        stock_bateau->espace_portique[0][i] = 0;
        stock_bateau->espace_portique[1][i] = 0;
    }

    // Creation de la file de messages pour les bateaux
    if ((cle = ftok(FICHIER_BATEAU, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_bateaux = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
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
    pthread_mutex_init(&stock_train->mutex, &mattr);
    for (int i = 0; i < nb_wagon_par_partie_du_train; ++i)
    {
        for (int j = 0; j < nb_conteneurs_par_wagon; ++j) {
            stock_train->espace_portique[0][i][j] = 0;
            stock_train->espace_portique[1][i][j] = 0;
        }
    }

    // Creation de la file de messages pour les trains
    if ((cle = ftok(FICHIER_TRAIN, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_trains = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Creation du segment de mémoire pour la synchronisation du superviseur avec les autres véhicules
    if ((cle = ftok(FICHIER, 1)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((shmid_synchronisation = shmget(cle, sizeof(debut_superviseur), IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Initialisation du segment de mémoire pour la synchronisation du superviseur avec les autres véhicules
    d_superviseur = (debut_superviseur *)shmat(shmid_synchronisation, NULL, 0);
    d_superviseur->nb_bateaux = 0;
    d_superviseur->nb_trains = 0;
    d_superviseur->nb_camions  = 0;
    pthread_cond_init(&d_superviseur->attente_vehicules, &cattr);
    pthread_mutex_init(&d_superviseur->mutex, &mattr);

    // Creation de la file de messages pour le superviseur
    if ((cle = ftok(FICHIER, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_superviseur = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Création des threads des véhicules
    pthread_create(&id_bateau, NULL, bateau, NULL);
    pthread_create(&id_portique[0], NULL, portique, NULL);
    pthread_create(&id_portique[1], NULL, portique, NULL);
    pthread_create(&id_train, NULL, train, NULL);

    // Création des threads camions
    for(int i = 0;  i < 2; ++i) {
        for (int j = 0; j < nb_camion_par_portique; ++j) {
            a_camion[i][j].numVoiePortique = i;
            a_camion[i][j].numEspacePortique = j;
            pthread_create(&id_camion[i], NULL, camion, (void*)&a_camion[i][j]);
        }
    }

    // Attente de l'initialisation des véhicules
    pthread_mutex_lock(&d_superviseur->mutex);
    while (d_superviseur->nb_trains < 1 || d_superviseur->nb_bateaux < 1 || d_superviseur->nb_camions < 2 * nb_camion_par_portique)
    {
        pthread_cond_wait(&d_superviseur->attente_vehicules, &d_superviseur->mutex);
    }
    pthread_mutex_unlock(&d_superviseur->mutex);

    // Initialisation des variables globales check_transport et check_camion
    check_transport[0] = 0;
    check_transport[1] = 0;
    check_camion[0] = 0;
    check_camion[1] = 0;

    // Lancement du superviseur qui peut envoyer jusqu'à deux ordre à la fois: un par portique
    while (1)
    {
        pthread_create(&id_ordre_superviseur[0], NULL, envoieOrdreVehicule, (void *)0);
        pthread_create(&id_ordre_superviseur[1], NULL, envoieOrdreVehicule, (void *)1);
        pthread_join(id_ordre_superviseur[0], NULL);
        pthread_join(id_ordre_superviseur[1], NULL);
        printConteneursVehicules();
        pause();
    }
    return EXIT_SUCCESS;
}
