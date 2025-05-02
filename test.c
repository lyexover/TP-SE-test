#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_BUS_X 5
#define NUM_BUS_Y 4
#define NUM_TRIPS 10

// Sens de circulation
typedef enum { X_TO_Y, Y_TO_X, NONE } Direction;

pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   cv_X = PTHREAD_COND_INITIALIZER;
pthread_cond_t   cv_Y = PTHREAD_COND_INITIALIZER;

int in_tunnel   = 0;      // Nombre de bus actuellement dans le tunnel
Direction dir   = NONE;   // Direction courante
tt
int waiting_X   = 0;      // Bus X→Y en attente
int waiting_Y   = 0;      // Bus Y→X en attente

// Entrée dans le tunnel avec circulation groupée
void enter_tunnel(Direction my_dir) {
    pthread_mutex_lock(&mutex);
    if (my_dir == X_TO_Y) waiting_X++;
    else                  waiting_Y++;

    // Attendre si un groupe adverse est dans le tunnel
    while (in_tunnel > 0 && dir != my_dir) {
        if (my_dir == X_TO_Y)
            pthread_cond_wait(&cv_X, &mutex);
        else
            pthread_cond_wait(&cv_Y, &mutex);
    }

    // Je peux entrer
    if (my_dir == X_TO_Y) waiting_X--;
    else                  waiting_Y--;

    dir = my_dir;
    in_tunnel++;
    pthread_mutex_unlock(&mutex);
}

// Sortie du tunnel et possible bascule de direction
void exit_tunnel(Direction my_dir) {
    pthread_mutex_lock(&mutex);
    in_tunnel--;

    // Si dernier bus du groupe
    if (in_tunnel == 0) {
        // Priorité à l'autre côté s'il y a des bus en attente
        if ((my_dir == X_TO_Y && waiting_Y > 0) ||
            (my_dir == Y_TO_X && waiting_X > 0)) {
            dir = (my_dir == X_TO_Y ? Y_TO_X : X_TO_Y);
            if (dir == X_TO_Y) pthread_cond_broadcast(&cv_X);
            else               pthread_cond_broadcast(&cv_Y);
        }
        // Sinon, continuer même sens si attente
        else if ((my_dir == X_TO_Y && waiting_X > 0) ||
                 (my_dir == Y_TO_X && waiting_Y > 0)) {
            if (dir == X_TO_Y) pthread_cond_broadcast(&cv_X);
            else               pthread_cond_broadcast(&cv_Y);
        }
        else {
            dir = NONE;
        }
    }

    pthread_mutex_unlock(&mutex);
}

// Fonction thread pour bus de X
void* bus_X(void* arg) {
    int id = *(int*)arg;
    for (int i = 1; i <= NUM_TRIPS; i++) {
        enter_tunnel(X_TO_Y);
        printf("Bus %d de X : X->Y (Trajet %d)\n", id, i);
        usleep((rand() % 501 + 1000) * 1000);
        exit_tunnel(X_TO_Y);

        enter_tunnel(Y_TO_X);
        printf("Bus %d de X : Y->X (Trajet %d)\n", id, i);
        usleep((rand() % 501 + 1000) * 1000);
        exit_tunnel(Y_TO_X);
    }
    return NULL;
}

// Fonction thread pour bus de Y
void* bus_Y(void* arg) {
    int id = *(int*)arg;
    for (int i = 1; i <= NUM_TRIPS; i++) {
        enter_tunnel(Y_TO_X);
        printf("Bus %d de Y : Y->X (Trajet %d)\n", id, i);
        usleep((rand() % 501 + 1000) * 1000);
        exit_tunnel(Y_TO_X);

        enter_tunnel(X_TO_Y);
        printf("Bus %d de Y : X->Y (Trajet %d)\n", id, i);
        usleep((rand() % 501 + 1000) * 1000);
        exit_tunnel(X_TO_Y);
    }
    return NULL;
}

int main() {
    pthread_t buses_X[NUM_BUS_X];
    pthread_t buses_Y[NUM_BUS_Y];
    int ids_X[NUM_BUS_X];
    int ids_Y[NUM_BUS_Y];

    srand(time(NULL));

    // Création des threads bus X
    for (int i = 0; i < NUM_BUS_X; i++) {
        ids_X[i] = i + 1;
        pthread_create(&buses_X[i], NULL, bus_X, &ids_X[i]);
    }

    // Création des threads bus Y
    for (int i = 0; i < NUM_BUS_Y; i++) {
        ids_Y[i] = i + 1;
        pthread_create(&buses_Y[i], NULL, bus_Y, &ids_Y[i]);
    }

    // Attente de la fin
    for (int i = 0; i < NUM_BUS_X; i++) pthread_join(buses_X[i], NULL);
    for (int i = 0; i < NUM_BUS_Y; i++) pthread_join(buses_Y[i], NULL);

    printf("Simulation terminée.\n");
    return 0;
}
