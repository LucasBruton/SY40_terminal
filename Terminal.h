#ifndef TERMINALE_H
#define TERMINALE_H

#define MAX_CAMION_PORTIQUE 50
#define MAX_BATEAU_PORTIQUE 50

#define ESPACE_CONTENEUR_VIDE 0
#define CONTENEUR_POUR_BATEAU 1
#define CONTENEUR_POUR_TRAIN 2
#define CONTENEUR_POUR_CAMION 3

#define FICHIER_MUTEX "Makefile"
#define FICHIER_CAMION "Camion.c"
#define FICHIER_BATEAU "Bateau.c"

void * bateau(void *);
void * portique(void *);
void * train(void *);
void * camion(void *);

// Structure contenant les conteneurs des camions
typedef struct {
    // Nombre de camions qu'un portique peut accéder à un moment donné
    int nb_camion_par_portique;
    // Tableau contenant les conteneurs des camions accessible par le portique 1 et le portique 2
    int espace_portique[2][MAX_CAMION_PORTIQUE];
} stockage_camion;

// Structure utilisé pour passer des paramètres à un thread camion
typedef struct {
    int numEspacePortique;
    int numVoiePortique;
} args_camions;

// Structure contenant les conteneurs d'un bateau
typedef struct {
    int nb_conteneurs_par_partie_du_bateau;
    int espace_portique[2][MAX_BATEAU_PORTIQUE];
} stockage_bateau;

// Structure contenant les mutexs utilisés par plusiers threads
typedef struct {
    pthread_mutex_t mutex_stockage_camion;
} struct_mutexs;

#endif