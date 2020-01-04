/*
 ============================================================================
 Name        : Finding_eccentricities.c
 Author      : Alexandru Grigoras
 Version     : 02
 Copyright   : Copyright Alexandru Grigoras
 Description : Finding the eccentricities in a tree
 References	 : The algorithm is implemented from
 	 	 	   [N. Santoro, Design and Analysis of Distributed Algorithms,
 	 	 	    Ottawa: WILEY-INTERSCIENCE, 2006]
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define ROOT 0
#define NR_NODES 6

#define TRUE 1
#define FALSE 0

enum STATUS_ENUM {
	AVAILABLE = 0,
	ACTIVE = 1,
	PROCESSING = 2,
	DONE = 3
};

enum MESSAGE_TYPE {
	ACTIVATE = 0,
	SATURATION = 1,
	RESOLUTION = 2
};

void Initialize(int *dist, int np)
{
	for(int i=0; i<np; i++)
	{
		dist[i] = 0;
	}
}

int Prepare_Message(int *dist, int np)
{
	int maxdist = dist[0];
	for(int i=1; i<np; i++)
	{
		if(dist[i] > maxdist)
		{
			maxdist = dist[i];
		}
	}

	return (maxdist + 1);
}

void Process_Message(int *dist, int received_distance, int sender)
{
	dist[sender] = received_distance;
}

int Calculate_Eccentricities(int *dist, int np)
{
	int maxdist = dist[0];
	for(int i=1; i<np; i++)
	{
		if(dist[i] > maxdist)
		{
			maxdist = dist[i];
		}
	}

	return maxdist;
}

int Resolve(int *dist, int nodes[][NR_NODES], int received_distance, int my_rank, int parent, int sender, int np)
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
				if((dist[i] > maxdist))
				{
					maxdist = dist[i];
				}
			}

			tag = RESOLUTION;
			message = maxdist + 1;
			MPI_Send(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
			//printf("[%d] I am sending RESOLUTION to %d\n", my_rank, dest);
		}
	}

	return eccentricity;
}

void Print_vector(int *v, int nr){
	printf("V = [ ");
	for(int i=0; i<nr; i++)
	{
		printf("%d ", v[i]);
	}
	printf("]\n");
}

int main(int argc, char* argv[]){
	int my_rank; 					// rank of process
	int nr_processes;				// number of processes
	int source;   					// rank of sender
	int dest;    					// rank of receiver
	int tag=0;    					// tag for messages
	int message;					// storage for message
	MPI_Status status;				// return status for receive
	
	int i;							// index
	int node_status = AVAILABLE;	// status of the node; initially is set to AVAILABLE
	int parent = -1;				// parent of the node; initially the parent is unknown
	int nr_neighbors = 0;			// the number of neighbors
	int temp_nr_neighbors = 0;

	int nodes[NR_NODES][NR_NODES] = {
			{0, 1, 0, 0, 0, 0},
			{1, 0, 1, 1, 1, 0},
			{0, 1, 0, 0, 0, 0},
			{0, 1, 0, 0, 0, 1},
			{0, 1, 0, 0, 0, 0},
			{0, 0, 0, 1, 0, 0}
	};
	int distances[NR_NODES];
	int finished = FALSE;
	int eccentricity = -1;

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
		}
	}
	temp_nr_neighbors = nr_neighbors;

	while(!finished)
	{
		switch(node_status)
		{
		// ACTIVATION stage
		// the root is sending activation messages to it's neighbors to activate and the nodes to their neighbors
		case AVAILABLE:
			if(my_rank == ROOT)
			{
				//printf("[%d] I am AVAILABLE and sending ACTIVATION\n", my_rank);

				// sending activation message
				for(dest=0; dest<nr_processes; dest++)
				{
					if(nodes[my_rank][dest])
					{
						tag = ACTIVATE;
						message = 0;
						MPI_Send(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
					}
				}

				// initialize and send SATURATION to parent if process is leaf
				Initialize(distances, nr_processes);
				for(dest=0; dest<nr_processes; dest++)
				{
					if(nodes[my_rank][dest])
					{
						if(parent == -1)
						{
							parent = dest;
						}
					}
				}
				message = Prepare_Message(distances, nr_processes);
				tag = SATURATION;
				dest = parent;
				MPI_Send(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
				node_status = PROCESSING;
			}
			else
			{
				// receiving activation message
				MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				source = status.MPI_SOURCE;
				tag = status.MPI_TAG;
				if(node_status == AVAILABLE && tag == ACTIVATE)
				{
					//printf("[%d] I am AVAILABLE and receiving ACTIVATION\n", my_rank);

					// sending activation message to neighbors
					for(dest=0; dest<nr_processes; dest++)
					{
						if(nodes[my_rank][dest] && dest != source)
						{
							tag = ACTIVATE;
							message = 0;
							MPI_Send(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
						}
					}

					// initialize and send SATURATION to parent if process is leaf
					Initialize(distances, nr_processes);
					if(nr_neighbors == 1)
					{
						if(parent == -1)
						{
							parent = source;
						}
						tag = SATURATION;
						message = Prepare_Message(distances, nr_processes);
						dest = parent;
						MPI_Send(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
						node_status = PROCESSING;
					}
					else
					{
						node_status = ACTIVE;
					}
				}
			}
			break;
		case ACTIVE:
			MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			source = status.MPI_SOURCE;
			tag = status.MPI_TAG;
			temp_nr_neighbors -= 1;
			//printf("[%d] I am ACTIVE from %d\n", my_rank, source);
			Process_Message(distances, message, source);
			if(temp_nr_neighbors == 1)
			{
				message = Prepare_Message(distances, nr_processes);
				parent = source;
				tag = SATURATION;
				dest = parent;
				MPI_Send(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
				node_status = PROCESSING;
			}
			break;
		case PROCESSING:
			MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			source = status.MPI_SOURCE;
			tag = status.MPI_TAG;

			//printf("[%d] I am PROCESSING and my parent is %d\n", my_rank, parent);

			if(tag == SATURATION){
				eccentricity = Resolve(distances, nodes, message, my_rank, parent, source, nr_processes);
				tag = RESOLUTION;
				dest = parent;
				message = eccentricity;
				MPI_Send(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
				//printf("[%d] I am SATURATED from %d with eccentricity %d\n", my_rank, source, eccentricity);
				node_status = DONE;
			}
			else if(tag == RESOLUTION){
				eccentricity = Resolve(distances, nodes, message, my_rank, parent, source, nr_processes);
				//printf("[%d] I am RESOLUTION from %d with eccentricity %d\n" my_rank, source, eccentricity);
				node_status = DONE;
			}
			break;
		case DONE:
			finished = TRUE;
			break;
		}
	}

	printf("[%d] I am FINISHED with eccentricity %d\n", my_rank, eccentricity);

	// shut down MPI
	MPI_Finalize();
	
	return 0;
}
