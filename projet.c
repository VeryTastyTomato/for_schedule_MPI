#include "projet.h"

// private
int masterFunction(MPI_Comm masterComm, MPI_Comm comm, int nbTask, int chunkMin, int *resultArray);
int slaveFunction(MPI_Comm masterComm, MPI_Comm comm, int nbTask, int (*pointerWork)(int));

// printing functions
void display(int nb);
void displayArray(int *array, int count);

int verbose = 1;

void mpiForScheduled(int nbTask, int chunkMin, int (*pointerWork)(int), int *resultArray)
{
	srand(time(NULL));

	int rank = 0;
	int index = 0;
	int size = 0;

	MPI_Comm newComm;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// check the minimum chunk
	if (chunkMin <= 0)
	{
		chunkMin = 1;
	}

    // master / slave model
	MPI_Comm_split(MPI_COMM_WORLD, 0 == rank, 0, &newComm);

	if (0 == rank)
	{
		masterFunction(MPI_COMM_WORLD, newComm, nbTask, chunkMin, resultArray);
	}
	else
	{
		slaveFunction(MPI_COMM_WORLD, newComm, nbTask, pointerWork);
	}
}

int masterFunction(MPI_Comm masterComm, MPI_Comm comm, int nbTask, int chunkMin, int *resultArray)
{
	int i, j, k, size;
	int nbMaster;
	int nbSlave;

	char buf[256];
	MPI_Status status;

	int *taskArray = (int*) malloc(sizeof(int));
	int taskIndex = 0;

	int nbRest = nbTask;
	int chunk = chunkMin;
	int resultRestCount = nbRest;

	MPI_Comm_size(masterComm, &size);
	MPI_Comm_size(comm, &nbMaster);

	nbSlave = size - nbMaster;

	vprintf("Number of processors: %d\n", size);
	vprintf("Number of slaves: %d\n", nbSlave);

	int resultCount = 0;
	int resultIndex = 0;

	// either we still have to work or chunk is not equal to zero
	while (nbRest || chunk != 0)
	{
        // slave is free
		MPI_Recv(&resultCount, 1, MPI_INT, MPI_ANY_SOURCE, 0, masterComm, &status);

		if (0 == resultCount)
		{
			vprintf("Slave %d is free\n", status.MPI_SOURCE);
		}
		else
		{
			vprintf("Slave %d sent this result size %d\n", status.MPI_SOURCE, resultCount);
			MPI_Recv(&resultIndex, 1, MPI_INT, status.MPI_SOURCE, 0, masterComm, &status);
			vprintf("Result index %d\n", resultIndex);

    		// ready to receive
			int *resArray = (int*) calloc(resultCount, sizeof(int));
			MPI_Recv(resArray, resultCount, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);

			// getting the array up to day
            for (k = 0 ; k < resultCount ; k++)
			{
				resultArray[resultIndex] = resArray[k];
				resultIndex++;
			}

			resultRestCount -= resultCount;

			free(resArray);
		}

		chunk = nbRest / nbSlave;

		if ((0 == chunk) && (nbRest <= chunkMin))
		{
			chunk = nbRest;
		}
		else if (chunk < chunkMin)
		{
			chunk = chunkMin;
		}

		if (chunk)
		{
            // send chunk size
			MPI_Send(&chunk, 1, MPI_INT, status.MPI_SOURCE, 0, masterComm);

			int *tmpTaskArray = (int*) realloc(taskArray, chunk * sizeof(int));

			if (!tmpTaskArray)
			{
				vprintf("Reallocation failed\n");

				return 1;
			}

			taskArray = tmpTaskArray;
			resultIndex = taskIndex;

			MPI_Send(&resultIndex, 1, MPI_INT, status.MPI_SOURCE, 0, masterComm);

            // sending a chunk to a free slave
			for (j = 0 ; j < chunk ; j++)
			{
				taskArray[j] = taskIndex;
				taskIndex++;
			}

			vprintf("Task array\n");
			displayArray(taskArray, chunk);
			vprintf("\n");
			MPI_Send(taskArray, chunk, MPI_INT, status.MPI_SOURCE, 0, masterComm);

            nbRest = nbRest - chunk;
		}
	}

	// receving the pending result
	while (resultRestCount > 0)
	{
		MPI_Recv(&resultCount, 1, MPI_INT, MPI_ANY_SOURCE, 0, masterComm, &status);

		if (resultCount != 0)
		{
			MPI_Recv(&resultIndex, 1, MPI_INT, status.MPI_SOURCE, 0, masterComm, &status);

			// ready for receiving
			int *resArray = (int*) calloc(resultCount, sizeof(int));
			MPI_Recv(resArray, resultCount, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);

	        // getting the array up to day
			for (k = 0 ; k < resultCount ; k++)
			{
				resultArray[resultIndex] = resArray[k];
				resultIndex++;
				resultRestCount--;
			}

			free(resArray);
		}
	}

    for (i = 1 ; i <= nbSlave ; i++)
	{
		MPI_Send(&chunk, 1, MPI_INT, i, 0, masterComm);
	}

	for (i = 1 ; i <= nbSlave ; i++)
	{
        // slave is out
		MPI_Recv(buf, 256, MPI_CHAR, i, 0, masterComm, &status);
        vprintf("%s", buf);
        vprintf("\n");
    }

    free(taskArray);
}

int slaveFunction(MPI_Comm masterComm, MPI_Comm comm, int nbTask, int (*pointerWork)(int))
{
	int i;
	int *taskArray = (int*) malloc(sizeof(int));
	int chunkSize = -1;
	char buf[256];

	int rank;
	MPI_Status status;

	int zero = 0;
	int resultIndex = 0;
	MPI_Comm_rank(comm, &rank);

	// slave is free
	MPI_Send(&zero, 1, MPI_CHAR, 0, 0, masterComm);

	while (chunkSize != 0)
	{
        // receiving chunk size
        MPI_Recv(&chunkSize, 1, MPI_INT, 0, 0, masterComm, &status);
        vprintf("Slave %d received a chunk size: %d\n", rank, chunkSize);

        if (chunkSize != 0)
		{
        	// slave receiving the index
        	MPI_Recv(&resultIndex, 1, MPI_INT, 0, 0, masterComm, &status);

        	int *tmpTaskArray = (int*) realloc(taskArray, chunkSize * sizeof(int));

        	if (!tmpTaskArray)
			{
        		vprintf("Reallocation failed\n");

        		return 1;
        	}

        	taskArray = tmpTaskArray;

        	MPI_Recv(taskArray, chunkSize, MPI_INT, 0, 0, masterComm, &status);

        	display(rank);
        	vprintf("Slave %d received some tasks\n", rank);

            int *resultArray = (int*) calloc(chunkSize, sizeof(int));

        	for (i = 0 ; i < chunkSize ; i++)
			{
        		int result = (*pointerWork)(taskArray[i]);

        		resultArray[i] = result;
        	}

            // sending some info to the master
            // sending the size
            MPI_Send(&chunkSize, 1, MPI_INT, 0, 0, masterComm);
            // sending the index
            MPI_Send(&resultIndex, 1, MPI_INT, 0, 0, masterComm);
            // sending the results
        	MPI_Send(resultArray, chunkSize, MPI_INT, 0, 0, masterComm);

        	free(resultArray);
        }
    }

    // send a signal to master
    display(rank); // for display
    sprintf(buf, "Slave %d is out\n", rank);
    MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, 0, 0, masterComm);

    free(taskArray);

    return 0;
}

void display(int nb)
{
	int i;

	for (i = 0 ; i <= nb ; i++)
	{
		vprintf("  ");
	}
}

void displayArray(int *array, int count)
{
	int i = 0;

	vprintf("Array:\n");

	for (i = 0 ; i < count ; i++)
	{
        vprintf("%d\n", array[i]);
	}
}
