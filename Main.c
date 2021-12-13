#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "Terminal.h"

void *test(void *arg)
{
    int *valeur;
    int test = 2;
    valeur = &test;
    printf("Annexe :  %d \n", *valeur);
}

int main(int argc, char const *argv[])
{
    pthread_t num_thread;

    pthread_create(&num_thread, NULL, bateau, NULL);
    pthread_create(&num_thread, NULL, portique, NULL);
    pthread_create(&num_thread, NULL, camion, NULL);
    pthread_create(&num_thread, NULL, train, NULL);
    pthread_join(num_thread, NULL);
    return EXIT_SUCCESS;
}
