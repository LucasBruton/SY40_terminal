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

int shmid_camions, shmid_bateaux, shmid_trains,
    msgid_camions[2], msgid_trains, msgid_bateaux, msgid_superviseur, msgid_portiques,
    check_transport[2], check_camion[2], nb_camion_par_portique, msgid_camions_creation,
    msgid_bateaux_creation, nb_camion_attente, msgid_trains_creations, msgid_portiques_creations,
    rotation_camions[2][MAX_CAMION_PORTIQUE], msgid_camions_attente, prochain_camion_attente,
    msgid_camions_com, retour_superviseur[2];
stockage_bateau *stock_bateau;
stockage_train *stock_train;
stockage_camion *stock_camion;
pthread_mutex_t mutex_prochain_camion;

// Fonction d'arrêt qui est appelé lorsque le programme reçoit un signal SIGINT
void arret(int signo)
{
    // Libérations des objets IPC
    if (shmctl(shmid_camions, IPC_RMID, 0) == -1)
    {
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
    if (msgctl(msgid_bateaux, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages des bateaux\n");
    }
    if (msgctl(msgid_camions[0], IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture 1e file de messages des camions\n");
    }
    if (msgctl(msgid_camions[1], IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture 2e file de messages des camions\n");
    }
    if (msgctl(msgid_camions_attente, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages pour l'attente des camions\n");
    }
    if (msgctl(msgid_camions_com, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages pour la communication entre les camions\n");
    }
    if (msgctl(msgid_trains, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages des trains\n");
    }
    if (msgctl(msgid_superviseur, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages du superviseur\n");
    }
    if (msgctl(msgid_portiques, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages des portiques\n");
    }
    if (msgctl(msgid_bateaux_creation, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages de la création des bateaux\n");
    }
    if (msgctl(msgid_camions_creation, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages de la création des camions\n");
    }
    if (msgctl(msgid_trains_creations, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages de la création des trains\n");
    }
    if (msgctl(msgid_portiques_creations, IPC_RMID, 0) == -1)
    {
        printf("Erreur fermeture file de messages de la création des portiques\n");
    }
    printf("Fin de l'exécution du terminal...\n");
    exit(0);
}


/*
Fonction permettant de déplacer un conteneur du bateau auprès d'un autre transport
int portique: numéro du portique qui va déplacer le conteneur du bateau
retour int: TRUE si la fonction a déplacé un conteneur, sinon FALSE
*/
int deplacerConteneurBateau(int portique)
{
    message_bateau envoi;
    int conteneur, destination;
    envoi.envoie_conteneur = TRUE;
    envoi.voie_portique = portique;
    envoi.type = 1;

    // Parcours les emplacement de conteneurs accessibles sur la voie précisé par la variable portique
    pthread_mutex_lock(&stock_bateau->mutex);
    for (int i = 0; i < stock_bateau->nb_conteneurs_par_partie_du_bateau; ++i)
    {
        conteneur = stock_bateau->espace_portique[portique][i];
        // Si le conteneur est destiné à un camion, on tente de l'envoyer
        if (conteneur == CONTENEUR_POUR_CAMION)
        {
            // On regarde si un camion est vide
            // On parcours les camions accessibles sur la voie précisé par la variable portique
            pthread_mutex_lock(&stock_camion->mutex);
            for (int j = 0; j < stock_camion->nb_camion_par_portique; ++j)
            {
                destination = stock_camion->espace_portique[portique][j];
                // Si un camion est vide, on indique au bateau d'envoyer le conteneur au camion vide
                if (destination == ESPACE_CONTENEUR_VIDE)
                {
                    envoi.destinataire = CONTENEUR_POUR_CAMION;
                    envoi.camion_destinataire = j+1;
                    envoi.emplacement_conteneur = i;
                    // Message indiquant au bateau d'envoyer le conteneur au camion vide
                    msgsnd(msgid_bateaux, &envoi, sizeof(message_bateau) - sizeof(long), 0);
                    pthread_mutex_unlock(&stock_bateau->mutex);
                    pthread_mutex_unlock(&stock_camion->mutex);
                    return TRUE;
                }
            }
            pthread_mutex_unlock(&stock_camion->mutex);
        } // Si le conteneur est destiné à un train, on tente de lui envoyer
        else if (conteneur == CONTENEUR_POUR_TRAIN)
        {
            // On vérifie que le train est un emplacement de conteneur vide
            // Parcours des wagons accessibles sur la voie précisé par la variable portique
            pthread_mutex_lock(&stock_train->mutex);
            for (int j = 0; j < stock_train->nb_wagon_par_partie_du_train; ++j)
            {
                // Parcours des emplacements de conteneur du wagon
                for (int k = 0; k < stock_train->nb_conteneurs_par_wagon; ++k)
                {
                    destination = stock_train->espace_portique[portique][j][k];
                    // Si l'emplacement du wagon est vide, on lui envoit le conteneur
                    if (destination == ESPACE_CONTENEUR_VIDE)
                    {
                        envoi.destinataire = CONTENEUR_POUR_TRAIN;
                        envoi.emplacement_conteneur = i;
                        envoi.train_wagon = j;
                        envoi.wagon_emplacement = k;
                        // message indiquant au bateau d'envoyer le conteneur à l'emplacement vide du train
                        msgsnd(msgid_bateaux, &envoi, sizeof(message_bateau) - sizeof(long), 0);
                        pthread_mutex_unlock(&stock_train->mutex);
                        pthread_mutex_unlock(&stock_bateau->mutex);
                        return TRUE;
                    }
                }
            }
            pthread_mutex_unlock(&stock_train->mutex);
        }
    }
    pthread_mutex_unlock(&stock_bateau->mutex);

    return FALSE;
}

/*
Fonction permettant de déplacer un conteneur du train auprès d'un autre transport
int portique: numéro du portique qui va déplacer le conteneur du train
retour int: TRUE si la fonction a déplacé un conteneur, sinon FALSE
*/
int deplacerConteneurTrain(int portique)
{
    message_train envoi;
    int conteneur, destination;
    envoi.envoie_conteneur = TRUE;
    envoi.type = 1;
    envoi.voie_portique = portique;
    // Parcours les wagons accessibles sur la voie précisé par la variable portique
    pthread_mutex_lock(&stock_train->mutex);
    for (int i = 0; i < stock_train->nb_wagon_par_partie_du_train; ++i)
    {
        // Parcours les emplacements de conteneurs du wagon
        for (int j = 0; j < stock_train->nb_conteneurs_par_wagon; ++j) {
            conteneur = stock_train->espace_portique[portique][i][j];
            // Si le conteneur est destiné au bateau, on tente de lui envoyer 
            if (conteneur == CONTENEUR_POUR_BATEAU)
            {
                // On vérifie que le bateau est un emplacement vide
                pthread_mutex_lock(&stock_bateau->mutex);
                for (int k = 0; k < stock_bateau->nb_conteneurs_par_partie_du_bateau; ++k)
                {
                    destination = stock_bateau->espace_portique[portique][k];
                    // Si le bateau à un emplacement libre, on indique au train d'envoyer le conteneur à l'emplacement libre du bateau
                    if (destination == ESPACE_CONTENEUR_VIDE)
                    {
                        envoi.wagon = i;
                        envoi.wagon_emplacement = j;
                        envoi.bateau_emplacement = k;
                        envoi.destinataire = CONTENEUR_POUR_BATEAU;
                        // Message indiquant au train d'envoyer le conteneur à l'emplacement libre du bateau
                        msgsnd(msgid_trains, &envoi, sizeof(message_train) - sizeof(long), 0);
                        pthread_mutex_unlock(&stock_bateau->mutex);
                        pthread_mutex_unlock(&stock_train->mutex);
                        return TRUE;
                    }
                }
                pthread_mutex_unlock(&stock_bateau->mutex);
            } // Si le conteneur est destiné à un camion, on tente de l'envoyer'
            else if (conteneur == CONTENEUR_POUR_CAMION)
            {
                // On essaye de trouver un camion vide
                // parcours des camions accessibles sur la voie précisé par la variable portique
                pthread_mutex_lock(&stock_camion->mutex);
                for (int k = 0; k < stock_camion->nb_camion_par_portique; ++k)
                {
                    destination = stock_camion->espace_portique[portique][k];
                    // Si le camion est vide, on indique au train d'envoyer le conteneur au camion
                    if (destination == ESPACE_CONTENEUR_VIDE)
                    {
                        envoi.wagon = i;
                        envoi.wagon_emplacement = j;
                        envoi.destinataire = CONTENEUR_POUR_CAMION;
                        envoi.camion_destinataire = k+1;
                        // Message indiquant au train d'envoyer le conteneur au camion
                        msgsnd(msgid_trains, &envoi, sizeof(message_train) - sizeof(long), 0);
                        pthread_mutex_unlock(&stock_train->mutex);
                        pthread_mutex_unlock(&stock_camion->mutex);
                        return TRUE;
                    }
                }
                pthread_mutex_unlock(&stock_camion->mutex);
            }
        }
    }
    pthread_mutex_unlock(&stock_train->mutex);

    return FALSE;
}

/*
Fonction permettant de déplacer un conteneur d'un camion auprès d'un autre transport
int portique: numéro du portique qui va déplacer le conteneur du camion
retour int: TRUE si la fonction a déplacé un conteneur, sinon FALSE
*/
int deplacerConteneurCamion(int portique)
{
    message_camion envoi;
    message_attente_camion msg_attente;
    envoi.envoie_conteneur = TRUE;
    envoi.voie_portique = portique;
    envoi.attente = FALSE;
    msg_attente.endormir_camion = TRUE;
    int conteneur, destination;
    // Parcours les camions accessibles sur la voie précisé par la variable portique
    pthread_mutex_lock(&stock_camion->mutex);
    for (int i = 0; i < stock_camion->nb_camion_par_portique; ++i)
    {
        conteneur = stock_camion->espace_portique[portique][check_camion[portique]];
        // Si le conteneur est destiné au bateau, on tente de lui envoyer
        if (conteneur == CONTENEUR_POUR_BATEAU)
        {
            // Parcours les emplacement de conteneurs accessibles sur la voie précisé par la variable portique
            pthread_mutex_lock(&stock_bateau->mutex);
            for (int j = 0; j < stock_bateau->nb_conteneurs_par_partie_du_bateau; ++j)
            {
                destination = stock_bateau->espace_portique[portique][j];
                // Si le bateau à un emplacement libre, on indique au camion d'envoyer le conteneur à l'emplacement libre du bateau
                if (destination == ESPACE_CONTENEUR_VIDE)
                {
                    envoi.type = check_camion[portique] + 1;
                    envoi.desinataire = CONTENEUR_POUR_BATEAU;
                    envoi.bateau_emplacement = j;
                    msgsnd(msgid_camions[portique], &envoi, sizeof(message_camion) - sizeof(long), 0);
                    check_camion[portique] = (check_camion[portique] + 1) % stock_camion->nb_camion_par_portique;
                    pthread_mutex_unlock(&stock_bateau->mutex);
                    pthread_mutex_unlock(&stock_camion->mutex);
                    return TRUE;
                }
            }
            pthread_mutex_unlock(&stock_bateau->mutex);
        } // Si le conteneur est destiné au train, on tente de lui envoyer
        else if (conteneur == CONTENEUR_POUR_TRAIN)
        {
            // On vérifie que le train est un emplacement de conteneur vide
            // Parcours des wagons accessibles sur la voie précisé par la variable portique            pthread_mutex_lock(&stock_train->mutex);
            for (int j = 0; j < stock_train->nb_wagon_par_partie_du_train; ++j)
            {
                // Parcours des emplacements de conteneur du wagon
                for (int k = 0; k < stock_train->nb_conteneurs_par_wagon; ++k)
                {
                    // Si l'emplacement du wagon est vide, on lui envoit le conteneur
                    destination = stock_train->espace_portique[portique][j][k];
                    if (destination == ESPACE_CONTENEUR_VIDE)
                    {
                        envoi.type = check_camion[portique] + 1;
                        envoi.train_wagon = j;
                        envoi.wagon_emplacement = k;
                        envoi.desinataire = CONTENEUR_POUR_TRAIN;
                        // message indiquant au camion d'envoyer le conteneur à l'emplacement vide du train
                        msgsnd(msgid_camions[portique], &envoi, sizeof(message_camion) - sizeof(long), 0);
                        check_camion[portique] = (check_camion[portique] + 1) % stock_camion->nb_camion_par_portique;
                        pthread_mutex_unlock(&stock_train->mutex);
                        pthread_mutex_unlock(&stock_camion->mutex);
                        return TRUE;
                    }
                }
            }
            pthread_mutex_unlock(&stock_train->mutex);
        }
        /* 
        Si le camion n'a pu déplacer son conteneur ou qu'il est vide,
        on incrémente le rotations_camions du camion.
        Si rotaions_camions du camionest égale au nombre de camions qui attendent, 
        on dit à un camion de la file d'attente de remplacer le camion qui
        vient d'être examiné.
        */
        rotation_camions[portique][check_camion[portique]]++;
        if (rotation_camions[portique][check_camion[portique]] == nb_camion_attente)
        {
            pthread_mutex_lock(&mutex_prochain_camion);
            printf("\nEchange du camion sur la voie %d à l'emplacement %d avec le camion attendant au numéro %d de la file d'attente\n", portique, check_camion[portique], prochain_camion_attente);
            rotation_camions[portique][check_camion[portique]] = 0;
            msg_attente.type = prochain_camion_attente;
            msg_attente.voie_portique = portique;
            msg_attente.emplacement_portique = check_camion[portique];
            // On envoie un message au camion attendant au numéro prochain_camion_attente
            msgsnd(msgid_camions_attente, &msg_attente, sizeof(message_attente_camion) - sizeof(long), NULL);
            prochain_camion_attente = prochain_camion_attente%nb_camion_attente + 1;
            pthread_mutex_unlock(&mutex_prochain_camion);
            pthread_mutex_unlock(&stock_camion->mutex);
            return TRUE;
        }
        check_camion[portique] = (check_camion[portique] + 1) % stock_camion->nb_camion_par_portique;
    }
    pthread_mutex_unlock(&stock_camion->mutex);

    return FALSE;
}

/*
Cette fonction va essayé de déplacé un conteneur d'un des véhicules en utilisant les 
fonctions précédentes.
void *arg: numéro du portique que la fonction va utiliser
*/ 
void envoieOrdreVehicule(void *arg)
{
    int portique = (int)arg;
    retour_superviseur[portique] = FALSE;
    for (int i = 0; (i < 3) && (retour_superviseur[portique] == FALSE); ++i)
    {
        if (check_transport[portique] == 0) {
            retour_superviseur[portique] = deplacerConteneurBateau(portique);
        }else if (check_transport[portique] == 1) {
            retour_superviseur[portique] = deplacerConteneurTrain(portique);
        }else {
            retour_superviseur[portique] = deplacerConteneurCamion(portique);
        }
        check_transport[portique] = (check_transport[portique] + 1)%3;
    }
}

// Cette fonction print les conteneurs des véhicules qui sont traités par les portiques
void printConteneursVehicules()
{
    int i, j, k;
    pthread_mutex_lock(&stock_camion->mutex);
    pthread_mutex_lock(&stock_bateau->mutex);
    pthread_mutex_lock(&stock_train->mutex);
    printf("\nLégende: \nEspace de conteneur vide = %d\nConteneur destiné aux bateaux = %d\nConteneur destiné aux trains = %d\nConteneur destiné aux camions = %d\n", ESPACE_CONTENEUR_VIDE, CONTENEUR_POUR_BATEAU, CONTENEUR_POUR_TRAIN, CONTENEUR_POUR_CAMION);
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

int main(int argc, char const *argv[])
{
    pthread_t num_thread;
    int nb_conteneurs_par_partie_du_bateau, nb_wagon_par_partie_du_train, 
    nb_conteneurs_par_wagon, ordre_reussi, *ordre_fini;
    key_t cle;
    pthread_mutexattr_t mattr;
    pthread_t id_bateau, id_portique[2], id_camion, id_train, id_ordre_superviseur[2];
    message_fin_ordre_superviseur msg_fin_ordre;
    message_creation_camion msg_creation_camion;
    message_retour msg_creation_retour;
    message_attente_camion msg_attente_camion;

    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

    // Vérification du nombre d'arguments passés
    if (argc < 6)
    {
        printf("Usage: %s <nbCamionParPortique> <nbCamionsAttente> <nbConteneursParPartieDuBateau> <nbWagonsParPartieDuTrain> <nbConteneursParWagon>\n", argv[0]);
        return EXIT_FAILURE;
    }
    signal(SIGINT, arret);
    // On assigne les arguments a des int
    nb_camion_par_portique = atoi(argv[1]);
    nb_camion_attente = atoi(argv[2]);
    nb_conteneurs_par_partie_du_bateau = atoi(argv[3]);
    nb_wagon_par_partie_du_train = atoi(argv[4]);
    nb_conteneurs_par_wagon = atoi(argv[5]);

    // Vérification des valeurs données en arguments
    if (nb_camion_par_portique <= 0 || nb_camion_par_portique > MAX_CAMION_PORTIQUE)
    {
        printf("Erreur: l'argument <nbCamionParPortique> doit être compris entre 1 et %d\n", MAX_CAMION_PORTIQUE);
        return EXIT_FAILURE;
    }
    if (nb_camion_attente <= 1 || nb_camion_attente > MAX_CAMION_ATTENTE)
    {
        printf("Erreur: l'argument <nbCamionsAttente> doit être compris entre 2 et %d\n", MAX_CAMION_ATTENTE);
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
    if ((shmid_camions = shmget(cle, sizeof(stockage_camion), IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création segment de mémoire pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Initialisation du segment de mémoire pour les camions
    stock_camion = (stockage_camion *)shmat(shmid_camions, NULL, 0);
    stock_camion->nb_camion_par_portique = nb_camion_par_portique;
    pthread_mutex_init(&stock_camion->mutex, &mattr);
    for (int i = 0; i < nb_camion_par_portique; ++i)
    {
        stock_camion->espace_portique[0][i] = 0;
        stock_camion->espace_portique[1][i] = 0;
    }

    // Creation de la 1e file de messages pour les camions
    if ((cle = ftok(FICHIER_CAMION, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions[0] = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Creation de la 2e file de messages pour les camions
    if ((cle = ftok(FICHIER_CAMION, 3)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions[1] = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages pour les camions\n");
        kill(getpid(), SIGINT);
    }

    // Creation de la file de messages pour la création des camions
    if ((cle = ftok(FICHIER_CAMION, 4)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions_creation = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages de la création des camions\n");
        kill(getpid(), SIGINT);
    }

    // Creation de la file de messages pour l'attente des camions
    if ((cle = ftok(FICHIER_CAMION, 5)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions_attente = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages de l'attente des camions\n");
        kill(getpid(), SIGINT);
    }

    // Creation de la file de messages pour la communication entre les camions
    if ((cle = ftok(FICHIER_CAMION, 6)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_camions_com = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages de communications entre les camions\n");
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
    stock_bateau->nb_conteneurs_par_partie_du_bateau = nb_conteneurs_par_partie_du_bateau;
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

    // Creation de la file de messages pour la création des bateaux
    if ((cle = ftok(FICHIER_BATEAU, 3)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_bateaux_creation = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages de la création des bateaux\n");
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
        for (int j = 0; j < nb_conteneurs_par_wagon; ++j)
        {
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
        printf("Erreur création de la file de messages pour les trains\n");
        kill(getpid(), SIGINT);
    }

    // Creation de la file de messages pour la création des camions
    if ((cle = ftok(FICHIER_TRAIN, 3)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_trains_creations = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages de la création des trains\n");
        kill(getpid(), SIGINT);
    }

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

    // Creation de la file de messages pour les portiques
    if ((cle = ftok(FICHIER_PORTIQUE, 1)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_portiques = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages pour les portiques\n");
        kill(getpid(), SIGINT);
    }

    // Creation de la file de messages pour la création des portiques
    if ((cle = ftok(FICHIER_PORTIQUE, 2)) == -1)
    {
        printf("Erreur ftok\n");
        return EXIT_FAILURE;
    }
    if ((msgid_portiques_creations = msgget(cle, IPC_CREAT | 0666)) == -1)
    {
        printf("Erreur création de la file de messages de créations des portiques\n");
        kill(getpid(), SIGINT);
    }

    // Création des threads du bateau, du train et des portiques
    pthread_create(&id_bateau, NULL, bateau, NULL);
    // Réception d'un message de confirmation de l'initialisation du véhicule
    msgrcv(msgid_bateaux_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 2, 0);
    pthread_create(&id_portique[0], NULL, portique, (void *)0);
    pthread_create(&id_portique[1], NULL, portique, (void *)1);
    // Réception d'un message de confirmation de l'initialisation du véhicule
    msgrcv(msgid_portiques_creations, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 2, 0);
    msgrcv(msgid_portiques_creations, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 2, 0);
    pthread_create(&id_train, NULL, train, NULL);
    // Réception d'un message de confirmation de l'initialisation du véhicule
    msgrcv(msgid_trains_creations, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 2, 0);

    msg_creation_camion.type = 1;
    msg_creation_camion.attente = FALSE;

    // Création des threads des camions qui vont être traiter par les portiques
    for (int i = 0; i < 2; ++i)
    {
        msg_creation_camion.voie_portique = i;
        for (int j = 0; j < nb_camion_par_portique; ++j)
        {
            
            msg_creation_camion.emplacement_portique = j;
            pthread_create(&id_camion, NULL, camion, NULL);
            // Envoie de paramètres au camion créé
            msgsnd(msgid_camions_creation, &msg_creation_camion, sizeof(message_creation_camion) - sizeof(long), 0);
            // Réception d'un message de confirmation de l'initialisation du véhicule
            msgrcv(msgid_camions_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 2, 0);
            rotation_camions[i][j] = 0;
        }
    }

    // Création des camions qui attendant dans la file d'attente des camions
    msg_creation_camion.attente = TRUE;
    for(int i = 0; i<nb_camion_attente; ++i) {
        msg_creation_camion.num_attente = i+1;
        pthread_create(&id_camion, NULL, camion, NULL);
        // Envoie de paramètres au camion créé
        msgsnd(msgid_camions_creation, &msg_creation_camion, sizeof(message_creation_camion) - sizeof(long), 0);
        // Réception d'un message de confirmation de l'initialisation du véhicule
        msgrcv(msgid_camions_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 2, 0);
    }

    // Initialisation des variables globales check_transport et check_camion
    check_transport[0] = 0;
    check_transport[1] = 0;
    check_camion[0] = 0;
    check_camion[1] = 0;
    prochain_camion_attente = 1;
    msg_attente_camion.endormir_camion = FALSE;
    pthread_mutex_init(&mutex_prochain_camion, NULL);

    // On print les conteneurs des véhicules
    printConteneursVehicules();
    msg_creation_camion.attente = FALSE;

    // Lancement du superviseur qui peut envoyer un ordre par portique
    while (1)
    {
        // Envoie des ordres
        pthread_create(&id_ordre_superviseur[0], NULL, envoieOrdreVehicule, (void *)0);
        pthread_create(&id_ordre_superviseur[1], NULL, envoieOrdreVehicule, (void *)1);
        // Attente de la fin des ordres
        pthread_join(id_ordre_superviseur[0], NULL);
        pthread_join(id_ordre_superviseur[1], NULL);
        // On compte le nombre d'ordres qui ont réussi
        ordre_reussi = 0;
        ordre_reussi += retour_superviseur[0];
        ordre_reussi += retour_superviseur[1];
        // On attend des messages de confirmations des véhicules des ordres envoyés (en fonction de la variable ordre_reussi)
        for(int i = 0; i<ordre_reussi; ++i) {
            // Attente d'un message de confirmation d'un véhicule
            msgrcv(msgid_superviseur, &msg_fin_ordre, sizeof(message_fin_ordre_superviseur) - sizeof(long), 1, 0);
            // Si le véhicule est plein, il quitte le terminal et le superviseur en crée un nouveau
            if(msg_fin_ordre.plein_conteneurs == TRUE) {
                /*
                Si le véhicule qui est plein est un camion, on dit à un camion qui attend de remplacer
                le camion qui est parti puis on crée un nouveau camion qui attend dans la file d'attente 
                au même emplacement du camion qui vient de quitter la file d'attente
                */
                if(msg_fin_ordre.type_transport == CONTENEUR_POUR_CAMION) {
                    printf("Création nouveau camion + Déplacement d'un camion de la file d'attente à la voie %d à l'emplacement %d\n", msg_fin_ordre.camion_voie_portique, msg_fin_ordre.camion_emplacement);
                    msg_attente_camion.type = prochain_camion_attente;
                    msg_attente_camion.voie_portique = msg_fin_ordre.camion_voie_portique;
                    msg_attente_camion.emplacement_portique = msg_fin_ordre.camion_emplacement;
                    msg_attente_camion.endormir_camion = FALSE;
                    msg_creation_camion.type = 1;
                    msg_creation_camion.attente = TRUE;
                    msg_creation_camion.num_attente = prochain_camion_attente;
                    // Envoie d'un message à un camion qui attend pour qu'il remplace le camion qui vient de quitter le terminal de transport
                    msgsnd(msgid_camions_attente, &msg_attente_camion, sizeof(message_attente_camion) - sizeof(long), 0);
                    // Attente d'un message de confirmation du message précédent
                    msgrcv(msgid_camions_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 3, 0);
                    // Création d'un nouveau cmaion
                    pthread_create(&id_camion, NULL, camion, NULL);
                    // Envoie de paramètres au camion créé
                    msgsnd(msgid_camions_creation, &msg_creation_camion, sizeof(message_creation_camion) - sizeof(long), 0);
                    // Réception d'un message de confirmation de l'initialisation du véhicule
                    msgrcv(msgid_camions_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 2, 0);
                    prochain_camion_attente = prochain_camion_attente % nb_camion_attente + 1;
                } // Si le train est plein, il quitte le terminal de transport et le superviseur crée un nouveau train
                else if (msg_fin_ordre.type_transport == CONTENEUR_POUR_TRAIN) {
                    printf("Création d'un nouveau train\n");
                    // Création d'un nouveau train
                    pthread_create(&id_train, NULL, train, NULL);
                    // Réception d'un message de confirmation de l'initialisation du véhicule
                    msgrcv(msgid_trains_creations, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 2, 0);
                } // Si le bateau est plein, il quitte le terminal de transport et le superviseur crée un nouveau bateau
                else if (msg_fin_ordre.type_transport == CONTENEUR_POUR_BATEAU) {
                    printf("Création d'un nouveau bateau\n");
                    // Création d'un nouveau bateau
                    pthread_create(&id_bateau, NULL, bateau, NULL);
                    // Réception d'un message de confirmation de l'initialisation du véhicule
                    msgrcv(msgid_bateaux_creation, &msg_creation_retour, sizeof(message_retour) - sizeof(long), 2, 0);
                }
            }
            
        }
        // On print les conteneurs des véhicules
        printConteneursVehicules();
        // Le superviseur dort 1 seconde avant d'envoyer des nouveaux ordres
        sleep(1);
    }
    return EXIT_SUCCESS;
}
