#include <stdio.h>
#include <stdlib.h>
#include "projet.h"

int work(int x)
{
    return x * x;
}

void printTableau(int *tab, int count)
{
    printf("array :\n");
    int i = 0;

    for (i = 0 ; i < count ; ++i)
    {
        /* code */
        printf("%d \n", tab[i]);
    }
}

int main(int argc, char **argv)
{
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int(*pointerWork)(int);
    pointerWork = work;

    int nbTask = 11;
    int chunk_min = 2;

    int *resultatTab = (int*)calloc(nbTask, sizeof(int));

    mpiForScheduled(nbTask, chunk_min, pointerWork, resultatTab);

    if (rank == 0)
    {
        printf("final result\n");
        printTableau(resultatTab, nbTask);
    }

    free(resultatTab);
    MPI_Finalize();

    return 0;
}
