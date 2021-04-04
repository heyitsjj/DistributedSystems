#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define TAGINIT 0
#define TAGANNOUNCE 1
#define NB_SITES 6
#define STARTER 0


int main (int argc, char* argv[]) {
    int nb_proc, rank, successor, my_value, message;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    successor = (rank + 1) % nb_proc;
    srand(time(NULL) + rank);
    my_value =  rand()%100+1;
    printf("%d> local value is %d\n", rank, my_value);

    if (rank == STARTER) {
        MPI_Send(&my_value, 1, MPI_INT, successor, TAGINIT, MPI_COMM_WORLD);
        MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
        if (message < my_value)
            my_value = message;
        printf("%d> minimum value is %d\n", rank, my_value);
        MPI_Send(&my_value, 1, MPI_INT, successor, TAGANNOUNCE, MPI_COMM_WORLD);
    } else {
        MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
        if (message < my_value)
            my_value = message;
        MPI_Send(&my_value, 1, MPI_INT, successor, TAGINIT, MPI_COMM_WORLD);
        MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, TAGANNOUNCE, MPI_COMM_WORLD, &status);
        my_value = message;
        printf("%d> minimum value is %d\n", rank, my_value);
        if (rank < nb_proc - 1)
            MPI_Send(&my_value, 1, MPI_INT, successor, TAGANNOUNCE, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}
