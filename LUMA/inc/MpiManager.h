/*
 * --------------------------------------------------------------
 *
 * ------ Lattice Boltzmann @ The University of Manchester ------
 *
 * -------------------------- L-U-M-A ---------------------------
 *
 *  Copyright (C) 2015, 2016
 *  E-mail contact: info@luma.manchester.ac.uk
 *
 * This software is for academic use only and not available for
 * distribution without written consent.
 *
 */

#pragma once

#include "definitions.h"
#include "GridObj.h"


// Define the loop expressions required to inspect the overlap regions of a grid for ease of coding
#define range_i_left	i = 0; i < GridUtils::downToLimit((int)pow(2, g->level + 1), N_lim); i++
#define range_j_down	j = 0; j < GridUtils::downToLimit((int)pow(2, g->level + 1), M_lim); j++
#define range_k_front k = 0; k < GridUtils::downToLimit((int)pow(2, g->level + 1), K_lim); k++
#define range_i_right i = GridUtils::upToZero(N_lim - (int)pow(2, g->level + 1)); i < N_lim; i++
#define range_j_up	j = GridUtils::upToZero(M_lim - (int)pow(2, g->level + 1)); j < M_lim; j++
#define range_k_back	k = GridUtils::upToZero(K_lim - (int)pow(2, g->level + 1)); k < K_lim; k++


/* Manager class to handle all things MPI -- designed as a Singleton */
class MpiManager
{

	// Private constructor / destructor
private :
	MpiManager(void);
	~MpiManager(void);
	static MpiManager* me;	// Pointer to self

public :
	/*
	***************************************************************************************************************
	********************************************* Member Data *****************************************************
	***************************************************************************************************************
	*/

	// MPI world data (all public)
	MPI_Comm world_comm;						// Global MPI communicator
	static const int MPI_cartlab[3][26];		// Cartesian unit vectors pointing to each neighbour in Cartesian topology
	int MPI_dims[L_dims];						// Size of MPI Cartesian topology
	int neighbour_rank[L_MPI_dir];				// Neighbour rank number for each direction in Cartesian topology
	int neighbour_coords[L_dims][L_MPI_dir];	// Coordinates in MPI topology of neighbour ranks
	
	// Communicators for sub-grid / region combinations
#if (L_NumLev > 0)
	MPI_Comm subGrid_comm[L_NumLev*L_NumReg];	
#else
	MPI_Comm subGrid_comm[1];	// Default to size = 1
#endif

	/* Structure storing indices and boolean flags
	* related to the presence of halo regions in
	* MPI PHDF5 writing. Also stores the amount of
	* writable data on the grid. */
	struct phdf5_struct {
	
		int i_start;
		int i_end;
		int j_start;
		int j_end;
		int k_start;
		int k_end;
		int halo_min;
		int halo_max;

		// Identifiers
		int level;
		int region;

		// Data count
		unsigned int writable_data_count = 0;
	};
	std::vector<phdf5_struct> p_data;			// Structure containing halo descriptors for block writing (HDF5)
	
	// Static Data (commonly used and grid-independent)
	static int my_rank;				// Rank number
	static int num_ranks;			// Total number of ranks in MPI Cartesian topology
	static int MPI_coords[L_dims];	// Coordinates in MPI Cartesian topolgy


	// Grid data
	int global_dims[3];				// Dimensions of problem coarse lattice
	std::vector<int> local_size;	// Dimensions of coarse lattice represented on this rank (includes inner and outer overlapping layers)
	// Global indices of cooarse lattice nodes represented on this rank (excluding outer overlapping layer)
	std::vector< std::vector<int> > global_edge_ind;	// Rows are x,y,z start and end pairs and columns are rank number
	// Global positions of coarse lattice nodes represented on this rank (excluding outer overlapping layer)
	std::vector< std::vector<double> > global_edge_pos;			// Rows are x,y,z start and end pairs and columns are rank number
	// Structures containing sender and receiver layer edges as global physical position
	struct layer_edges {
		double X[4];
		double Y[4];
		double Z[4];
	} sender_layer_pos, recv_layer_pos;		// [left_min left_max right_min right_max] for X,Y and Z.
	static GridObj* Grids;					// Pointer to grid hierarchy
	

	// Buffer data
	std::vector< std::vector<double>> f_buffer_send;	// Array of resizeable outgoing buffers used for data transfer
	std::vector< std::vector<double>> f_buffer_recv;	// Array of resizeable incoming buffers used for data transfer
	MPI_Status recv_stat;					// Status structure for Receive return information
	MPI_Request send_requests[L_MPI_dir];		// Array of request structures for handles to posted ISends
	MPI_Status send_stat[L_MPI_dir];			// Array of statuses for each Isend
	// Structure storing the buffer sizes in each direction for a particular level and region
	struct buffer_struct {
		int size[L_MPI_dir];
		int level;
		int region;
	};
	std::vector<buffer_struct> buffer_send_info, buffer_recv_info;	// Array of buffer_info structures holding buffer size information

	// Logfile
	static std::ofstream* logout;



	/*
	***************************************************************************************************************
	********************************************* Member Methods **************************************************
	***************************************************************************************************************
	*/

	// Singleton design
	static MpiManager* getInstance();	// Get the pointer to the singleton instance (create it if necessary)
	static void destroyInstance();

	// Initialisation
	void mpi_init();		// Initialisation of MpiManager & Cartesian topology
	void mpi_gridbuild( );	// Do domain decomposition to build local grid dimensions
	int mpi_buildCommunicators();	// Create a new communicator for each sub-grid and region combo

	// Buffer methods
	void mpi_buffer_pack( int dir, GridObj* g );		// Pack the buffer ready for data transfer on the supplied grid in specified direction
	void mpi_buffer_unpack( int dir, GridObj* g );		// Unpack the buffer back to the grid given
	void mpi_buffer_size();								// Set buffer size information for grids in hierarchy given and 
														// set pointer to hierarchy for subsequent access
	void mpi_buffer_size_send( GridObj*& g );			// Routine to find the size of the sending buffer on supplied grid
	void mpi_buffer_size_recv( GridObj*& g );			// Routine to find the size of the receiving buffer on supplied grid

	// IO
	void mpi_writeout_buf( std::string filename, int dir );		// Write out the buffers of direction dir to file

	// Comms
	void mpi_communicate( int level, int regnum );		// Wrapper routine for communication between grids of given level/region
	int mpi_getOpposite(int direction);					// Version of GridUtils::getOpposite for MPI_directions rather than lattice directions
};
