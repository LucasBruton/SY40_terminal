#ifndef TERMINALE_H
#define TERMINALE_H

#include <pthread.h>

#define MAX_CAMION_PORTIQUE 50
#define MAX_CAMION_ATTENTE 100
#define MAX_BATEAU_PORTIQUE 50
#define MAX_WAGON_TRAIN 5
#define MAX_CONTENEURS_WAGON 50

#define ESPACE_CONTENEUR_VIDE 0
#define CONTENEUR_POUR_BATEAU 1
#define CONTENEUR_POUR_TRAIN 2
#define CONTENEUR_POUR_CAMION 3

#define TRUE 1
#define FALSE 0

#define FICHIER "Makefile"
#define FICHIER_CAMION "Camion.c"
#define FICHIER_BATEAU "Bateau.c"
#define FICHIER_TRAIN "Train.c"
#define FICHIER_PORTIQUE "Portique.c"

// Fonction permettant de créer un bateau
void * bateau(void *);
// Fonction permettant de créer un portique
void * portique(void *);
// Fonction permettant de créer un train
void *train(void *);
// Fonction permettant de créer un camion
void *camion(void *);

// Structure contenant les conteneurs des camions
typedef struct {
    // Mutex pour l'utilisation de la structure
    pthread_mutex_t mutex;
    // Nombre de camions qu'un portique peut accéder à un moment donné
    int nb_camion_par_portique;
    // Tableau contenant les conteneurs des camions accessiblent par les portiques
    int espace_portique[2][MAX_CAMION_PORTIQUE];
} stockage_camion;

// Structure contenant les conteneurs d'un bateau
typedef struct {
    // Mutex pour l'utilisation de la structure
    pthread_mutex_t mutex;
    // Nombre de conteneurs qu'un portique peut accéder à un moment donné
    int nb_conteneurs_par_partie_du_bateau;
    // Tableau contenant les conteneurs du bateau accessiblent par les portiques
    int espace_portique[2][MAX_BATEAU_PORTIQUE];
} stockage_bateau;

// Structure contenant les conteneurs d'un train
typedef struct
{
    // Mutex pour l'utilisation de la structure
    pthread_mutex_t mutex;
    // Nombre de conteneurs qu'un wagon possède
    int nb_conteneurs_par_wagon;
    // Nombre de wagons accessible par un portique à un moment donné
    int nb_wagon_par_partie_du_train;
    // Tableau contenant les conteneurs du train accessiblent par les portiques
    int espace_portique[2][MAX_WAGON_TRAIN][MAX_CONTENEURS_WAGON];
} stockage_train;

// Structure utilisé pour envoyé des messages aux camions

typedef struct
{
    // Destinataire du message = emplacement du camion +1
    long type;
    // Boolean indiquant si le camion envoie ou recoit un conteneur
    int envoie_conteneur;
    // Cette variable est utilisée si le camion envoie son conteneur. 
    // La variable précise la destination du conteneur envoyé
    int desinataire;
    // Cette variable est utilisée si le camion envoie son conteneur au train. 
    // La variable précise le wagon destinataire
    int train_wagon;
    // Cette variable est utilisée si le camion envoie son conteneur au train. 
    // La variable précise l'emplacement du conteneur dans le wagon
    int wagon_emplacement;
    // Cette variable est utilisée si le camion envoie son conteneur au bateau.
    // Cette variable précise l'emplacement du conteneur sur le bateau
    int bateau_emplacement;
    // Cette variable contient le numéro de voie du portique
    int voie_portique;
    // Boolean indiquant si le camion doit rejoindre la file d'attente des camions
    int attente;
    // Variable utilisée si la variable attente est vraie
    // Précise le numéro du camion dans la file d'attente
    int num_attente;
} message_camion;

// Structure utilisé pour envoyé des messages au bateau
typedef struct
{
    // Type du message
    long type;
    // Boolean indiquant si le bateau envoie ou recoit un conteneur
    int envoie_conteneur;
    // Cette variable est utilisée si le bateau envoie un conteneur.
    // La variable précise la destination du conteneur envoyé
    int destinataire;
    // Cette variable est utilisée si le bateau envoie un conteneur à un camion
    // Indique quel camion est destinataire du conteneur
    int camion_destinataire;
    // Indique la voie où se trouve le conteneur
    int voie_portique;
    // Indique l'emplacement du conteneur sur la voie voie_portique
    int emplacement_conteneur;
    // Cette variable est utilisée si le bateau envoie un conteneur au train
    // Précise quel wagon du train est destinataire du conteneur
    int train_wagon;
    // Cette variable est utilisée si le bateau envoie un conteneur au train
    // Précise quel est l'emplacement du conteneur dans le wagon train_wagon
    int wagon_emplacement;
} message_bateau;

// Structure utilisé pour envoyé des messages aux portique
typedef struct
{
    // Type du message, correspond au numéro du portique +1
    long type;
    // Précise quel est type de véhicule qui va recevoir le conteneur
    int destinataire;
    // Cette variable est utilisée si le destinataire du conteneur est un camion
    // Indique l'emplacement du camion
    int camion_destinataire;
    // Cette variable est utilisée si le destinataire du conteneur est le train
    // Indique le wagon destinataire du conteneur
    int train_wagon;
    // Cette variable est utilisée si le destinataire du conteneur est le train
    // Indique l'emplacement du conteneur dans le wagon
    int wagon_emplacement;
    // Cette variable est utilisée si le destinataire du conteneur est le bateau
    // Indique l'emplacement du conteneur sur le bateau
    int bateau_emplacement;
} message_portique;

// Structure utilisé pour envoyé des messages au train
typedef struct
{
    // Type du message
    long type;
    // Boolean indiquant si le train envoie ou recoit un conteneur
    int envoie_conteneur;
    // Cette variable est utilisée si le bateau envoie un conteneur.
    // La variable précise la destination du conteneur envoyé
    int destinataire;
    // Cette variable est utilisée si le train envoie un conteneur à un camion
    // Indique quel camion est destinataire du conteneur
    int camion_destinataire;
    // Indique la voie où se trouve le conteneur
    int voie_portique;
    // Indique quel wagon récupère ou envoie un conteneur
    int wagon;
    // Indique l'emplacement du conteneur dans le wagon
    int wagon_emplacement;
    // Cette variable est utilisée si le train envoie un conteneur au bateau
    // Indique quel est l'emplacement du conteneur sur le bateau
    int bateau_emplacement;
} message_train;

// Structure utilisé pour envoyé des messages superviseur afin de lui indiqué qu'un des ordres qu'il a envoyé est terminé
typedef struct
{
    // Type du message
    long type;
    // Boolean indiquant si un moyen de transport quitte le terminal de transport ou non
    int plein_conteneurs;
    // Indique quel type de transport a envoyé le message
    int type_transport;
    // Cette variable est utilisée si c'est un camion qui a envoyé le message
    // Précise la voie du camion
    int camion_voie_portique;
    // Cette variable est utilisée si c'est un camion qui a envoyé le message
    // Précise l'emplacement du camion sur la voie
    int camion_emplacement;
} message_fin_ordre_superviseur;

// Structure utilisé pour envoyé des messages aux camions à leur création
typedef struct 
{
    // Type du message
    long type;
    // Boolean indiquant si un camion attend dans la file d'attente ou non
    int attente;
    // Cette variable est utilisée si attente est vraie
    // Précise le numéro d'attente du camion
    int num_attente;
    // Cette variable est utilisée si attente est fause
    // Précise la voie sur laquelle le camion se positionne
    int voie_portique;
    // Cette variable est utilisée si attente est fause
    // Précise l'emplacement du camion sur la voie
    int emplacement_portique;

} message_creation_camion;

// Structure utilisé pour envoyé des messages de confirmations 
typedef struct 
{
    // Type du message
    long type;
}message_retour;

// Structure utilisé pour envoyé des messages aux camions qui attendent
typedef struct
{
    // type du message, correspond au numéro d'attente d'un camion
    long type;
    // Indique sur quelle voie va se positionné le camion qui attend
    int voie_portique;
    // Indique l'emplacement sur la voie du portique
    int emplacement_portique;
    // Boolean indiquant si le camion qui attend doit faire attendre un autre camion
    int endormir_camion;
} message_attente_camion;

#endif