#include "projet.h"

// private
int master_Function(MPI_Comm master_comm, MPI_Comm comm, int nbTask, int chunk_min, int *resultatTab);
int slave_Function(MPI_Comm master_comm, MPI_Comm comm, int nbTask, int(*pointerWork)(int));

// printing functions
void dispay(int nb);
void dispayTab(int *tab, int count);

int verbose = 1;

void mpiForScheduled(int nbTask, int chunk_min, int(*pointerWork)(int), int *resultatTab)
{
	srand(time(NULL));

	int processus;
	int nbTaskRest = nbTask;

	int rank = 0;
	int nombreMachine = 0;

	int index = 0;
	int size = 0;

	MPI_Comm new_comm;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// verifier le chunk min
	if (chunk_min <= 0)
	{
		chunk_min = 1;
	}

    // Modele maitre / esclave
	MPI_Comm_split(MPI_COMM_WORLD, rank == 0, 0, &new_comm);

	if (rank == 0)
	{
		master_Function(MPI_COMM_WORLD, new_comm, nbTask, chunk_min, resultatTab);
	}
	else
	{
		slave_Function(MPI_COMM_WORLD, new_comm, nbTask, pointerWork);
	}
}

int master_Function(MPI_Comm master_comm, MPI_Comm comm, int nbTask, int chunk_min, int *resultatTab)
{
	int i, j, k, size;
	int nombreMaster;
	int nombreSlave;
	char buf[256];
	MPI_Status status;

	int *taskTab = (int *)malloc(sizeof(int));
	int taskIndex = 0;

	int nombreRest = nbTask;
	int chunk = chunk_min;

	int resultRestCount = nombreRest;

	MPI_Comm_size(master_comm, &size);
	MPI_Comm_size(comm, &nombreMaster);
	vprintf("number of procs : %d\n", size);
	nombreSlave = size - nombreMaster;
	vprintf("number of slave : %d\n", nombreSlave);

	int ResultCount = 0;
	int ResultIndex = 0;

	// either we still have to work or chunk is not equal to zero
	while (nombreRest || chunk != 0)
	{
        // slave is free
		MPI_Recv(&ResultCount, 1, MPI_INT, MPI_ANY_SOURCE, 0, master_comm, &status);

		if (ResultCount == 0)
		{
			vprintf("slave %d is free \n", status.MPI_SOURCE);
		}
		else
		{
			vprintf("slave %d sent this result size %d \n", status.MPI_SOURCE, ResultCount);
			MPI_Recv(&ResultIndex, 1, MPI_INT, status.MPI_SOURCE, 0, master_comm, &status);
			vprintf("result index %d \n", ResultIndex);

    		// ready to receive
			int *resulatTab = (int*)calloc(ResultCount, sizeof(int));
			MPI_Recv(resulatTab, ResultCount, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);

			// getting the array up to day
            for (k = 0 ; k < ResultCount ; k++)
			{
				resultatTab[ResultIndex] = resulatTab[k];
				ResultIndex++;
			}

			resultRestCount -= ResultCount;
			free(resulatTab);
		}

		chunk = nombreRest / nombreSlave;

		if ((chunk == 0) && (nombreRest <= chunk_min))
		{
			chunk = nombreRest;
		}
		else if (chunk < chunk_min)
		{
			chunk = chunk_min;
		}

		if (chunk)
		{
            // send chunk size
			MPI_Send(&chunk, 1, MPI_INT, status.MPI_SOURCE, 0, master_comm);

			int *tmpTaskTab = (int*)realloc(taskTab, chunk * sizeof(int));

			if (!tmpTaskTab)
			{
				vprintf("reallocation failed\n");
				return 0;
			}

			taskTab = tmpTaskTab;

			ResultIndex = taskIndex;
			MPI_Send(&ResultIndex, 1, MPI_INT, status.MPI_SOURCE, 0, master_comm);

            // sending a chunk to a free slave
			for (j = 0 ; j < chunk ; j++)
			{
				taskTab[j] = taskIndex;
				taskIndex++;
			}

			vprintf("task tab\n");
			dispayTab(taskTab, chunk);
			vprintf("\n");
			MPI_Send(taskTab, chunk, MPI_INT, status.MPI_SOURCE, 0, master_comm);

            nombreRest = nombreRest - chunk;
		}
	}

	// receving the pending result
	while (resultRestCount > 0)
	{
		MPI_Recv(&ResultCount, 1, MPI_INT, MPI_ANY_SOURCE, 0, master_comm, &status);

		if (ResultCount != 0)
		{
			MPI_Recv(&ResultIndex, 1, MPI_INT, status.MPI_SOURCE, 0, master_comm, &status);

			// ready for receiving
			int *resulatTab = (int*)calloc(ResultCount, sizeof(int));
			MPI_Recv(resulatTab, ResultCount, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);

	        // getting the array up to day
			for (k = 0 ; k < ResultCount ; k++)
			{
				resultatTab[ResultIndex] = resulatTab[k];
				ResultIndex++;
				resultRestCount--;
			}

			free(resulatTab);
		}
	}

    for (i = 1 ; i <= nombreSlave ; ++i)
	{
		MPI_Send(&chunk, 1, MPI_INT, i, 0, master_comm);
	}

	for (i = 1 ; i <= nombreSlave ; i++)
	{
        // slave is out
		MPI_Recv(buf, 256, MPI_CHAR, i, 0, master_comm, &status);
        vprintf("%s", buf);
        vprintf("\n");
    }

    free(taskTab);
}

/* Fonction du slave */
int slave_Function(MPI_Comm master_comm, MPI_Comm comm, int nbTask, int(*pointerWork)(int))
{
	int i;
	int *taskTab = (int *)malloc(sizeof(int));
	int chunkSize = -1;
	char buf[256];

	int rank;
	MPI_Status status;

	int zero = 0;
	int ResultIndex = 0;
	MPI_Comm_rank(comm, &rank);

	// slave is free
	MPI_Send(&zero, 1, MPI_CHAR, 0, 0, master_comm);

	while (chunkSize != 0)
	{
        // receiving chunk size
        MPI_Recv(&chunkSize, 1, MPI_INT, 0, 0, master_comm, &status);
        vprintf("slave %d receved a chunk size : %d\n", rank, chunkSize);

        if (chunkSize != 0)
		{
        	// slave receiving the index
        	MPI_Recv(&ResultIndex, 1, MPI_INT, 0, 0, master_comm, &status);

        	int *tmpTaskTab = (int*)realloc(taskTab, chunkSize * sizeof(int));

        	if (!tmpTaskTab)
			{
        		vprintf("reallocation failed\n");

        		return 1;
        	}

        	taskTab = tmpTaskTab;

        	MPI_Recv(taskTab, chunkSize, MPI_INT, 0, 0, master_comm, &status);

        	dispay(rank);
        	vprintf("slave %d receved some tasks \n", rank);

            int *resultatTab = (int*)calloc(chunkSize, sizeof(int));

        	for (i = 0 ; i < chunkSize ; ++i)
			{
        		int resultat = (*pointerWork)(taskTab[i]);
        		resultatTab[i] = resultat;
        	}

            // sending some info to the master
            // sending the size
            MPI_Send(&chunkSize, 1, MPI_INT, 0, 0, master_comm);
            // sending the index
            MPI_Send(&ResultIndex, 1, MPI_INT, 0, 0, master_comm);
            // sending the results
        	MPI_Send(resultatTab, chunkSize, MPI_INT, 0, 0, master_comm);

        	free(resultatTab);
        }
    }

    // send a signal to master
    dispay(rank); // for display
    sprintf(buf, "slave %d is out\n", rank);
    MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, 0, 0, master_comm);
    free(taskTab);

    return 0;
}

void dispay(int nb)
{
	int i;

	for (i = 0 ; i <= nb ; i++)
	{
		vprintf("  ");
	}
}

void dispayTab(int *tab, int count)
{
	int i = 0;

	vprintf("array :\n");

	for (i = 0 ; i < count ; ++i)
	{
        vprintf("%d \n", tab[i]);
	}
}
