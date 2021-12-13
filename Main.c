#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "Terminal.h"

pthread_t id_bateau, id_portique[2], id_camion[5], id_train;

int main(int argc, char const *argv[]) {
    pthread_t num_thread;
    int nb_camion;
    // Vérification du nombre d'arguments passés
    if(argc < 2) {
        printf("Usage: %s <nbCamion>\n", argv[0]);
        return EXIT_FAILURE;
    }
    nb_camion = atoi(argv[1]);
    pthread_create(&id_bateau, NULL, bateau, NULL);
    pthread_create(&id_portique[0], NULL, portique, NULL);
    pthread_create(&id_portique[1], NULL, portique, NULL);

    for(int i = 0; i<nb_camion; ++i) {
        pthread_create(&id_camion[i], NULL, camion, NULL);
    }
    pthread_create(&id_train, NULL, train, NULL);
    pause();

    return EXIT_SUCCESS;
}
