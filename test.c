#include <stdio.h>
#include <stdlib.h>
#include "projet.h"

int work(int x)
{
    return x * x;
}

void printArray(int *array, int count)
{
    int i = 0;

    printf("Array:\n");

    for (i = 0 ; i < count ; i++)
    {
        /* code */
        printf("%d \n", array[i]);
    }
}

int main(int argc, char **argv)
{
    int rank;
    int (*pointerWork)(int);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    pointerWork = work;

    int nbTask = 11;
    int chunkMin = 2;

    int *resultArray = (int*) calloc(nbTask, sizeof(int));

    mpiForScheduled(nbTask, chunkMin, pointerWork, resultArray);

    if (0 == rank)
    {
        printf("Final result:\n");
        printArray(resultArray, nbTask);
    }

    free(resultArray);
    MPI_Finalize();

    return EXIT_SUCCESS;
}
