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

#include "../inc/stdafx.h"

// Static declarations
GridManager* GridManager::me;

/// \brief	Default constructor
///
///			Uses compile-time parameters to populate global grid information.
///			Also executes the auto-sub-grid generation if requested.
GridManager::GridManager()
{

	// Store the discrete interpretation of the grid information //

	// Store global sizes and edges for L0 from definitions
	global_size[eXDirection][0] = static_cast<int>(L_N);
	global_edges[eXMin][0] = 0.0;
	global_edges[eXMax][0] = L_BX;

	global_size[eYDirection][0] = static_cast<int>(L_M);
	global_edges[eYMin][0] = 0.0;
	global_edges[eYMax][0] = L_BY;

	global_size[eZDirection][0] = static_cast<int>(L_K);
	global_edges[eZMin][0] = 0.0;
	global_edges[eZMax][0] = L_BZ;

	/* Store global sizes for all sub-grids:
	* This is an initialisation before any grids are built so cannot use any
	* of the utility functions which require an initialised grid. Instead we
	* must perform the correct rounding and sizing explicitly. Logic is to round
	* loose boundaries up and down depending on their position relative to the
	* discrete voxel centre of the previous grid built. */

	// Get base dh
	double dh = static_cast<double>(L_BX) / static_cast<double>(L_N);
	int idx;
	
	// Loop over every sub-grid
	for (int lev = 1; lev <= L_NUM_LEVELS; ++lev)
	{
		for (int reg = 0; reg < L_NUM_REGIONS; ++reg)
		{

			idx = lev + reg * L_NUM_LEVELS;

			/* If using auto sub-grid generation, the padding from the definitions 
			 * file will be used to define the grid edges. */
#ifdef L_AUTO_SUBGRIDS
			if (lev > 1)
			{
				// Use padding parameters to specify relative to TL of parent
				global_edges[eXMin][idx] = global_edges[eXMin][idx - 1] + (2.0 * dh) + std::round(L_PADDING_X_MIN / dh) * dh;
				global_edges[eXMax][idx] = global_edges[eXMax][idx - 1] - (2.0 * dh) - std::round(L_PADDING_X_MAX / dh) * dh;
				global_edges[eYMin][idx] = global_edges[eYMin][idx - 1] + (2.0 * dh) + std::round(L_PADDING_Y_MIN / dh) * dh;
				global_edges[eYMax][idx] = global_edges[eYMax][idx - 1] - (2.0 * dh) - std::round(L_PADDING_Y_MAX / dh) * dh;
				global_edges[eZMin][idx] = global_edges[eZMin][idx - 1] + (2.0 * dh) + std::round(L_PADDING_Z_MIN / dh) * dh;
				global_edges[eZMax][idx] = global_edges[eZMax][idx - 1] - (2.0 * dh) - std::round(L_PADDING_Z_MAX / dh) * dh;

			}
			else
				// Level 1 must be placed based on the user-defined value //
#endif
			{
				// Round definitions file values to voxel resolution on parent level
				global_edges[eXMin][idx] = std::round(cRefStartX[lev - 1][reg] / dh) * dh;
				global_edges[eXMax][idx] = std::round(cRefEndX[lev - 1][reg] / dh) * dh;
				global_edges[eYMin][idx] = std::round(cRefStartY[lev - 1][reg] / dh) * dh;
				global_edges[eYMax][idx] = std::round(cRefEndY[lev - 1][reg] / dh) * dh;
				global_edges[eZMin][idx] = std::round(cRefStartZ[lev - 1][reg] / dh) * dh;
				global_edges[eZMax][idx] = std::round(cRefEndZ[lev - 1][reg] / dh) * dh;

			}

			// Print warnings for grids sizes that extend beyond parent grid
			if (lev > 1)
			{
				if (global_edges[eXMin][idx] <= global_edges[eXMin][idx - 1])
					L_WARN("Level " + std::to_string(lev) + " Region " + std::to_string(reg) + " X grid start is coincident with or outside its parent grid!", GridUtils::logfile);
				if (global_edges[eXMax][idx] >= global_edges[eXMax][idx - 1])
					L_WARN("Level " + std::to_string(lev) + " Region " + std::to_string(reg) + " X grid end is coincident with or outside its parent grid!", GridUtils::logfile);
				if (global_edges[eYMin][idx] <= global_edges[eYMin][idx - 1])
					L_WARN("Level " + std::to_string(lev) + " Region " + std::to_string(reg) + " Y grid start is coincident with or outside its parent grid!", GridUtils::logfile);
				if (global_edges[eYMax][idx] >= global_edges[eYMax][idx - 1])
					L_WARN("Level " + std::to_string(lev) + " Region " + std::to_string(reg) + " Y grid end is coincident with or outside its parent grid!", GridUtils::logfile);
				if (global_edges[eZMin][idx] <= global_edges[eZMin][idx - 1])
					L_WARN("Level " + std::to_string(lev) + " Region " + std::to_string(reg) + " Z grid start is coincident with or outside its parent grid!", GridUtils::logfile);
				if (global_edges[eZMax][idx] >= global_edges[eZMax][idx - 1])
					L_WARN("Level " + std::to_string(lev) + " Region " + std::to_string(reg) + " Z grid end is coincident with or outside its parent grid!", GridUtils::logfile);
			}


			// Populate sizes
			global_size[eXDirection][idx] = static_cast<int>(std::round(2.0 * (global_edges[eXMax][idx] - global_edges[eXMin][idx]) / dh));
			global_size[eYDirection][idx] = static_cast<int>(std::round(2.0 * (global_edges[eYMax][idx] - global_edges[eYMin][idx]) / dh));
#if (L_DIMS == 3)
			global_size[eZDirection][idx] = static_cast<int>(std::round(2.0 * (global_edges[eZMax][idx] - global_edges[eZMin][idx]) / dh));
#else
			global_size[eZDirection][idx] = 1;
#endif

			// Error if grid sizes are negative or zero
			if (global_size[eXDirection][idx] <= 0) L_ERROR("Level " + std::to_string(lev) + " Region " + std::to_string(reg) + " sub-grid has size < 0 in X-direction -- not enough base resolution to support this level!", GridUtils::logfile);
			if (global_size[eYDirection][idx] <= 0) L_ERROR("Level " + std::to_string(lev) + " Region " + std::to_string(reg) + " sub-grid has size < 0 in Y-direction -- not enough base resolution to support this level!", GridUtils::logfile);
			if (global_size[eZDirection][idx] <= 0) L_ERROR("Level " + std::to_string(lev) + " Region " + std::to_string(reg) + " sub-grid has size < 0 in Z-direction -- not enough base resolution to support this level!", GridUtils::logfile);

			/* If the start and end edges are the same, assume the user is trying to 
			 * make the sub-grid periodic so size of sub-grid is just double the 
			 * parent grid size in that direction. */
			if (global_edges[eXMin][idx] == global_edges[eXMax][idx]) global_size[eXDirection][idx] = global_size[eXDirection][idx - 1] * 2;
			if (global_edges[eYMin][idx] == global_edges[eYMax][idx]) global_size[eYDirection][idx] = global_size[eYDirection][idx - 1] * 2;
			if (global_edges[eZMin][idx] == global_edges[eZMax][idx]) global_size[eZDirection][idx] = global_size[eZDirection][idx - 1] * 2;


		}

		// Get resolution for next
		dh /= 2.0;
	}
	
	// Print out grid edges to file
	std::string msg("Global Grid Edges computed and stored as:\n");

	for (int lev = 0; lev <= L_NUM_LEVELS; ++lev)
	{
		for (int reg = 0; reg < L_NUM_REGIONS; ++reg)
		{

			// Skip invalid combos
			if (lev == 0 && reg != 0) continue;

			idx = lev + reg * L_NUM_LEVELS;
			msg += "L" + std::to_string(lev) + " R" + std::to_string(reg) + "\t"
				+ std::to_string(global_edges[eXMin][idx]) + " -- " + std::to_string(global_edges[eXMax][idx]) + "\t"
				+ std::to_string(global_edges[eYMin][idx]) + " -- " + std::to_string(global_edges[eYMax][idx]) + "\t"
				+ std::to_string(global_edges[eZMin][idx]) + " -- " + std::to_string(global_edges[eZMax][idx]) + "\t";
			msg += "\n";

		}

	}
	L_INFO(msg, GridUtils::logfile);


	// Set some local information -- same as problem dimensions for now.
	// When using MPI, these values will be updated at initialisation.
	local_size.push_back(static_cast<int>(L_N));
	local_size.push_back(static_cast<int>(L_M));
	local_size.push_back(static_cast<int>(L_K));

	
}

/// Default destructor
GridManager::~GridManager()
{
}

/// Instance creator
GridManager* GridManager::getInstance()
{
	if (!me) me = new GridManager();	// Private construction
	return me;							// Return pointer to new object
}

/// Instance destroyer
void GridManager::destroyInstance()
{
	delete me;			// Delete pointer from static context (destructor will be called automatically)
}

/// Set grid hierarchy in the manager after grid creation
void GridManager::setGridHierarchy(GridObj *const grids)
{
	Grids = grids;
}


/// \brief	Method to set the local coarse size.
///
///			Accessible to the MPI manager so can be set remotely in parallel builds.
///
///	\param	size_vector	vector of sizes to assign.
void GridManager::setLocalCoarseSize(const std::vector<int>& size_vector)
{
	this->local_size.clear();
	this->local_size = size_vector;
}

/// \brief	Method to create a blank store which holds the writable region 
///			information for a given grid.
///
///	\param[out]	datastruct	reference to pointer to newly created data.
void GridManager::createWritableDataStore(HDFstruct *& datastruct)
{
	p_data.emplace_back();
	datastruct = &(p_data.back());
	return;
}

/// \brief	Method to create and populate a writable data info store.
///
///	\param	targetGrid	constant pointer to constant grid to be used to populate information.
///	\returns			boolean indicator as to whether writable data on this grid.
bool GridManager::createWritableDataStore(GridObj const * const targetGrid)
{

	if (targetGrid->level == 0)
	{

		// Start by adding the L0 details
		HDFstruct *p_data = nullptr;
		createWritableDataStore(p_data);
		p_data->level = 0;
		p_data->region = 0;

		// Get local L0 grid sizes
		int N_lim = static_cast<int>(targetGrid->N_lim);
		int M_lim = static_cast<int>(targetGrid->M_lim);
		int K_lim = static_cast<int>(targetGrid->K_lim);

		p_data->k_start = 0;
		p_data->k_end = 0;

#ifdef L_BUILD_FOR_MPI
		// Halo exists on all edges on L0 and there are no transition layers
		p_data->i_end = N_lim - 2;
		p_data->i_start = 1;
		p_data->j_end = M_lim - 2;
		p_data->j_start = 1;
#if (L_DIMS == 3)
		p_data->k_end = K_lim - 2;
		p_data->k_start = 1;
#endif

#else
		// In serial, no halos or TL
		p_data->i_end = N_lim - 1;
		p_data->i_start = 0;
		p_data->j_end = M_lim - 1;
		p_data->j_start = 0;
#if (L_DIMS == 3)
		p_data->k_end = K_lim - 1;
		p_data->k_start = 0;
#endif

#endif	// L_BUILD_FOR_MPI

		// Writable data count from indices
		p_data->writable_data_count =
			(p_data->i_end - p_data->i_start + 1) *
			(p_data->j_end - p_data->j_start + 1) *
			(p_data->k_end - p_data->k_start + 1);

	}

	// Generating information for sub-grids is harder!
	else
	{

		/* Since we are doing this only for HDF5 we can exclude those ranks
		* which do not contain sub-grid regions that will need to be
		* written out. If we didn't, at write-time the buffer size would be
		* zero and the HDF5 write would fail. So now we check for writable
		* data by excluding halo and TL sites in a 2-step process. The first
		* step is to determine whether a halo exists by querying the buffer
		* size information then compute the offset. The second is the shift
		* the indices a second time if they are pointing to a TL. */

		// Get local grid sizes (halo included)
		int N_lim = static_cast<int>(targetGrid->N_lim);
		int M_lim = static_cast<int>(targetGrid->M_lim);
		int K_lim = static_cast<int>(targetGrid->K_lim);
		int lev = targetGrid->level;
		int reg = targetGrid->region_number;

		// Add a new writable data store in the grid manager
		HDFstruct *p_data = nullptr;
		createWritableDataStore(p_data);
		p_data->level = targetGrid->level;
		p_data->region = targetGrid->region_number;


		// Serial Case
#if !defined L_BUILD_FOR_MPI

		// Set writable region start and end indices for the serial case (TL always 2 cells thick)
		p_data->i_start = subgrid_tlayer_key[eXMin][(lev - 1) + reg * L_NUM_LEVELS] * 2;
		p_data->i_end = N_lim - 1 - subgrid_tlayer_key[eXMax][(lev - 1) + reg * L_NUM_LEVELS] * 2;
		p_data->j_start = subgrid_tlayer_key[eYMin][(lev - 1) + reg * L_NUM_LEVELS] * 2;
		p_data->j_end = M_lim - 1 - subgrid_tlayer_key[eYMax][(lev - 1) + reg * L_NUM_LEVELS] * 2;
#if (L_DIMS == 3)
		p_data->k_start = subgrid_tlayer_key[eZMin][(lev - 1) + reg * L_NUM_LEVELS] * 2;
		p_data->k_end = K_lim - 1 - subgrid_tlayer_key[eZMax][(lev - 1) + reg * L_NUM_LEVELS] * 2;
#else
		p_data->k_start = 0;
		p_data->k_end = 0;
#endif

		p_data->writable_data_count =
			(p_data->i_end - p_data->i_start + 1) *
			(p_data->j_end - p_data->j_start + 1) *
			(p_data->k_end - p_data->k_start + 1);

		return true;


		// Parallel Case
#else
		/* First we define the writable region limits in local indices
		* taking into account the elements that sit in the halo
		* (recv layer) which will not be written out.
		* We can do this simply by looking up the position of the last and
		* first sites on the grid and checking to see whether they need
		* moving off the halo (recv layer).
		* We do this by starting at the grid edges shift the index if that
		* site is on a receiver layer. Once off the receiver layer (i.e.
		* out of the halo) we have computed the offset.
		* This fixes a previous issue where it was assumed that the shift
		* always be a full halo shift which is not true if a TL is in the
		* halo and also fixes an issue that could have arisen if only a
		* corner of a grid was built exclusively on the halo. */

		// Declarations
		int shifted_index;

		// Set writable region start and end indices
		p_data->i_start = 0;
		p_data->i_end = 0;
		p_data->j_start = 0;
		p_data->j_end = 0;
		p_data->k_start = 0;
		p_data->k_end = 0;


		// Check upper x-direction edge for presence of halo
		shifted_index = N_lim;
		do --shifted_index;
		while (shifted_index >= 0 && GridUtils::isOnRecvLayer(targetGrid->XPos[shifted_index], eXMax));
		p_data->i_end = shifted_index;	// Index has now shifted off halo

#if defined L_HDF_DEBUG
		if (shifted_index != N_lim - 1)
			*GridUtils::logfile << "Upper X halo found. Thickness = " << (N_lim - 1) - shifted_index << std::endl;
#endif

		// Check lower x-direction edge for presence of halo
		shifted_index = -1;
		do ++shifted_index;
		while (shifted_index < N_lim && GridUtils::isOnRecvLayer(targetGrid->XPos[shifted_index], eXMin));
		p_data->i_start = shifted_index;

#if defined L_HDF_DEBUG
		if (shifted_index != 0)
			*GridUtils::logfile << "Lower X halo found. Thickness = " << shifted_index << std::endl;
#endif

		// Check y-directions
		shifted_index = M_lim;
		do --shifted_index;
		while (shifted_index >= 0 && GridUtils::isOnRecvLayer(targetGrid->YPos[shifted_index], eYMax));
		p_data->j_end = shifted_index;

#if defined L_HDF_DEBUG
		if (shifted_index != M_lim - 1)
			*GridUtils::logfile << "Upper Y halo found. Thickness = " << (M_lim - 1) - shifted_index << std::endl;
#endif

		shifted_index = -1;
		do ++shifted_index;
		while (shifted_index < M_lim && GridUtils::isOnRecvLayer(targetGrid->YPos[shifted_index], eYMin));
		p_data->j_start = shifted_index;

#if defined L_HDF_DEBUG
		if (shifted_index != 0)
			*GridUtils::logfile << "Lower Y halo found. Thickness = " << shifted_index << std::endl;
#endif

#if (L_DIMS == 3)
		// Check z-directions for halo
		shifted_index = K_lim;
		do --shifted_index;
		while (shifted_index >= 0 && GridUtils::isOnRecvLayer(targetGrid->ZPos[shifted_index], eZMax));
		p_data->k_end = shifted_index;

#if defined L_HDF_DEBUG
		if (shifted_index != K_lim - 1)
			*GridUtils::logfile << "Upper Z halo found. Thickness = " << (K_lim - 1) - shifted_index << std::endl;
#endif

		shifted_index = -1;
		do ++shifted_index;
		while (shifted_index < K_lim && GridUtils::isOnRecvLayer(targetGrid->ZPos[shifted_index], eZMin));
		p_data->k_start = shifted_index;

#if defined L_HDF_DEBUG
		if (shifted_index != 0)
			*GridUtils::logfile << "Lower Z halo found. Thickness = " << shifted_index << std::endl;
#endif

#else
		p_data->k_start = 0;
		p_data->k_end = 0;
#endif

#if defined L_HDF_DEBUG
		*GridUtils::logfile << "After halo shifting, local writable indices are: " << std::endl <<
			"i = " << p_data->i_start << " - " << p_data->i_end << std::endl <<
			"j = " << p_data->j_start << " - " << p_data->j_end << std::endl <<
			"k = " << p_data->k_start << " - " << p_data->k_end << std::endl;
#endif


		/* Now account for transition layers -- If the current sites
		* defining the edges of the writable region have a position
		* within the TL (i.e. within 2 * dh of the grid edge)
		* then we shift them off the TL. It is possible that
		* this causes indices to be negative or writable regions to
		* have a size < 0 which indicates that the rank has no writable
		* data. TL is only ever 2 sites thick so we can do 2 passes to
		* make sure all the necessary shifting is complete. */
		for (int i = 0; i < 2; ++i)
		{
			// If on TL, then shift writable region index by one
			if (GridUtils::isOnTransitionLayer(targetGrid->XPos[p_data->i_start], eXMin, targetGrid))
				p_data->i_start++;
			if (GridUtils::isOnTransitionLayer(targetGrid->XPos[p_data->i_end], eXMax, targetGrid))
				p_data->i_end--;
			if (GridUtils::isOnTransitionLayer(targetGrid->YPos[p_data->j_start], eYMin, targetGrid))
				p_data->j_start++;
			if (GridUtils::isOnTransitionLayer(targetGrid->YPos[p_data->j_end], eYMax, targetGrid))
				p_data->j_end--;
#if (L_DIMS == 3)
			if (GridUtils::isOnTransitionLayer(targetGrid->ZPos[p_data->k_start], eZMin, targetGrid))
				p_data->k_start++;

			if (GridUtils::isOnTransitionLayer(targetGrid->ZPos[p_data->k_end], eZMax, targetGrid))
				p_data->k_end--;
#endif

}


#if defined L_HDF_DEBUG
		*GridUtils::logfile << "After TL shifting, local writable indices are: " << std::endl <<
			"i = " << p_data->i_start << " - " << p_data->i_end << std::endl <<
			"j = " << p_data->j_start << " - " << p_data->j_end << std::endl <<
			"k = " << p_data->k_start << " - " << p_data->k_end << std::endl;
#endif

		// Test for writable data
		if (
			(p_data->i_end - p_data->i_start + 1) > 0 &&
			(p_data->j_end - p_data->j_start + 1) > 0 &&
			(p_data->k_end - p_data->k_start + 1) > 0
			) {

			// Writable data found to include in communicator
			p_data->writable_data_count =
				(p_data->i_end - p_data->i_start + 1) *
				(p_data->j_end - p_data->j_start + 1) *
				(p_data->k_end - p_data->k_start + 1);
			return true;
		}
		else {

			// No writable data on the grid so exclude from communicator
			p_data->writable_data_count = 0;
			return false;
		}

#endif	// !L_BUILD_FOR_MPI

	}

	return true;
}

/// \brief	Returns the number of active cells in the union between the domain 
///			and the bounds.
///
///			This method wraps the getCellCount() method. Grid hierarchy is 
///			traversed with active cells from each grid being added to the total 
///			in turn. Does not taken into account the halo cells at present 
///			although could be modified to do so in the future.
///
///	\param	bounds	pointer to an array containing the bounds of the region to be checked.
///	\returns		number of active cells in the union.
long GridManager::getActiveCellCount(double *bounds)
{

	// Get count on course grid
	unsigned int idx = 0;
	long cell_count = 0;
	long cells_on_this_grid = 0;
	cell_count = getCellCount(0, 0, bounds);

	// Loop over the grids and retrieve cell counts
	for (int lev = 0; lev < L_NUM_LEVELS + 1; ++lev)
	{
		for (int reg = 0; reg < L_NUM_REGIONS; ++reg)
		{
			// Retrieve cell count in union
			cells_on_this_grid = getCellCount(lev, reg, bounds);

			// Add the contribution of active cells from this grid
			cell_count += cells_on_this_grid;
				
			// Deduct course cells that are refined
			cell_count -= cells_on_this_grid / static_cast<long>(pow(2, L_DIMS));

		}
	}

	return cell_count;
}

/// \brief	Returns the number of cells in the union between the target 
///			grid and the bounds.
///
///			This method does not taken into account the halo cells at present
///			although could be modified to do so in the future. It also does not 
///			snap bounds edges to cell edges hence the value returned is only an
///			approximation of cell count rather than the actual cell count.
///
///	\param	targetLevel		grid level on which to check for union.
///	\param	targetRegion	grid region on which to check for union.
///	\param	bounds			pointer to an array containing the bounds of the region to be checked.
///	\returns				number of cells in the union.
long GridManager::getCellCount(int targetLevel, int targetRegion, double *bounds)
{
	// Access edge array using idx
	unsigned int idx = 0;
	if (targetLevel != 0) idx = targetLevel + targetRegion * L_NUM_LEVELS;

	// If start of bounds outside end of grid, no union
	if (
		bounds[eXMin] > global_edges[eXMax][idx] ||
		bounds[eYMin] > global_edges[eYMax][idx]
#if (L_DIMS == 3)
		|| bounds[eZMin] > global_edges[eZMax][idx]
#endif
		) return 0;

	// If end of bounds is before the start of the grid, no union
	if (
		bounds[eXMax] < global_edges[eXMin][idx] ||
		bounds[eYMax] < global_edges[eYMin][idx]
#if (L_DIMS == 3)
		|| bounds[eZMax] < global_edges[eZMin][idx]
#endif
		) return 0;

	// All other scenarios can produce a union //

	// Create store for the union edges
	double union_bounds[6];
	union_bounds[eXMin] = (bounds[eXMin] > global_edges[eXMin][idx]) ? bounds[eXMin] : global_edges[eXMin][idx];
	union_bounds[eXMax] = (bounds[eXMax] < global_edges[eXMax][idx]) ? bounds[eXMax] : global_edges[eXMax][idx];
	union_bounds[eYMin] = (bounds[eYMin] > global_edges[eYMin][idx]) ? bounds[eYMin] : global_edges[eYMin][idx];
	union_bounds[eYMax] = (bounds[eYMax] < global_edges[eYMax][idx]) ? bounds[eYMax] : global_edges[eYMax][idx];
#if (L_DIMS == 3)
	union_bounds[eZMin] = (bounds[eZMin] > global_edges[eZMin][idx]) ? bounds[eZMin] : global_edges[eZMin][idx];
	union_bounds[eZMax] = (bounds[eZMax] < global_edges[eZMax][idx]) ? bounds[eZMax] : global_edges[eZMax][idx];
#endif


	// Determine area / volume of the union
	double volume = (union_bounds[eXMax] - union_bounds[eXMin]) * (union_bounds[eYMax] - union_bounds[eYMin]);
#if (L_DIMS == 3)
	volume *= (union_bounds[eZMax] - union_bounds[eZMin]);
#endif

	// Use knowledge of discretisation to return the number of cells in union
	double base_cell_size = L_BX / static_cast<double>(L_N);
	double local_cell_size = base_cell_size / pow(2, targetLevel);
#if (L_DIMS == 3)
	return static_cast<long>(volume / (local_cell_size * local_cell_size * local_cell_size));
#else
	return static_cast<long>(volume / (local_cell_size * local_cell_size));
#endif
}