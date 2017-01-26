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
#include "../inc/GridObj.h"
#include "../inc/ObjectManager.h"
#include "../inc/MpiManager.h"
#include "../inc/GridUtils.h"


// *****************************************************************************
///	\brief Performs MPI communication depending on enumeration type case.
///	\param	type		type of communication.
///	\param	iBody		the current IB body.
///	\param	numMarkers	where the buffer is being unpacked to
void ObjectManager::ibm_getNumMarkers(eIBInfoType type, IBBody *iBody, std::vector<int> &rankID, std::vector<int> &numMarkers) {

	// Get MPI Manager Instance
	MpiManager *mpim = MpiManager::getInstance();

	switch (type)
	{
	case eBodyNumMarkers:
		{
			// Data type to be sent
			MPI_Datatype mpi_type = MPI_INT;

			// Get how many markers from each rank the owning rank should ask for and pack into buffer
			std::vector<int> bufferSend;
			ibm_mpi_pack(type, iBody, bufferSend);

			// Instantiate the receive buffer
			std::vector<int> bufferRecv;

			// Only resize buffer if this is the owning rank
			if (mpim->my_rank == iBody->owningRank)
				bufferRecv.resize(bufferSend.size() * mpim->num_ranks);

			// Send to owning rank (root process)
			MPI_Gather(&bufferSend.front(), bufferSend.size(), mpi_type, &bufferRecv.front(), bufferSend.size(), mpi_type, iBody->owningRank, mpim->world_comm);

			// Unpack back into required vectors class
			if (mpim->my_rank == iBody->owningRank)
				ibm_mpi_unpack(type, bufferRecv, bufferSend.size(), rankID, numMarkers);

			break;
		}
	}
}

// *****************************************************************************
///	\brief Performs MPI communication depending on enumeration type case.
///	\param	type		type of communication.
///	\param	iBody		the current IB body.
///	\param	numMarkers	where the buffer is being unpacked to
void ObjectManager::ibm_getMarkerPositions(eIBInfoType type, IBBody *iBody, std::vector<double> &markerPos, std::vector<int> &rankID, std::vector<int> &numMarkers) {

	// Get MPI Manager Instance
	MpiManager *mpim = MpiManager::getInstance();

	switch (type)
	{
	case eBodyMarkerPositions:
		{
			// Data type to be sent
			MPI_Datatype mpi_type = MPI_DOUBLE;

			// Get how many markers from each rank the owning rank should ask for and pack into buffer
			std::vector<double> bufferSend;
			if (iBody->markers.size() > 0)
				ibm_mpi_pack(type, iBody, bufferSend);

//			if (mpim->my_rank == 1) {
//				for (int i = 0; i < bufferSend.size(); i++)
//					std::cout << bufferSend[i] << " ";
//			}

			// Instantiate the receive buffer
			std::vector<std::vector<double>> bufferRecv;

			// Only resize buffer if this is the owning rank
			if (mpim->my_rank == iBody->owningRank) {
				bufferRecv.resize(rankID.size());
				for (int i = 0; i < bufferRecv.size(); i++) {
					bufferRecv[i].resize(numMarkers[i] * L_DIMS);
				}
			}

			// Send the message
			if (iBody->markers.size() > 0 && mpim->my_rank != iBody->owningRank)
				MPI_Send(&bufferSend.front(), bufferSend.size(), mpi_type, iBody->owningRank, mpim->my_rank, mpim->world_comm);

			// Receive the message
			if (mpim->my_rank == iBody->owningRank) {
				for (int i = 0; i < rankID.size(); i++) {
					MPI_Recv(&bufferRecv[i].front(), bufferRecv[i].size(), mpi_type, rankID[i], rankID[i], mpim->world_comm, MPI_STATUS_IGNORE);
				}
			}

			if (mpim->my_rank == iBody->owningRank) {
				std::cout << "Rank " << mpim->my_rank << std::endl;
				for (int i = 0; i < bufferRecv.size(); i++) {
					for (int j = 0; j < bufferRecv[i].size(); j++) {
						std::cout << bufferRecv[i][j] << " ";
					}
					std::cout << std::endl;
				}
			}


			MPI_Barrier(mpim->world_comm);
			exit(0);


//
//			// Send to owning rank (root process)
//			MPI_Gather(&bufferSend.front(), bufferSend.size(), mpi_type, &bufferRecv.front(), bufferSend.size(), mpi_type, iBody->owningRank, mpim->world_comm);
//
//			// Unpack back into required vectors class
//			if (mpim->my_rank == iBody->owningRank)
//				ibm_mpi_unpack(type, bufferRecv, bufferSend.size(), rankID, numMarkers);

			break;
		}
	}
}

///	\brief Packs the send buffer into bufferSend.
///	\param	type			type of communication.
///	\param	iBody			pointer to the current iBody
///	\param	buffer			the sender buffer that is being packed.
void ObjectManager::ibm_mpi_pack(eIBInfoType type, IBBody *iBody, std::vector<int> &buffer) {

	// Get MPI Manager Instance
	MpiManager *mpim = MpiManager::getInstance();

	switch (type)
	{

	case eBodyNumMarkers:
		{

			// Fill the vector with required data
			buffer.push_back(mpim->my_rank);
			buffer.push_back(iBody->markers.size());

			break;
		}
	}
}


///	\brief Packs the send buffer into bufferSend.
///	\param	type			type of communication.
///	\param	iBody			pointer to the current iBody
///	\param	buffer			the sender buffer that is being packed.
void ObjectManager::ibm_mpi_pack(eIBInfoType type, IBBody *iBody, std::vector<double> &buffer) {

	switch (type)
	{

	case eBodyMarkerPositions:
		{

			// Resize the buffer first
			buffer.resize(iBody->markers.size() * L_DIMS);

			// Loop through all markers to be sent and add them to the buffer
			for (int m = 0; m < iBody->markers.size(); m++) {
				buffer[m*L_DIMS] = iBody->markers[m].position[eXDirection];
				buffer[m*L_DIMS+1] = iBody->markers[m].position[eYDirection];
#if (L_DIMS == 3)
				buffer[m*L_DIMS+2] = iBody->markers[m].position[eZDirection];
#endif

			}

			break;
		}
	}
}

///	\brief Unpacks the receive buffer back into IBInfo class.
///	\param	type			type of communication.
///	\param	bufferRecv		the receiver buffer that is being unpacked.
///	\param	numMarkers		where the buffer is being unpacked to
void ObjectManager::ibm_mpi_unpack(eIBInfoType type, std::vector<int> &bufferRecv, int bufferSendSize, std::vector<int> &rankID, std::vector<int> &numMarkers) {

	// Get MPI Manager Instance
	MpiManager *mpim = MpiManager::getInstance();

	switch (type)
	{
	case eBodyNumMarkers:
		{

			// Loop through each rank
			for (int i = 0; i < mpim->num_ranks; i++) {

				// Check which ranks it should be receiving non-zero number of markers from
				if (bufferRecv[i*bufferSendSize] != mpim->my_rank && bufferRecv[i*bufferSendSize+1] > 0) {
					rankID.push_back(bufferRecv[i*bufferSendSize]);
					numMarkers.push_back(bufferRecv[i*bufferSendSize+1]);
				}
			}

			break;
		}
	}
}

// ****************************************************************************