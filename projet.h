#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#define vprintf(...)(verbose ? printf(__VA_ARGS__) : 0)

void mpiForScheduled(int nbTask, int chunkMin, int (*pointerWork)(int), int *resultArray);
