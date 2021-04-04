#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define TAGINIT 0
#define TAGCOMPUTE 1
#define TAGANNOUNCE 2
#define NB_SITES 6


void simulator(void) {
    int i;
    /* nb_neighbors[i] is the number of neighbors of node i */
    int nb_neighbors[NB_SITES+1] = {-1, 2, 3, 2, 1, 1, 1};
    int local_min[NB_SITES+1] = {-1, 3, 11, 8, 14, 5, 17};
    /* neighbor ists */
    int neighbors[NB_SITES+1][3] = {{-1, -1, -1}, {2, 3, -1}, {1, 4, 5},
        {1, 6, -1}, {2, -1, -1}, {2, -1, -1}, {3, -1, -1}};
    for(i=1; i<=NB_SITES; i++){
        MPI_Send(&nb_neighbors[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
        MPI_Send(neighbors[i],nb_neighbors[i], MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
        MPI_Send(&local_min[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
    }
}

void compute_min(int rank) {
    MPI_Status status;
    int my_degree;
    int my_neighbors[3];
    int my_local_min;
    /**** TODO: Complete local variables ****/
    int received_channels[3];
    int received_nb = 0;
    int decision = 0;
    int emitter = -1;
    int sent = 0;
    int message;

    MPI_Recv(&my_degree, 1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
    MPI_Recv(my_neighbors, my_degree, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&my_local_min, 1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);

    printf("%d(d=%d)> Starting TREE algorithm with local value %d\n", rank, my_degree, my_local_min);

    /**** TODO: Complete tree algorithm ****/
    for (int i = 0; i < 3; i++)
        received_channels[i] = 0;
    
    while (decision == 0) {
        /* SEND MESSAGE? */
        if ((!sent) && (received_nb == my_degree - 1))
            for (int i = 0; i < 3; i++)
                if ((my_neighbors[i] != -1) && (received_channels[i] == 0)) {
                    MPI_Send(&my_local_min, 1, MPI_INT, my_neighbors[i], TAGCOMPUTE, MPI_COMM_WORLD);
                    sent = 1;
                }

        /* RECEIVE MESSAGE */
        MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        received_nb++;
        emitter = status.MPI_SOURCE;
        if (my_local_min > message)
            my_local_min = message;
        for (int i = 0; i < 3; i++)
            if (my_neighbors[i] == emitter)
                received_channels[i] = 1;

        /* DECIDE AND ANNOUNCE */
        if (received_nb == my_degree) {
            decision = 1;
            if (status.MPI_TAG == TAGCOMPUTE)
                printf("%d> I have decided %d\n", rank, my_local_min);
            if (status.MPI_TAG == TAGANNOUNCE)
                printf("%d> Someone else has decided %d\n", rank, my_local_min);
            for (int i = 0; i < 3; i++)
                if ((my_neighbors[i] != -1) && (my_neighbors[i] != emitter))
                    MPI_Send(&my_local_min, 1, MPI_INT, my_neighbors[i], TAGANNOUNCE, MPI_COMM_WORLD);
        }
    }
}


int main (int argc, char* argv[]) {
    int nb_proc, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
    if (nb_proc != NB_SITES+1) {
        printf("Number of processes is incorrect!\n");
        MPI_Finalize();
        exit(2);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        simulator();
    } else {
        compute_min(rank);
    }
    MPI_Finalize();
    return 0;
}
