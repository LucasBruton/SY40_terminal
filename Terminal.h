#ifndef TERMINALE_H
#define TERMINALE_H

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

void * bateau(void *);
void * portique(void *);
void * train(void *);
void * camion(void *);

// Structure contenant les conteneurs des camions
typedef struct {
    pthread_mutex_t mutex;
    // Nombre de camions qu'un portique peut accéder à un moment donné
    int nb_camion_par_portique;
    // Tableau contenant les conteneurs des camions accessible par le portique 1 et le portique 2
    int espace_portique[2][MAX_CAMION_PORTIQUE];
} stockage_camion;

// Structure contenant les conteneurs d'un bateau
typedef struct {
    pthread_mutex_t mutex;
    int nb_conteneurs_par_partie_du_bateau;
    int espace_portique[2][MAX_BATEAU_PORTIQUE];
} stockage_bateau;

// Structure contenant les conteneurs d'un train
typedef struct
{
    pthread_mutex_t mutex;
    int nb_conteneurs_par_wagon;
    int nb_wagon_par_partie_du_train;
    int espace_portique[2][MAX_WAGON_TRAIN][MAX_CONTENEURS_WAGON];
} stockage_train;

typedef struct
{
    long type;
    int envoie_conteneur;
    int desinataire;
    int train_wagon;
    int wagon_emplacement;
    int bateau_emplacement;
    int voie_portique;
    int attente;
    int num_attente;
} message_camion;

typedef struct
{
    long type;
    int envoie_conteneur;
    int destinataire;
    int camion_destinataire;
    int voie_portique;
    int emplacement_conteneur;
    int train_wagon;
    int wagon_emplacement;
} message_bateau;

typedef struct
{
    long type;
    int quitter_terminal;
    int transport;
    int camion_emplacement;
} message_transport_superviseur;


typedef struct
{
    long type;
    int destinataire;
    int camion_destinataire;
    int train_wagon;
    int wagon_emplacement;
    int bateau_emplacement;
} message_portique;

typedef struct
{
    long type;
    int envoie_conteneur;
    int destinataire;
    int camion_destinataire;
    int voie_portique;
    int wagon;
    int wagon_emplacement;
    int bateau_emplacement;
} message_train;

typedef struct
{
    long type;
    int plein_conteneurs;
    int type_transport;
    int camion_voie_portique;
    int camion_emplacement;
} message_fin_ordre_superviseur;

typedef struct 
{
    long type;
    int voie_portique;
    int emplacement_portique;
    int attente;
    int num_attente;
} message_creation_camion;

typedef struct 
{
    long type;
}message_retour;

typedef struct
{
    long type;
    int voie_portique;
    int emplacement_portique;
    int endormir_camion;
} message_attente_camion;

#endif