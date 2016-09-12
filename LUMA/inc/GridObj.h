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

/** GridObj class which represents a lattice */

#include <vector>
#include "IVector.h"
#include "IBBody.h"
#include <iostream>
#include <fstream>
#include "hdf5luma.h"

// Enumeration for Lattice Typing
enum eType
{
	eSolid,
	eFluid,
	eRefined,
	eTransitionToCoarser,
	eTransitionToFiner,
	eBFL,
	eSymmetry,
	eInlet,
	eOutlet,
	eRefinedSolid,
	eRefinedSymmetry,
	eRefinedInlet
};

// Enumeration for BC application type
enum eBCType
{
	eBCAll,
	eBCSolidSymmetry,
	eBCInlet,
	eBCOutlet,
	eBCInletOutlet,
	eBCBFL
};

// Base class
class GridObj
{

	// Allow MpiManager and ObjectManager objects to access private Grid information as required
	friend class MpiManager;
	friend class ObjectManager;
	friend class GridUtils;

public:

	// Constructors & Destructor //
	GridObj( ); // Default constructor
	GridObj(int level); // Basic grid constructor
	GridObj(int RegionNumber, GridObj& pGrid); // Sub grid constructor with region and reference to parent grid for initialisation
	// MPI L0 constructor with local size and global edges
	GridObj(int level, std::vector<int> local_size, 
		std::vector< std::vector<int> > GlobalLimsInd, 
		std::vector< std::vector<double> > GlobalLimsPos);
	~GridObj( ); // Default destructor


	/*
	***************************************************************************************************************
	********************************************* Member Data *****************************************************
	***************************************************************************************************************
	*/

private :

	// 1D subgrid array (size = L_NumReg)
	std::vector<GridObj> subGrid;

	// Start and end indices of corresponding coarse level
	// When using MPI these values are local to a particular coarse grid
	int CoarseLimsX[2];
	int CoarseLimsY[2];
	int CoarseLimsZ[2];

	// 1D arrays
public :
	std::vector<int> XInd; // Vectors of indices
	std::vector<int> YInd;
	std::vector<int> ZInd;
	std::vector<double> XPos; // Vectors of positions of sites
	std::vector<double> YPos;
	std::vector<double> ZPos;

private :
	// Inlet velocity profile
	std::vector<double> ux_in, uy_in, uz_in;

	// Vector nodal properties
	// Flattened 4D arrays (i,j,k,vel)
	IVector<double> f;
	IVector<double> feq;
	IVector<double> u;
	IVector<double> force_xyz;
	IVector<double> force_i;

	// Scalar nodal properties
	// Flattened 3D arrays (i,j,k)
	IVector<double> rho;

	// Grid scalars
	double dx, dy, dz;		// Physical spacing
	int region_number;		// ID of region at a particular level in the embedded grid hierarchy

	// Time averaged statistics
	IVector<double> rho_timeav;		// Time-averaged density at each grid point (i,j,k)
	IVector<double> ui_timeav;		// Time-averaged velocity at each grid point (i,j,k,L_dims)
	IVector<double> uiuj_timeav;	// Time-averaged velocity products at each grid point (i,j,k,3*L_dims-3)


	// Public data members
public :

	IVector<eType> LatTyp;			// Flattened 3D array of site labels
	int level;						// Level in embedded grid hierarchy
	double dt;						// Physical time step size
	int t;							// Number of completed iterations
	double nu;						// Kinematic viscosity (in lattice units)
	double omega;					// Relaxation frequency

	// Timing variables
	double timeav_mpi_overhead;		// Time of MPI communication
	double timeav_timestep;			// Time of a timestep

	// Local grid sizes
	int N_lim;
	int M_lim;
	int K_lim;


	/*
	***************************************************************************************************************
	********************************************* Member Methods **************************************************
	***************************************************************************************************************
	*/

public :

	// Initialisation functions
	void LBM_initVelocity();		// Initialise the velocity field
	void LBM_initRho();				// Initialise the density field
	void LBM_initGrid();			// Non-MPI wrapper for initialiser
	void LBM_initGrid(std::vector<int> local_size,
		std::vector< std::vector<int> > GlobalLimsInd,
		std::vector< std::vector<double> > GlobalLimsPos);		// Initialise top level grid with fields and labels
	void LBM_initSubGrid(GridObj& pGrid);		// Initialise subgrid with all quantities
	void LBM_initBoundLab();					// Initialise labels for walls
	void LBM_initSolidLab();					// Initialise labels for solid objects
	void LBM_initRefinedLab(GridObj& pGrid);	// Initialise labels for refined regions
	void LBM_init_getInletProfile();			// Initialise the store for inlet profile data from file

	// LBM operations
	void LBM_multi(bool IBM_flag);		// Launch the multi-grid kernel
	void LBM_collide();					// Apply collision + 1 overload for equilibrium calculation
	double LBM_collide(int i, int j, int k, int v);
	void LBM_kbcCollide(int i, int j, int k, IVector<double>& f_new);		// KBC collision operator
	void LBM_stream();							// Stream populations
	void LBM_macro();							// Compute macroscopic quantities + 1 overload for single site
	void LBM_macro(int i, int j, int k);
	void LBM_boundary(int bc_type_flag);		// Apply boundary conditions
	void LBM_forcegrid(bool reset_flag);		// Apply a force to the grid points (or simply reset force vectors if flag is true)

	// Boundary operations
	void bc_applyBounceBack(int label, int i, int j, int k);	// Application of HWBB BC
	void bc_applySpecReflect(int label, int i, int j, int k);	// Application of HWSR BC
	void bc_applyRegularised(int label, int i, int j, int k);	// Application of Regaulrised BC
	void bc_applyExtrapolation(int label, int i, int j, int k);			// Application of Extrapolation BC
	void bc_applyBfl(int i, int j, int k);														// Application of BFL BC
	void bc_applyNrbc(int i, int j, int k);														// Application of characteristic NRBC
	void bc_solidSiteReset();																	// Reset all the solid site velocities to zero
	double bc_getWallDensityForRBC(std::vector<double>& ftmp, int normal,
		int i, int j, int k);		// Gets wall density for generalised, regularised velocity BC

	// Multi-grid operations
	void LBM_explode(int RegionNumber);			// Explode populations from coarse to fine
	void LBM_coalesce(int RegionNumber);		// Coalesce populations from fine to coarse
	void LBM_addSubGrid(int RegionNumber);		// Add and initialise subgrid structure for a given region number

	// IO methods
	void io_textout(std::string output_tag);	// Writes out the contents of the class as well as any subgrids to a text file
	void io_restart(bool IO_flag);				// Reads/writes data from/to the global restart file
	void io_probeOutput();						// Output routine for point probes
	void io_lite(double tval, std::string Tag);	// Generic writer to individual files with Tag
	int io_hdf5(double tval);					// HDF5 writer returning integer to indicate success or failure

};
