/*
 ========================================================================================================
 | Name        	: Finding_eccentricities.c								|
 | Author      	: Alexandru Grigoras									|
 | Version     	: 04											|
 | Copyright   	: Copyright Alexandru Grigoras								|
 | Description 	: Finding the eccentricities in a tree							|
 | References	: The algorithm is implemented from							|
 | 	 	 	   		[N. Santoro, Design and Analysis of Distributed Algorithms,	|
 |	 	 	    	Ottawa: WILEY-INTERSCIENCE, 2006]					|
 ========================================================================================================
 */

#include <stdio.h>
#include <string.h>
#include "mpi.h"

// Macros
#define ROOT 0
#define NR_NODES 6
#define TRUE 1
#define FALSE 0

// The status of the node
enum STATUS_ENUM {
	AVAILABLE = 0,
	ACTIVE = 1,
	PROCESSING = 2,
	DONE = 3
};

// The type of the message that is sent to neighbors
enum MESSAGE_TYPE {
	ACTIVATE = 0,
	SATURATION = 1,
	RESOLUTION = 2
};

// Initialization of the distances vector with 0 on all locations
void Initialize(int *dist, int np)
{
	for(int i=0; i<np; i++)
	{
		dist[i] = 0;
	}
}

// Build the message value with the maximum distance from neighbors + 1
int Prepare_Message(int *dist, int np)
{
	int i;
	int maxdist = dist[0];

	for(i=1; i<np; i++)
	{
		if(dist[i] > maxdist)
		{
			maxdist = dist[i];
		}
	}

	maxdist++;

	return maxdist;
}

// Update the distances vector with the received distance from sender
void Process_Message(int *dist, int received_distance, int sender)
{
	dist[sender] = received_distance;
}

// Calculate the node eccentricity
int Calculate_Eccentricities(int *dist, int np)
{
	int i;
	int maxdist = dist[0];

	for(i=1; i<np; i++)
	{
		if(dist[i] > maxdist)
		{
			maxdist = dist[i];
		}
	}

	return maxdist;
}

// Saturated nodes enter in the resolution stage:
// 		- calculate the eccentricity
//		- send the maxdist accordingly to each neighbor
// 		- return the eccentricity
int Resolve(int *dist, int nodes[][NR_NODES], int received_distance, int my_rank, int parent, int sender, int np, MPI_Request request)
{
	int dest, tag;
	int maxdist;
	int message;
	int eccentricity;

	Process_Message(dist, received_distance, sender);
	eccentricity = Calculate_Eccentricities(dist, np);

	for(dest=0; dest<np; dest++)
	{
		if(nodes[my_rank][dest] != 0 && dest != parent)
		{
			maxdist = 0;
			for(int i=0; i<np; i++)
			{
				if((dist[i] > maxdist) && (i != dest))
				{
					maxdist = dist[i];
				}
			}

			tag = RESOLUTION;
			message = maxdist + 1;
			MPI_Isend(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);

			//printf("[%d] I am sending RESOLUTION to %d\n", my_rank, dest);
		}
	}

	return eccentricity;
}

// Display a vector to console
void Print_vector(int *v, int nr){
	printf("V = [ ");
	for(int i=0; i<nr; i++)
	{
		printf("%d ", v[i]);
	}
	printf("]\n");
}

// Main function for all the processes
int main(int argc, char* argv[]){
	int my_rank; 					// rank of process
	int nr_processes;				// number of processes
	int source;   					// rank of sender
	int dest;    					// rank of receiver
	int tag = 0;    				// tag for messages
	int message = 0;				// storage for message
	MPI_Status status;				// return status for receive
	MPI_Request request;			// the request parameter
	
	int i;							// index
	int node_status = AVAILABLE;	// status of the node; initially is set to AVAILABLE
	int parent = -1;				// parent of the node; initially the parent is unknown
	int nr_neighbors = 0;			// the number of neighbors
	int temp_nr_neighbors = 0;		// temporary number of neighbors

	int nodes[NR_NODES][NR_NODES] = {
			{0, 1, 0, 0, 0, 0},
			{1, 0, 1, 1, 1, 0},
			{0, 1, 0, 0, 0, 0},
			{0, 1, 0, 0, 0, 1},
			{0, 1, 0, 0, 0, 0},
			{0, 0, 0, 1, 0, 0}
	};
	/*
	int nodes[NR_NODES][NR_NODES] = {
			{0, 1, 1, 0, 0},
			{1, 0, 0, 1, 1},
			{1, 0, 0, 0, 0},
			{0, 1, 0, 0, 0},
			{0, 1, 0, 0, 0}
	};
	*/

	int distances[NR_NODES];
	int finished = FALSE;
	int eccentricity = -1;
	int neighbors_sum = 0;

	// start up MPI
	MPI_Init(&argc, &argv);
	// find out process rank
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	// find out number of processes
	MPI_Comm_size(MPI_COMM_WORLD, &nr_processes);
	
	// getting the number of neighbors for each node
	for(i=0; i<nr_processes; i++)
	{
		if(nodes[my_rank][i])
		{
			nr_neighbors++;
			neighbors_sum += i;
		}
	}
	// and store it to a temporary variable
	temp_nr_neighbors = nr_neighbors;

	// main loop where each process goes through the available states
	while(!finished)
	{
		switch(node_status)
		{
		// ACTIVATION state:
		// - the root is sending activation messages to its neighbors
		// - other nodes forward the activation to their neighbors
		// - the root chooses the parent the first neighbor
		// - other nodes choose the parent the neighbor if leaf or the last neighbor from which it received a message
		case AVAILABLE:
			if(my_rank == ROOT)
			{
				// sending activation message to neighbors

				printf("[%d] AVAILABLE and sending ACTIVATION to neighbors\n", my_rank);

				for(dest=0; dest<nr_processes; dest++)
				{
					if(nodes[my_rank][dest])
					{
						tag = ACTIVATE;
						message = 0;
						MPI_Isend(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);
					}
				}

				// initialize and send SATURATION to parent and set status to PROCESSING if process is leaf
				// otherwise set status to active
				Initialize(distances, nr_processes);
				if(nr_neighbors == 1)
				{
					for(dest=0; dest<nr_processes; dest++)
					{
						if(nodes[my_rank][dest])
						{
							parent = dest;
						}
					}

					message = Prepare_Message(distances, nr_processes);
					tag = SATURATION;
					dest = parent;

					printf("[%d] AVAILABLE and sending SATURATION to %d\n", my_rank, dest);

					MPI_Isend(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);
					node_status = PROCESSING;
				}
				else
				{
					node_status = ACTIVE;
				}
			}
			else
			{
				// receiving activation message
				MPI_Irecv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
				MPI_Wait(&request, &status);
				source = status.MPI_SOURCE;
				tag = status.MPI_TAG;

				printf("[%d] AVAILABLE and receiving ACTIVATION from %d\n", my_rank, source);

				// forward the activation
				if(tag == ACTIVATE)
				{
					// sending activation message to neighbors, except source
					for(dest=0; dest<nr_processes; dest++)
					{
						if(nodes[my_rank][dest])
						{
							if(dest != source)
							{
								tag = ACTIVATE;
								message = 0;
								MPI_Isend(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);
							}
						}
					}

					// initialize and send SATURATION to parent and set status to PROCESSING if process is leaf
					// otherwise set status to active
					Initialize(distances, nr_processes);
					if(nr_neighbors == 1)
					{
						parent = source;
						tag = SATURATION;
						message = Prepare_Message(distances, nr_processes);
						dest = parent;

						printf("[%d] AVAILABLE and sending SATURATION to %d\n", my_rank, dest);

						MPI_Isend(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);
						node_status = PROCESSING;
					}
					else
					{
						node_status = ACTIVE;
					}
				}
			}

			break;

		// ACTIVE STAGE:
		// - process incoming messages
		// - on the last message received send SATURATION message to source and become PROCESSING
		case ACTIVE:
			MPI_Irecv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
			MPI_Wait(&request, &status);
			source = status.MPI_SOURCE;
			tag = status.MPI_TAG;

			printf("[%d] ACTIVE and receiving SATURATION from %d\n", my_rank, source);

			if(tag == SATURATION) {
				temp_nr_neighbors -= 1;
				neighbors_sum -= source;

				Process_Message(distances, message, source);
				if(temp_nr_neighbors == 1)
				{
					message = Prepare_Message(distances, nr_processes);
					parent = neighbors_sum;
					tag = SATURATION;
					dest = parent;

					printf("[%d] ACTIVE and sending SATURATION to %d\n", my_rank, dest);

					MPI_Isend(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);
					node_status = PROCESSING;
				}
			}
			break;

		// PROCESSING STAGE:
		// - saturated nodes are starting the resolution step where they send the needing information to
		//	 other nodes
		case PROCESSING:
			MPI_Irecv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
			MPI_Wait(&request, &status);
			source = status.MPI_SOURCE;
			tag = status.MPI_TAG;

			if(tag == SATURATION){
				eccentricity = Resolve(distances, nodes, message, my_rank, parent, source, nr_processes, request);
				tag = RESOLUTION;
				dest = parent;
				message = eccentricity;

				printf("[%d] SATURATED from %d and sending RESOLUTION to parent\n", my_rank, source);

				MPI_Isend(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);
				node_status = DONE;
			}
			else if(tag == RESOLUTION){
				eccentricity = Resolve(distances, nodes, message, my_rank, parent, source, nr_processes, request);

				printf("[%d] PROCESSING from %d and sending RESOLUTION to neighbors\n", my_rank, source);

				node_status = DONE;
			}

			break;

		// DONE State
		// - nodes finish their execution
		case DONE:
			printf("r(%d) = %d\n", my_rank, eccentricity);
			fflush(stdout);

			finished = TRUE;
			break;
		}
	}

	// shut down MPI
	MPI_Finalize();
	
	return 0;
}
