#ifndef TERMINALE_H
#define TERMINALE_H

#define MAX_CAMION_PORTIQUE 50

#define ESPACE_CONTENEUR_VIDE 0
#define CONTENEUR_POUR_BATEAU 1
#define CONTENEUR_POUR_TRAIN 2
#define CONTENEUR_POUR_CAMION 3

void * bateau(void *);
void * portique(void *);
void * train(void *);
void * camion(void *);

typedef struct {
    // Nombre de camions qu'un portique peut accéder à un moment donné
    int nb_camion_par_portique;
    int espace_portique1[MAX_CAMION_PORTIQUE];
    int espace_portique2[MAX_CAMION_PORTIQUE];
} stockage_camion;

#endif