#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define TAGINIT 0          /* Topology setup */
#define TAGDOWN 1          /* Exploration to attach new nodes to the tree */
#define TAGUP 2            /* Backflow of messages (child -> parent) */
#define TAGANNOUNCE 3      /* Propagation of decision (root -> leaves) */
#define NB_SITES 6


void simulator(void) {
    int i;
    /* nb_neighbors[i] is the number of neighbors of node i */
    int nb_neighbors[NB_SITES+1] = {-1, 3, 3, 2, 3, 5, 2};
    int local_min[NB_SITES+1] = {-1, 12, 11, 8, 14, 5, 17};
    /* neighbor lists */
    int neighbors[NB_SITES+1][5] = {{-1, -1, -1, -1, -1},
        {2, 5, 3, -1, -1}, {4, 1, 5, -1, -1},
        {1, 5, -1, -1, -1}, {6, 2, 5, -1, -1},
        {1, 2, 6, 4, 3}, {4, 5, -1, -1, -1}};
    for(i=1; i<=NB_SITES; i++){
        MPI_Send(&nb_neighbors[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
        MPI_Send(neighbors[i],nb_neighbors[i], MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
        MPI_Send(&local_min[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
    }
}

void compute_min(int rank, int starter) {
    MPI_Status status;
    int my_degree;
    int my_neighbors[5];
    int my_local_min;
    /**** TODO: Complete local variables ****/
    int message;
    int decision = 0;
    int i;
    int parent = -1;
    int received_nb = 0;
    int children[5] = {0, 0, 0, 0, 0};


    MPI_Recv(&my_degree, 1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
    MPI_Recv(my_neighbors, my_degree, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&my_local_min, 1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);

    printf("%d(d=%d)> Starting ECHO algorithm with local value %d\n", rank, my_degree, my_local_min);
    if (rank == starter)
        printf("%d> STARTER\n", rank);

    /* STARTER INITIATION */
    if (rank == starter) {
        parent = rank;
        for (i = 0; i < NB_SITES-1; i++)
            if (my_neighbors[i] != -1)
                MPI_Send(&my_local_min, 1, MPI_INT, my_neighbors[i], TAGDOWN, MPI_COMM_WORLD);
    }

    /* MESSAGE RECEPTION LOOP */
    while (!decision) {
        MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        /* update local variables */
        received_nb++;
        if (my_local_min > message)
            my_local_min = message;
        /* first message reception */
        /*      => adopt parent */
        /*      => forward to other neighbors */
        if (parent == -1) {
            parent = status.MPI_SOURCE;
            for (i = 0; i < NB_SITES-1; i++)
                if ((my_neighbors[i] != -1) && (my_neighbors[i] != parent))
                    MPI_Send(&my_local_min, 1, MPI_INT, my_neighbors[i], TAGDOWN, MPI_COMM_WORLD);
        }
        /* backflow message => adopt child */
        if (status.MPI_TAG == TAGUP)
            for (i = 0; i < NB_SITES-1; i++)
                if (status.MPI_SOURCE == my_neighbors[i])
                    children[i] = 1;
        /* last message to be expected in ECHO */
        if (received_nb == my_degree) {
            /* start backflow to parent, then await decision announcement */
            if (rank != starter) {
                MPI_Send(&my_local_min, 1, MPI_INT, parent, TAGUP, MPI_COMM_WORLD);
                MPI_Recv(&message, 1, MPI_INT, parent, TAGANNOUNCE, MPI_COMM_WORLD, &status);
                my_local_min = message;
            }
            decision = 1;
            /* announce decision to children */
            for (i = 0; i < NB_SITES-1; i++)
                if (children[i] == 1)
                    MPI_Send(&my_local_min, 1, MPI_INT, my_neighbors[i], TAGANNOUNCE, MPI_COMM_WORLD);
            /* display decision */
            printf("%d(%d)> The minimum value is %d\n", rank, parent, my_local_min);
        }
    }
    
}


int main (int argc, char* argv[]) {
    int nb_proc, rank;
    int starter; /* starter node given as program argument */
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
        starter = atoi(argv[1]);
        compute_min(rank, starter);
    }
    MPI_Finalize();
    return 0;
}
