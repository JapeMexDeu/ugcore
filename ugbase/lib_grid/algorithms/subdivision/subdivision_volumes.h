/*
 * Copyright (c) 2014-2015:  G-CSC, Goethe University Frankfurt
 * Author: Martin Stepniewski
 *
 * This file is part of UG4.
 *
 * UG4 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 *
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on UG4 (www.ug4.org/license)".
 *
 * (2) The following notice must be displayed at a prominent place in the
 * terminal output of covered works: "Based on UG4 (www.ug4.org/license)".
 *
 * (3) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S., Vogel, A., Heppner, I., Rupp, M., and Wittum, G. A massively
 *   parallel geometric multigrid solver on hierarchically distributed grids.
 *   Computing and visualization in science 16, 4 (2013), 151-164"
 * "Vogel, A., Reiter, S., Rupp, M., Nägel, A., and Wittum, G. UG4 -- a novel
 *   flexible software system for simulating pde based models on high performance
 *   computers. Computing and visualization in science 16, 4 (2013), 165-179"
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#ifndef __H__UG__SUBDIVISION_VOLUMES__
#define __H__UG__SUBDIVISION_VOLUMES__

#include <vector>
#include <cassert>
#include "lib_grid/lg_base.h"
#include "lib_grid/algorithms/debug_util.h"
#include "lib_grid/grid_objects/tetrahedron_rules.h"
#include "lib_grid/algorithms/attachment_util.h"

#ifdef UG_PARALLEL
	#include "lib_grid/parallelization/util/compol_attachment_reduce.h"
	#include "lib_grid/parallelization/util/compol_copy_attachment.h"
	#include "lib_grid/parallelization/util/attachment_operations.hpp"
	#include "lib_grid/parallelization/distributed_grid.h"
	#include "pcl/pcl_interface_communicator.h"
#endif

#include  "common/profiler/profiler.h"

namespace ug
{

////////////////////////////////////////////////////////////////////////////////
//	BOUNDARY REFINEMENT RULE

/// identification of boundary refinement rule to be used
enum GlobalBoundaryRefinementRule
{
	LINEAR,
	SUBDIV_SURF_LOOP_SCHEME,
	SUBDIV_SURF_AVERAGING_SCHEME,
	SUBDIV_VOL
};


/// global boundary refinement rule information switching between linear and subdivision Loop refinement
static GlobalBoundaryRefinementRule g_boundaryRefinementRule = LINEAR;


void SetBoundaryRefinementRule(GlobalBoundaryRefinementRule refRule)
{
	g_boundaryRefinementRule = refRule;
}


GlobalBoundaryRefinementRule GetBoundaryRefinementRule()
{
	return g_boundaryRefinementRule;
}


/// Function for splitting an octahedron to 4 sub-tetrahedrons
/** Recall the refinement of a tetrahedron (s. tetrahdron_rules.cpp). A tetrahedron is
 *	refined into 4 outer tetrahedrons and 4 inner tetrahedrons. After the 4 outer
 *	tetrahedrons are created the remaining inner cavity corresponds to an octahedron.
 *	This octahedron can be split into 4 tetrahedrons in 3 different ways, depending
 *	on the length of the following diagonals:
 *	Based on the original tetrahedron we look at the three diagonals between the
 *	following edge-centers: 0-5, 1-3, 2-4
 *
 *	The diagonal between edge-centers 0-5 of the tetrahedron equals
 *	a segment between vertices 1 and 3 of the octahedron
 *
 *	The diagonal between edge-centers 1-3 of the tetrahedron equals
 *	a segment between vertices 0 and 5 of the octahedron
 *
 *	the diagonal between edge-centers 2-4 of the tetrahedron equals
 *	a segment between vertices 2 and 4 of the octahedron
 *
 *	HINT: preferably use bestDiag = 0, as it is the inherent diagonal
 *		  along which the octahedron was adaptively orientated according to
 *		  tetrahedron_rules.cpp
 *
 *
 *	@param grid			reference to grid
 * 	@param oct			pointer to octahedron
 * 	@param parentVol	pointer to parent volume
 * 	@param vTetsOut		reference to vector with pointers to new tetrahedrons

**/
void SplitOctahedronToTetrahedrons(	Grid& grid, Octahedron* oct, Volume* parentVol,
									std::vector<Tetrahedron*>& vTetsOut, int bestDiag)
{
//	Position attachment management
	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);

//	Determine the shortest diagonal to split upon the octahedron
	if(bestDiag != 0 && bestDiag != 1 && bestDiag != 2)
	{
		bestDiag = 2;

		number d05 = VecDistanceSq(aaPos[oct->vertex(1)], aaPos[oct->vertex(3)]);
		number d13 = VecDistanceSq(aaPos[oct->vertex(0)], aaPos[oct->vertex(5)]);
		number d   = VecDistanceSq(aaPos[oct->vertex(2)], aaPos[oct->vertex(4)]);

		if(d13 < d){
			bestDiag = 1;
			d = d13;
		}
		if(d05 < d){
			bestDiag = 0;
		}
	}

	Tetrahedron* tet1;
	Tetrahedron* tet2;
	Tetrahedron* tet3;
	Tetrahedron* tet4;

	switch(bestDiag){

		case 0:// diag: 0-5
		//	Remark: element creation without father element specification
			tet1 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(1),
															oct->vertex(0),
															oct->vertex(4),
															oct->vertex(3)), parentVol);

			tet2 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(0),
															oct->vertex(2),
															oct->vertex(3),
															oct->vertex(1)), parentVol);

			tet3 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(4),
															oct->vertex(3),
															oct->vertex(5),
															oct->vertex(1)), parentVol);

			tet4 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(1),
															oct->vertex(5),
															oct->vertex(2),
															oct->vertex(3)), parentVol);

			vTetsOut.push_back(tet1);
			vTetsOut.push_back(tet2);
			vTetsOut.push_back(tet3);
			vTetsOut.push_back(tet4);

		//	compare case 0 tetrahedron pattern from tetrahedron_rules.cpp:
			/*
			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 0;	inds[fi++] = NUM_VERTICES + 1;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 5;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 1;	inds[fi++] = NUM_VERTICES + 4;
			inds[fi++] = NUM_VERTICES + 5;	inds[fi++] = NUM_VERTICES + 0;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 5;
			inds[fi++] = NUM_VERTICES + 3;	inds[fi++] = NUM_VERTICES + 0;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 0;	inds[fi++] = NUM_VERTICES + 3;
			inds[fi++] = NUM_VERTICES + 4;	inds[fi++] = NUM_VERTICES + 5;
			*/

			break;

		case 1:// diag: 1-3
			tet1 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(1),
															oct->vertex(0),
															oct->vertex(4),
															oct->vertex(5)), parentVol);

			tet2 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(0),
															oct->vertex(2),
															oct->vertex(3),
															oct->vertex(5)), parentVol);

			tet3 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(4),
															oct->vertex(3),
															oct->vertex(5),
															oct->vertex(0)), parentVol);

			tet4 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(1),
															oct->vertex(5),
															oct->vertex(2),
															oct->vertex(0)), parentVol);

			vTetsOut.push_back(tet1);
			vTetsOut.push_back(tet2);
			vTetsOut.push_back(tet3);
			vTetsOut.push_back(tet4);

		//	compare case 1 tetrahedron pattern from tetrahedron_rules.cpp:
			/*
			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 0;	inds[fi++] = NUM_VERTICES + 1;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 3;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 1;	inds[fi++] = NUM_VERTICES + 4;
			inds[fi++] = NUM_VERTICES + 5;	inds[fi++] = NUM_VERTICES + 3;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 5;
			inds[fi++] = NUM_VERTICES + 3;	inds[fi++] = NUM_VERTICES + 1;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 0;	inds[fi++] = NUM_VERTICES + 3;
			inds[fi++] = NUM_VERTICES + 4;	inds[fi++] = NUM_VERTICES + 1;
			*/

			break;

		case 2:// diag 2-4
			tet1 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(1),
															oct->vertex(4),
															oct->vertex(5),
															oct->vertex(2)), parentVol);

			tet2 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(0),
															oct->vertex(4),
															oct->vertex(1),
															oct->vertex(2)), parentVol);

			tet3 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(4),
															oct->vertex(5),
															oct->vertex(2),
															oct->vertex(3)), parentVol);

			tet4 = *grid.create<Tetrahedron>(TetrahedronDescriptor(	oct->vertex(2),
															oct->vertex(0),
															oct->vertex(4),
															oct->vertex(3)), parentVol);

			vTetsOut.push_back(tet1);
			vTetsOut.push_back(tet2);
			vTetsOut.push_back(tet3);
			vTetsOut.push_back(tet4);

		//	compare case 2 tetrahedron pattern from tetrahedron_rules.cpp:
			/*
			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 0;	inds[fi++] = NUM_VERTICES + 2;
			inds[fi++] = NUM_VERTICES + 3;	inds[fi++] = NUM_VERTICES + 4;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 1;	inds[fi++] = NUM_VERTICES + 2;
			inds[fi++] = NUM_VERTICES + 0;	inds[fi++] = NUM_VERTICES + 4;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 3;
			inds[fi++] = NUM_VERTICES + 4;	inds[fi++] = NUM_VERTICES + 5;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 4;	inds[fi++] = NUM_VERTICES + 1;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 5;
			*/

			break;
	}
}


/// Conversion function for hybrid tetra-/octahedral multigrids
/** This function converts each octahedron in all levels to
 * 	four tetrahedra and deletes the original octahedra
 * 	from the multigrid.
 *
 * 	WARNING: correct parent <-> childhood relationships won't persist
 *
 * 	@param mg			reference to MultiGrid
 * 	@param bestDiag		specify fixed or adaptive diagonal for octahedral split
**/
void TetrahedralizeHybridTetOctGrid(MultiGrid& mg, int bestDiag)
{
	PROFILE_FUNC_GROUP("subdivision_volumes");

	if(bestDiag != 0 && bestDiag != 1 && bestDiag != 2)
	{
		bestDiag = -1;
	}

//	Position attachment management
	Grid::VertexAttachmentAccessor<APosition> aaPos(mg, aPosition);

	std::vector<Tetrahedron*> vTetsOut;

//	Loop over all levels and split octahedrons
	for(size_t i = mg.num_levels()-1; i > 0; --i)
	{
	//	Loop over all octahedrons on each level
		for(VolumeIterator octIter = mg.begin<Octahedron>(i); octIter != mg.end<Octahedron>(i); ++octIter)
		{
			Octahedron* oct 	= dynamic_cast<Octahedron*>(*octIter);
			Volume* parentVol 	= dynamic_cast<Volume*>(mg.get_parent(oct));

			SplitOctahedronToTetrahedrons(mg, oct, parentVol, vTetsOut, bestDiag);
		}
	}

//	Erase all octahedrons in multigrid
	while(mg.begin<Octahedron>() != mg.end<Octahedron>())
	{
		Octahedron* oct = *mg.begin<Octahedron>();
		mg.erase(oct);
	}
}


/// Projection function for smooth subdivision (volumes+surface) refinement
/** This function projects the vertices of all levels to their smooth limit
 * 	positions determined by the subdivision volumes refinement.
 *
 * 	@param mg			reference to MultiGrid
**/
void ProjectHierarchyToLimitSubdivisionVolume(MultiGrid& mg)
{
	PROFILE_FUNC_GROUP("subdivision_volumes");

//	Catch use of procedure for MultiGrids with just one level
	if(mg.num_levels() == 1)
	{
		UG_THROW("Error in ProjectHierarchyToLimitSubdivisionVolume: "
				 "Procedure only to be used for MultiGrids with more than one level.");
	}

	#ifdef UG_PARALLEL
	//	Attachment communication policies COPY
		ComPol_CopyAttachment<VertexLayout, AVector3> comPolCopyAPosition(mg, aPosition);

	//	Interface communicators and distributed domain manager
		pcl::InterfaceCommunicator<VertexLayout> com;
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
		GridLayoutMap& glm = dgm.grid_layout_map();
	#endif

	Grid::VertexAttachmentAccessor<APosition> aaPos(mg, aPosition);

//	AttachmentCopy VSLAVE->VMASTER on mg.top_level() in case not all VMasters in toplevel don't have correct position already
	#ifdef UG_PARALLEL
	//	copy v_slaves to ghosts = VMASTER
		com.exchange_data(glm, INT_V_SLAVE, INT_V_MASTER, comPolCopyAPosition);
		com.communicate();
	#endif

//	Loop all levels from toplevel down to base level
	for(int lvl = (int)mg.top_level(); lvl > 0; --lvl)
	{
	//	Loop all vertices of current level and submit positions to parent vertices
		for(VertexIterator vrtIter = mg.begin<Vertex>(lvl); vrtIter != mg.end<Vertex>(lvl); ++vrtIter)
		{
			Vertex* v = *vrtIter;
			Vertex* parent = dynamic_cast<Vertex*>(mg.get_parent(v));

		//	Only, if parent vertex exists
			if(parent)
			{
				aaPos[parent] = aaPos[v];
			}
		}

	#ifdef UG_PARALLEL
	//	copy v_slaves to ghosts = VMASTER
		com.exchange_data(glm, INT_V_SLAVE, INT_V_MASTER, comPolCopyAPosition);
		com.communicate();
	#endif
	}
}


/// Parent level vertex smoothing function for subdivision surfaces refinement (by C. Loop, 1987)
/** This function calculates the smoothed positions of all parent level vertices
 * 	determined by the subdivision surfaces refinement.
 *
 * 	@param mg						reference to MultiGrid
 * 	@param markSH					reference to SubsetHandler markSH containing marked (inner) boundary manifold
 * 	@param linearManifoldSH			reference to user-specified linearManifoldSubsets SubsetHandler
 * 	@param aSmoothBndPosEvenVrt		reference to aSmoothBndPosEvenVrt
 * 	@param aSmoothBndPosOddVrt		reference to aSmoothBndPosOddVrt
 * 	@param aNumManifoldEdges		reference to aNumManifoldEdges
**/
void CalculateSmoothManifoldPosInParentLevelLoopScheme(MultiGrid& mg, MGSubsetHandler& markSH,
											 	 	   MGSubsetHandler& linearManifoldSH,
													   APosition& aSmoothBndPosEvenVrt,
													   APosition& aSmoothBndPosOddVrt,
													   AInt& aNumManifoldEdges)
{
//	Catch use of procedure for MultiGrids with just one level
	if(mg.num_levels() == 1)
	{
		UG_THROW("Error in CalculateSmoothManifoldPosInParentLevel: "
				 "Procedure only to be used for MultiGrids with more than one level.");
	}

//	Define attachment accessors
	Grid::VertexAttachmentAccessor<APosition> aaPos(mg, aPosition);
	Grid::VertexAttachmentAccessor<APosition> aaSmoothBndPosEvenVrt(mg, aSmoothBndPosEvenVrt);
	Grid::EdgeAttachmentAccessor<APosition> aaSmoothBndPosOddVrt(mg, aSmoothBndPosOddVrt);
	Grid::VertexAttachmentAccessor<AInt> aaNumManifoldEdges(mg, aNumManifoldEdges);

	#ifdef UG_PARALLEL
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
	#endif

//	Declare centroid coordinate vector
	typedef APosition::ValueType pos_type;
	pos_type p;
	VecSet(p, 0);

//	Load subdivision surfaces rules
	SubdivRules_PLoop& subdiv = SubdivRules_PLoop::inst();

//	Calculate smooth position for EVEN vertices
	for(VertexIterator vrtIter = mg.begin<Vertex>(mg.top_level()-1); vrtIter != mg.end<Vertex>(mg.top_level()-1); ++vrtIter)
	{
		VecSet(p, 0);

		Vertex* vrt = *vrtIter;

	//	Skip ghost vertices
		#ifdef UG_PARALLEL
			if(dgm.is_ghost(vrt))
				continue;
		#endif

	//	In case of marked manifold vertices, which do not belong to the user-specified linear boundary manifold subsets,
	//	and activated subdivision Loop refinement calculate subdivision surfaces smooth position
		if(markSH.get_subset_index(vrt) != -1 && linearManifoldSH.get_subset_index(vrt) == -1)
		{
		//	perform loop subdivision on even manifold vertices
		//	first get neighbored manifold vertices
			for(Grid::AssociatedEdgeIterator eIter = mg.associated_edges_begin(vrt); eIter != mg.associated_edges_end(vrt); ++eIter)
			{
				Edge* e = *eIter;

			//	Only consider associated edges, which are marked as manifold edges
				if(markSH.get_subset_index(e) != -1)
				{
				//	Exclude ghost and horizontal slave neighbor vertices from contributing to centroid
					#ifdef UG_PARALLEL
						if(dgm.is_ghost(e))
							continue;

						if(dgm.contains_status(e, ES_H_SLAVE))
							continue;
					#endif

					VecAdd(p, p, aaPos[GetConnectedVertex(e, vrt)]);
				}
			}

			number centerWgt 	= subdiv.ref_even_inner_center_weight(aaNumManifoldEdges[vrt]);
			number nbrWgt 		= subdiv.ref_even_inner_nbr_weight(aaNumManifoldEdges[vrt]);

		//	Exclude horizontal slaves of the currently smoothed vertex to avoid multiple contributions to centroid
			#ifdef UG_PARALLEL
				if(dgm.is_ghost(vrt))
					continue;

				if(dgm.contains_status(vrt, ES_H_SLAVE))
				{
					VecScaleAppend(aaSmoothBndPosEvenVrt[vrt], nbrWgt, p);
					continue;
				}
			#endif

			VecScaleAppend(aaSmoothBndPosEvenVrt[vrt], centerWgt, aaPos[vrt], nbrWgt, p);
		}
	}

	/*
	 * Smoothing of odd vertices x
	 *
	 * Weights of face adjacent vertices to x: 1/8
	 * Weights of edge adjacent vertices to x: 3/8
	 *
				1/8
				/ \
			3/8--x--3/8
				\ /
				1/8
	 *
	 */

//	Calculate smooth position for ODD vertices
	for(EdgeIterator eIter = mg.begin<Edge>(mg.top_level()-1); eIter != mg.end<Edge>(mg.top_level()-1); ++eIter)
	{
		VecSet(p, 0);

		Edge* e = *eIter;

	//	Skip ghost edges
		#ifdef UG_PARALLEL
			if(dgm.is_ghost(e))
				continue;
		#endif

	//	In case of marked manifold edges, which do not belong to the user-specified linear boundary manifold subsets,
	//	and activated subdivision Loop refinement calculate subdivision surfaces smooth position
		if(markSH.get_subset_index(e) != -1 && linearManifoldSH.get_subset_index(e) == -1)
		{
		//	perform loop subdivision on odd manifold vertices
		//	get the neighbored manifold triangles
			std::vector<Face*> associatedFaces;
			std::vector<Face*> associatedManifoldFaces;

			CollectAssociated(associatedFaces, mg, e);

			for(size_t i = 0; i < associatedFaces.size(); ++i)
			{

			//	Only consider associated faces, which are marked as manifold faces
				if(markSH.get_subset_index(associatedFaces[i]) != -1)
				{
				//	Exclude ghost and horizontal slave manifold faces
					#ifdef UG_PARALLEL
						if(dgm.is_ghost(associatedFaces[i]))
							continue;

						if(dgm.contains_status(associatedFaces[i], ES_H_SLAVE))
							continue;
					#endif

					if(associatedManifoldFaces.size() < 2)
					{
						associatedManifoldFaces.push_back(associatedFaces[i]);
					}
				}
			}

			if(associatedManifoldFaces.size() <= 2)
			{
			//	Check, if all faces are triangles
				for(size_t i = 0; i < associatedManifoldFaces.size(); ++i)
				{
					if(associatedManifoldFaces[i]->num_vertices() != 3)
					{
						UG_THROW("ERROR in CalculateSmoothManifoldPosInParentLevel: Non triangular faces included in grid.");
					}
				}

			//	Summate centroid of face adjacent vertices
				for(size_t i = 0; i < associatedManifoldFaces.size(); ++i)
				{
					VecAdd(p, p, aaPos[GetConnectedVertex(e, associatedManifoldFaces[i])]);
				}

			//	Exclude ghost and horizontal slaves of the parent edge vertices of the currently smoothed vertex
			//	to avoid multiple contributions to the centroid of the edge adjacent vertices
				#ifdef UG_PARALLEL
					if(dgm.is_ghost(e))
					{
						continue;
					}

					if(dgm.contains_status(e, ES_H_SLAVE))
					{
						VecScaleAppend(aaSmoothBndPosOddVrt[e], 0.125, p);
						continue;
					}
				#endif

				VecScaleAppend(aaSmoothBndPosOddVrt[e], 0.375, aaPos[e->vertex(0)], 0.375, aaPos[e->vertex(1)], 0.125, p);
			}
			else
				UG_THROW("ERROR in CalculateSmoothManifoldPosInParentLevel: numAssociatedManifoldFaces > 2.");
		}
	}

//	Manage vertex and edge attachment communication in parallel case -> COMMUNICATE aSmoothBndPosEvenVrt, aSmoothBndPosOddVrt
	#ifdef UG_PARALLEL
	//	Reduce add operations:
	//	sum up h_slaves into h_masters

	//	Copy operations:
	//	copy h_masters to h_slaves for consistency
		AttachmentAllReduce<Vertex>(mg, aSmoothBndPosEvenVrt, PCL_RO_SUM);
		AttachmentAllReduce<Edge>(mg, aSmoothBndPosOddVrt, PCL_RO_SUM);
	#endif
}


/// Toplevel vertex smoothing function for subdivision surfaces refinement (Averaging scheme)
/** This function calculates the smoothed positions of all toplevel vertices
 * 	determined by the subdivision surfaces refinement.
 *
 * 	@param mg					reference to MultiGrid
 * 	@param markSH				reference to SubsetHandler markSH containing marked (inner) boundary manifold
 * 	@param linearManifoldSH		reference to user-specified linearManifoldSubsets SubsetHandler
 * 	@param aSmoothBndPos		reference to aSmoothBndPos
**/
void CalculateSmoothManifoldPosInTopLevelAveragingScheme(MultiGrid& mg, MGSubsetHandler& markSH,
														 MGSubsetHandler& linearManifoldSH,
										  	  	  	     APosition& aSmoothBndPos)
{
//	Define attachment accessors
	Grid::VertexAttachmentAccessor<APosition> aaPos(mg, aPosition);
	Grid::VertexAttachmentAccessor<APosition> aaSmoothBndPos(mg, aSmoothBndPos);

	#ifdef UG_PARALLEL
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
	#endif

//	Declare centroid coordinate vector
	typedef APosition::ValueType pos_type;
	pos_type p;
	VecSet(p, 0);

//	Loop all manifold faces of top_level
	for(FaceIterator fIter = mg.begin<Face>(mg.top_level()); fIter != mg.end<Face>(mg.top_level()); ++fIter)
	{
		Face* f = *fIter;

	//	Skip ghost volumes
		#ifdef UG_PARALLEL
			if(dgm.is_ghost(f))
				continue;
		#endif

	//	In case of marked manifold faces, which do not belong to the user-specified linear boundary manifold subsets,
	//	and activated Averaging scheme calculate subdivision surfaces smooth position
		if(markSH.get_subset_index(f) != -1 && linearManifoldSH.get_subset_index(f) == -1)
		{
			if(f->num_vertices() != 3)
				UG_THROW("ERROR in CalculateSmoothManifoldPosInTopLevelAveragingScheme: Non triangular faces included in grid.");

		//	Iterate over all face vertices, calculate and apply local centroid masks
			for(size_t i = 0; i < f->num_vertices(); ++i)
			{
			//	Init
				Vertex* vrt = f->vertex(i);
				VecSet(p, 0);

			//	Summate coordinates of neighbor vertices to vrt inside face
				for(size_t j = 0; j < f->num_vertices(); ++j)
				{
					if(j != i)
					{
						VecAdd(p, p, aaPos[f->vertex(j)]);
					}
				}

			//	Smooth vertex position
				VecScaleAppend(aaSmoothBndPos[vrt], 2.0/8, aaPos[vrt], 3.0/8, p);
			}
		}
	}

//	Manage vertex attachment communication in parallel case -> COMMUNICATE aaSmoothBndPos
	#ifdef UG_PARALLEL
	//	Reduce add operations:
	//	sum up h_slaves into h_masters

	//	Copy operations:
	//	copy h_masters to h_slaves for consistency
		AttachmentAllReduce<Vertex>(mg, aSmoothBndPos, PCL_RO_SUM);
	#endif
}


/// Toplevel vertex smoothing function for subdivision volumes refinement
/** This function calculates the smoothed positions of all toplevel vertices
 * 	determined by the subdivision volumes refinement.
 *
 * 	@param mg				reference to MultiGrid
 * 	@param markSH			reference to SubsetHandler markSH containing marked (inner) boundary manifold
 * 	@param aSmoothVolPos	reference to aSmoothVolPos
**/
void CalculateSmoothVolumePosInTopLevel(MultiGrid& mg, MGSubsetHandler& markSH,
										APosition& aSmoothVolPos)
{
	#ifdef UG_PARALLEL
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
	#endif

//	Define attachment accessors
	Grid::VertexAttachmentAccessor<APosition> aaPos(mg, aPosition);
	Grid::VertexAttachmentAccessor<APosition> aaSmoothVolPos(mg, aSmoothVolPos);

//	Declare volume centroid coordinate vector
	typedef APosition::ValueType pos_type;
	pos_type p;

//	Loop all volumes of top_level
	for(VolumeIterator vIter = mg.begin<Volume>(mg.top_level()); vIter != mg.end<Volume>(mg.top_level()); ++vIter)
	{
		Volume* vol = *vIter;

	//	Skip ghost volumes
		#ifdef UG_PARALLEL
			if(dgm.is_ghost(vol))
				continue;
		#endif

	//	Iterate over all volume vertices, calculate and apply local centroid masks
		for(size_t i = 0; i < vol->num_vertices(); ++i)
		{
		//	Init
			Vertex* vrt = vol->vertex(i);
			VecSet(p, 0);

		//	In case of linear or subdivision Loop boundary manifold refinement:
		//	handle vertices of separating manifolds separately
			if(markSH.get_subset_index(vrt) != -1 && g_boundaryRefinementRule != SUBDIV_VOL)
			{
				continue;
			}

		//	TETRAHEDRON CASE
			if(vol->reference_object_id() == ROID_TETRAHEDRON)
			{
			//	Summate coordinates of neighbor vertices to vrt inside tetrahedron
				for(size_t j = 0; j < vol->num_vertices(); ++j)
				{
					if(j != i)
					{
						VecAdd(p, p, aaPos[vol->vertex(j)]);
					}
				}

			//	Smooth vertex position
				VecScaleAppend(aaSmoothVolPos[vrt], -1.0/16, aaPos[vrt], 17.0/48, p);
			}

		//	OCTAHEDRON CASE
			else if(vol->reference_object_id() == ROID_OCTAHEDRON)
			{
			//	Get cell-adjacent vertex
				Vertex* oppVrt = vol->vertex(vol->get_opposing_object(vrt).second);

			//	Summate coordinates of DIRECT neighbor vertices to vrt inside octahedron
				for(size_t j = 0; j < vol->num_vertices(); ++j)
				{
					if(GetVertexIndex(vol, oppVrt) == -1)
					{
						UG_THROW("ERROR in CalculateSmoothVolumePosInTopLevel: identified opposing vertex actually not included in current volume.");
					}

					if(j != i && j != (size_t)GetVertexIndex(vol, oppVrt))
					{
						VecAdd(p, p, aaPos[vol->vertex(j)]);
					}
				}

			//	Smooth vertex position
				VecScaleAppend(aaSmoothVolPos[vrt], 3.0/8, aaPos[vrt], 1.0/12, p, 7.0/24, aaPos[oppVrt]);
			}

		//	UNSUPPORTED VOLUME ELEMENT CASE
			else
			{
				UG_THROW("ERROR in CalculateSmoothVolumePosInTopLevel: Volume type not supported for subdivision volumes refinement.");
			}
		}
	}

//	Manage vertex attachment communication in parallel case -> COMMUNICATE aSmoothVolPos
	#ifdef UG_PARALLEL
	//	Reduce add operations:
	//	sum up h_slaves into h_masters

	//	Copy operations:
	//	copy h_masters to h_slaves for consistency
		AttachmentAllReduce<Vertex>(mg, aSmoothVolPos, PCL_RO_SUM);
	#endif
}


/// Function for calculating the number of associated volumes of all toplevel vertices
/** This function calculates the number of associated volumes
 * 	for all toplevel vertices.
 *
 * 	@param mg				reference to MultiGrid
 * 	@param aNumElems		reference to aNumElems
**/
void CalculateNumElemsVertexAttachmentInTopLevel(MultiGrid& mg, AInt& aNumElems)
{
//	Define attachment accessor
	Grid::VertexAttachmentAccessor<AInt> aaNumElems(mg, aNumElems);

//	Manage vertex attachment communication in parallel case:
//	- Setup communication policy for the above attachment
//	- Setup interface communicator
//	- Setup distributed grid manager
//	- Setup grid layout map
	#ifdef UG_PARALLEL
	//	Attachment communication policies COPY
		ComPol_CopyAttachment<VertexLayout, AInt> comPolCopyNumElems(mg, aNumElems);

	//	Interface communicators and distributed domain manager
		pcl::InterfaceCommunicator<VertexLayout> com;
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
		GridLayoutMap& glm = dgm.grid_layout_map();
	#endif

//	Loop all volumes of top level and calculate number of volumes each vertex is contained by
	for(VolumeIterator vIter = mg.begin<Volume>(mg.top_level()); vIter != mg.end<Volume>(mg.top_level()); ++vIter)
	{
		Volume* vol = *vIter;

	//	Skip ghosts
		#ifdef UG_PARALLEL
			if(dgm.is_ghost(vol))
				continue;
		#endif

		for(size_t i = 0; i < vol->num_vertices(); ++i)
		{
			++aaNumElems[vol->vertex(i)];
		}
	}

//	Manage vertex attachment communication in parallel case -> COMMUNICATE aNumElems
	#ifdef UG_PARALLEL
	//	Reduce add operations:
	//	sum up h_slaves into h_masters

	//	Copy operations:
	//	copy h_masters to h_slaves for consistency
		AttachmentAllReduce<Vertex>(mg, aNumElems, PCL_RO_SUM);

	//	copy v_slaves to ghosts = VMASTER
		com.exchange_data(glm, INT_V_SLAVE, INT_V_MASTER, comPolCopyNumElems);
		com.communicate();
	#endif
}


/// Function for calculating the number of associated manifold edges of all parent level vertices
/** This function calculates the number of associated manifold edges
 * 	for all parent level vertices.
 *
 * 	@param mg					reference to MultiGrid
 * 	@param markSH				reference to SubsetHandler markSH containing marked (inner) boundary manifold
 * 	@param aNumManifoldEdges	reference to aNumManifoldEdges
**/
void CalculateNumManifoldEdgesVertexAttachmentInParentLevel(MultiGrid& mg, MGSubsetHandler& markSH,
															AInt& aNumManifoldEdges)
{
//	Define attachment accessor
	Grid::VertexAttachmentAccessor<AInt> aaNumManifoldEdges(mg, aNumManifoldEdges);

//	Manage vertex attachment communication in parallel case:
//	- Setup communication policy for the above attachment
//	- Setup interface communicator
//	- Setup distributed grid manager
//	- Setup grid layout map
	#ifdef UG_PARALLEL
	//	Attachment communication policies COPY
		ComPol_CopyAttachment<VertexLayout, AInt> comPolCopyNumManifoldEdges(mg, aNumManifoldEdges);

	//	Interface communicators and distributed domain manager
		pcl::InterfaceCommunicator<VertexLayout> com;
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
		GridLayoutMap& glm = dgm.grid_layout_map();
	#endif

//	Catch use of CalculateNumManifoldEdgesVertexAttachmentInParentLevel on MultiGrids with just one level
	if(mg.num_levels() == 1)
		UG_THROW("CalculateNumManifoldEdgesVertexAttachmentInParentLevel: method may not be used in base level 0.");

//	Loop all edges of parent level and calculate number of associated manifold edges of each vertex
	for(EdgeIterator eIter = mg.begin<Edge>(mg.top_level()-1); eIter != mg.end<Edge>(mg.top_level()-1); ++eIter)
	{
		Edge* e = *eIter;

	// 	Check, if edge is contained in subset with marked manifold elements
		if (markSH.get_subset_index(e) != -1)
		{
		//	Skip ghost and horizontal slave edges
			#ifdef UG_PARALLEL
				if(dgm.is_ghost(e))
					continue;

				if(dgm.contains_status(e, ES_H_SLAVE))
					continue;
			#endif

		   ++aaNumManifoldEdges[e->vertex(0)];
		   ++aaNumManifoldEdges[e->vertex(1)];
		}
	}

//	Manage vertex attachment communication in parallel case -> COMMUNICATE aNumManifoldEdges
	#ifdef UG_PARALLEL
	//	Reduce add operations:
	//	sum up h_slaves into h_masters

	//	Copy operations:
	//	copy h_masters to h_slaves for consistency
		AttachmentAllReduce<Vertex>(mg, aNumManifoldEdges, PCL_RO_SUM);

	//	copy v_slaves to ghosts = VMASTER
		com.exchange_data(glm, INT_V_SLAVE, INT_V_MASTER, comPolCopyNumManifoldEdges);
		com.communicate();
	#endif
}


/// Function for calculating the number of associated manifold faces of all toplevel manifold vertices
/** This function calculates the number of associated volumes
 * 	for all toplevel vertices.
 *
 * 	@param mg					reference to MultiGrid
 * 	@param markSH				reference to SubsetHandler markSH containing marked (inner) boundary manifold
 * 	@param aNumManifoldFaces	reference to aNumManifoldFaces
**/
void CalculateNumManifoldFacesVertexAttachmentInTopLevel(MultiGrid& mg, MGSubsetHandler& markSH, AInt& aNumManifoldFaces)
{
//	Define attachment accessor
	Grid::VertexAttachmentAccessor<AInt> aaNumManifoldFaces(mg, aNumManifoldFaces);

//	Manage vertex attachment communication in parallel case:
//	- Setup communication policy for the above attachment
//	- Setup interface communicator
//	- Setup distributed grid manager
//	- Setup grid layout map
	#ifdef UG_PARALLEL
	//	Attachment communication policies COPY
		ComPol_CopyAttachment<VertexLayout, AInt> comPolCopyNumManifoldFaces(mg, aNumManifoldFaces);

	//	Interface communicators and distributed domain manager
		pcl::InterfaceCommunicator<VertexLayout> com;
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
		GridLayoutMap& glm = dgm.grid_layout_map();
	#endif

//	Loop all manifold faces of top level and calculate number of faces each vertex is contained by
	for(FaceIterator fIter = mg.begin<Face>(mg.top_level()); fIter != mg.end<Face>(mg.top_level()); ++fIter)
	{
		Face* f = *fIter;

		if(f->num_vertices() != 3)
			UG_THROW("ERROR in CalculateNumManifoldFacesVertexAttachment: Non triangular faces included in grid.");

	//	Only consider boundary manifold faces
		if(markSH.get_subset_index(f) != -1)
		{
		//	Skip ghosts
			#ifdef UG_PARALLEL
				if(dgm.is_ghost(f))
					continue;
			#endif

			for(size_t i = 0; i < f->num_vertices(); ++i)
			{
				++aaNumManifoldFaces[f->vertex(i)];
			}
		}
	}

//	Manage vertex attachment communication in parallel case -> COMMUNICATE aNumElems
	#ifdef UG_PARALLEL
	//	Reduce add operations:
	//	sum up h_slaves into h_masters

	//	Copy operations:
	//	copy h_masters to h_slaves for consistency
		AttachmentAllReduce<Vertex>(mg, aNumManifoldFaces, PCL_RO_SUM);

	//	copy v_slaves to ghosts = VMASTER
		com.exchange_data(glm, INT_V_SLAVE, INT_V_MASTER, comPolCopyNumManifoldFaces);
		com.communicate();
	#endif
}


/// Procedure to initialize the linear boundary manifold subsets SubsetHandler with user-specified subsets
/** This procedure initializes the referenced linear boundary manifold subsets SubsetHandler
 * 	s.t. user-specified subsets
 *
 * 	@param dom						reference to Domain
 * 	@param linearManifoldSH			reference to user-specified linearManifoldSubsets SubsetHandler
**/
void InitLinearManifoldSubsetHandler(MultiGrid& mg, MGSubsetHandler& sh,
											   MGSubsetHandler& linearManifoldSH,
											   const char* linearManifoldSubsets)
{
//	Catch use of procedure for MultiGrids with just one level
	if(mg.num_levels() == 1)
	{
		UG_THROW("Error in InitLinearManifoldSubsetHandler: "
				 "Procedure only to be used for MultiGrids with more than one level.");
	}

//	tokenize user input
	std::vector<std::string> linearManifoldSubsetsString = TokenizeString(linearManifoldSubsets);

//	remove white space
	for(size_t i = 0; i < linearManifoldSubsetsString.size(); ++i)
	{
		RemoveWhitespaceFromString(linearManifoldSubsetsString[i]);
	}

//	if no subset passed, clear subsets
	if(linearManifoldSubsetsString.size() == 1 && linearManifoldSubsetsString[0].empty())
	{
		linearManifoldSubsetsString.clear();
	}

//	if subsets passed with separator, but not all tokens filled, throw error
	for(size_t i = 0; i < linearManifoldSubsetsString.size(); ++i)
	{
		if(linearManifoldSubsetsString.empty())
		{
			UG_THROW(	"ERROR in InitLinearManifoldSubsetHandler: "
						"linear boundary manifold subsets string passed lacks a "
						"subset specification at position "<<i<<"(of "
						<<linearManifoldSubsetsString.size()-1<<")");
		}
	}

// 	assign all user specified vertices to linear boundary manifold SubsetHandler
	for(size_t i = 0; i < linearManifoldSubsetsString.size(); ++i)
	{
		int j = sh.get_subset_index(linearManifoldSubsetsString[i].c_str());

		for(VertexIterator vrtIter = sh.begin<Vertex>(j, mg.top_level()); vrtIter != sh.end<Vertex>(j, mg.top_level()); ++vrtIter)
		{
			Vertex* vrt = *vrtIter;
			linearManifoldSH.assign_subset(vrt, 0);
		}

		for(VertexIterator vrtIter = sh.begin<Vertex>(j, mg.top_level()-1); vrtIter != sh.end<Vertex>(j, mg.top_level()-1); ++vrtIter)
		{
			Vertex* vrt = *vrtIter;
			linearManifoldSH.assign_subset(vrt, 0);
		}
	}

// 	assign all user specified edges to linear boundary manifold SubsetHandler
	for(size_t i = 0; i < linearManifoldSubsetsString.size(); ++i)
	{
		int j = sh.get_subset_index(linearManifoldSubsetsString[i].c_str());

		for(EdgeIterator eIter = sh.begin<Edge>(j, mg.top_level()); eIter != sh.end<Edge>(j, mg.top_level()); ++eIter)
		{
			Edge* e = *eIter;
			linearManifoldSH.assign_subset(e, 0);
		}

		for(EdgeIterator eIter = sh.begin<Edge>(j, mg.top_level()-1); eIter != sh.end<Edge>(j, mg.top_level()-1); ++eIter)
		{
			Edge* e = *eIter;
			linearManifoldSH.assign_subset(e, 0);
		}
	}

// 	assign all user specified faces to linear boundary manifold SubsetHandler
	for(size_t i = 0; i < linearManifoldSubsetsString.size(); ++i)
	{
		int j = sh.get_subset_index(linearManifoldSubsetsString[i].c_str());

		for(FaceIterator fIter = sh.begin<Face>(j, mg.top_level()); fIter != sh.end<Face>(j, mg.top_level()); ++fIter)
		{
			Face* f = *fIter;
			linearManifoldSH.assign_subset(f, 0);
		}

		for(FaceIterator fIter = sh.begin<Face>(j, mg.top_level()-1); fIter != sh.end<Face>(j, mg.top_level()-1); ++fIter)
		{
			Face* f = *fIter;
			linearManifoldSH.assign_subset(f, 0);
		}
	}

//	Debug log
//	UG_LOG("InitLinearManifoldSubsetHandler:" << std::endl);
//	UG_LOG(">> Applying linear subdivision on the following boundary manifold subsets:" << std::endl);
//
//	for(size_t i = 0; i < linearManifoldSubsetsString.size(); ++i)
//	{
//		UG_LOG("Subset # " << sh.get_subset_index(linearManifoldSubsetsString[i].c_str()) << ": " << linearManifoldSubsetsString[i] << std::endl);
//	}
}


/// Toplevel vertex repositioning function for subdivision surfaces refinement (by C. Loop, 1987)
/** This function repositions all toplevel manifold vertices to their smoothed positions
 * 	determined by the subdivision surfaces refinement.
 *
 * 	@param mg						reference to MultiGrid
 * 	@param markSH					reference to SubsetHandler markSH containing marked (inner) boundary manifold
 * 	@param linearManifoldSH			reference to user-specified linearManifoldSubsets SubsetHandler
 * 	@param aSmoothBndPosEvenVrt		reference to aSmoothBndPosEvenVrt
 * 	@param aSmoothBndPosOddVrt		reference to aSmoothBndPosOddVrt
**/
void ApplySmoothManifoldPosToTopLevelLoopScheme(MultiGrid& mg, MGSubsetHandler& markSH,
												MGSubsetHandler& linearManifoldSH)
{
//	Catch use of procedure for MultiGrids with just one level
	if(mg.num_levels() == 1)
	{
		UG_THROW("Error in ApplySmoothManifoldPosToTopLevelLoopScheme: "
				 "Procedure only to be used for MultiGrids with more than one level.");
	}


/*****************************************
 *
 *	(1) SETUP
 *
 *****************************************/

//	Vertex attachments for associated number of manifold edges and smooth position
//	(distinguish between volume and boundary smooth vertex positions
//	 and in case of boundary between EVEN and ODD smooth vertex positions)
	AInt aNumManifoldEdges;
	APosition aSmoothBndPosEvenVrt;
	APosition aSmoothBndPosOddVrt;

//	attach previously declared vertex attachments with initial value 0
	mg.attach_to_vertices_dv(aNumManifoldEdges, 0);
	mg.attach_to_vertices_dv(aSmoothBndPosEvenVrt, vector3(0, 0, 0));
	mg.attach_to_edges_dv(aSmoothBndPosOddVrt, vector3(0, 0, 0));

//	Define attachment accessors
	Grid::VertexAttachmentAccessor<APosition> aaPos(mg, aPosition);
	Grid::VertexAttachmentAccessor<APosition> aaSmoothBndPosEvenVrt(mg, aSmoothBndPosEvenVrt);
	Grid::EdgeAttachmentAccessor<APosition> aaSmoothBndPosOddVrt(mg, aSmoothBndPosOddVrt);

//	Manage vertex attachment communication in parallel case:
//	- Setup communication policy for the above attachment aPosition
//	- Setup interface communicator
//	- Setup distributed grid manager
//	- Setup grid layout map
	#ifdef UG_PARALLEL
	//	Attachment communication policies COPY
		ComPol_CopyAttachment<VertexLayout, AVector3> comPolCopyAPosition(mg, aPosition);

	//	Interface communicators and distributed domain manager
		pcl::InterfaceCommunicator<VertexLayout> com;
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
		GridLayoutMap& glm = dgm.grid_layout_map();
	#endif


/*****************************************
 *
 *	(2) DETERMINE aNumManifoldEdges
 *
 *****************************************/

	CalculateNumManifoldEdgesVertexAttachmentInParentLevel(mg, markSH, aNumManifoldEdges);


/*****************************************
 *
 *	(3) CALCULATE aSmoothBndPosEvenVrt,
 *				  aSmoothBndPosOddVrt
 *
 *****************************************/

//	Calculate aSmoothBndPosEvenVrt, aSmoothBndPosOddVrt
	CalculateSmoothManifoldPosInParentLevelLoopScheme(mg, markSH, linearManifoldSH, aSmoothBndPosEvenVrt, aSmoothBndPosOddVrt, aNumManifoldEdges);


/*****************************************
 *
 *	(4) APPLY
 *
 *****************************************/

//	Loop all vertices of top_level
	for(VertexIterator vrtIter = mg.begin<Vertex>(mg.top_level()); vrtIter != mg.end<Vertex>(mg.top_level()); ++vrtIter)
	{
		Vertex* vrt = *vrtIter;

	//	Catch vertices without parent
		if(mg.get_parent(vrt) == NULL)
			continue;

	//	In case of marked manifold vertices, which do not belong to the user-specified linear boundary manifold subsets,
	//	and activated Loop scheme refinement apply subdivision surfaces smoothing, else linear refinement
		if(markSH.get_subset_index(vrt) != -1 && linearManifoldSH.get_subset_index(vrt) == -1)
		{
		//	EVEN VERTEX
			if(mg.get_parent(vrt)->reference_object_id() == ROID_VERTEX)
			{
			//	Get parent vertex
				Vertex* parentVrt = static_cast<Vertex*>(mg.get_parent(vrt));

				aaPos[vrt] = aaSmoothBndPosEvenVrt[parentVrt];
			}

		//	ODD VERTEX
			else if(mg.get_parent(vrt)->reference_object_id() == ROID_EDGE)
			{
			//	Get parent edge
				Edge* parentEdge = static_cast<Edge*>(mg.get_parent(vrt));

				aaPos[vrt] =  aaSmoothBndPosOddVrt[parentEdge];
			}
		}
	}


/*****************************************
 *
 *	(5) COMMUNICATE VERTICALLY
 *	    AFTER SUBDIVISION SURFACES
 *
 *****************************************/

//	Communicate aPosition in parallel case
	#ifdef UG_PARALLEL
	//	copy ghosts = VMASTER to v_slaves
		com.exchange_data(glm, INT_V_MASTER, INT_V_SLAVE, comPolCopyAPosition);
		com.communicate();
	#endif


/*****************************************
 *
 *	(6) CLEAN UP
 *
 *****************************************/

//	detach vertex attachments
	mg.detach_from_vertices(aNumManifoldEdges);
	mg.detach_from_vertices(aSmoothBndPosEvenVrt);
	mg.detach_from_edges(aSmoothBndPosOddVrt);
}


/// Toplevel vertex repositioning function for subdivision surfaces refinement (Averaging scheme)
/** This function repositions all toplevel manifold vertices to their smoothed positions
 * 	determined by the subdivision surfaces refinement.
 *
 * 	@param mg					reference to MultiGrid
 * 	@param markSH				reference to SubsetHandler markSH containing marked (inner) boundary manifold
 * 	@param linearManifoldSH		reference to user-specified linearManifoldSubsets SubsetHandler
 * 	@param aSmoothBndPos		reference to aSmoothBndPos
 * 	@param aNumManifoldFaces	reference to aNumManifoldFaces
**/
void ApplySmoothManifoldPosToTopLevelAveragingScheme(MultiGrid& mg, MGSubsetHandler& markSH,
													 MGSubsetHandler& linearManifoldSH)
{
/*****************************************
 *
 *	(1) SETUP
 *
 *****************************************/

//	Vertex attachments for associated number of manifold faces and smooth position
	AInt aNumManifoldFaces;
	APosition aSmoothBndPos;

//	attach previously declared vertex attachments with initial value 0
	mg.attach_to_vertices_dv(aNumManifoldFaces, 0);
	mg.attach_to_vertices_dv(aSmoothBndPos, vector3(0, 0, 0));

//	Define attachment accessors
	Grid::VertexAttachmentAccessor<APosition> aaPos(mg, aPosition);
	Grid::VertexAttachmentAccessor<AInt> aaNumManifoldFaces(mg, aNumManifoldFaces);
	Grid::VertexAttachmentAccessor<APosition> aaSmoothBndPos(mg, aSmoothBndPos);

//	Manage vertex attachment communication in parallel case:
//	- Setup communication policy for the above attachment aPosition
//	- Setup interface communicator
//	- Setup distributed grid manager
//	- Setup grid layout map
	#ifdef UG_PARALLEL
	//	Attachment communication policies COPY
		ComPol_CopyAttachment<VertexLayout, AVector3> comPolCopyAPosition(mg, aPosition);

	//	Interface communicators and distributed domain manager
		pcl::InterfaceCommunicator<VertexLayout> com;
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
		GridLayoutMap& glm = dgm.grid_layout_map();
	#endif


/*****************************************
 *
 *	(2) DETERMINE aNumManifoldEdges
 *
 *****************************************/

	CalculateNumManifoldFacesVertexAttachmentInTopLevel(mg, markSH, aNumManifoldFaces);


/*****************************************
 *
 *	(3) CALCULATE
 *
 *****************************************/

//	Calculate aSmoothBndPosEvenVrt, aSmoothBndPosOddVrt
	CalculateSmoothManifoldPosInTopLevelAveragingScheme(mg, markSH, linearManifoldSH, aSmoothBndPos);


/*****************************************
 *
 *	(4) APPLY
 *
 *****************************************/

//	Move manifold vertices to their smoothed position
	for(VertexIterator vrtIter = mg.begin<Vertex>(mg.top_level()); vrtIter != mg.end<Vertex>(mg.top_level()); ++vrtIter)
	{
		Vertex* vrt = *vrtIter;

	//	In case of marked manifold vertices, which do not belong to the user-specified linear boundary manifold subsets,
	//	and activated averaging scheme apply subdivision surfaces smoothing, else linear refinement
		if(markSH.get_subset_index(vrt) != -1 && linearManifoldSH.get_subset_index(vrt) == -1)
		{
			if(aaNumManifoldFaces[vrt] == 0)
				UG_THROW("ERROR in ApplySmoothManifoldPosToTopLevelAveragingScheme: grid contains manifold vertex not contained in any manifold face.");

		//	Scale smooth vertex position by the number of associated volume elements (SubdivisionVolumes smoothing)
			VecScale(aaSmoothBndPos[vrt],  aaSmoothBndPos[vrt], 1.0/aaNumManifoldFaces[vrt]);
			VecScale(aaPos[vrt], aaSmoothBndPos[vrt], 1.0);
		}
	}


/*****************************************
 *
 *	(5) COMMUNICATE VERTICALLY
 *	    AFTER SUBDIVISION SURFACES
 *
 *****************************************/

//	Communicate aPosition in parallel case
	#ifdef UG_PARALLEL
	//	copy v_slaves to ghosts = VMASTER
		com.exchange_data(glm, INT_V_SLAVE, INT_V_MASTER, comPolCopyAPosition);
		com.communicate();
	#endif


/*****************************************
 *
 *	(6) CLEAN UP
 *
 *****************************************/

//	detach vertex attachments
	mg.detach_from_vertices(aNumManifoldFaces);
	mg.detach_from_vertices(aSmoothBndPos);
}


/// Toplevel vertex repositioning function for subdivision volumes refinement
/** This function repositions all toplevel inner vertices to their smoothed positions
 * 	determined by the subdivision volumes refinement.
 *
 * 	@param mg					reference to MultiGrid
 * 	@param markSH				reference to SubsetHandler markSH containing marked (inner) boundary manifold
 * 	@param linearManifoldSH		reference to user-specified linearManifoldSubsets SubsetHandler
 * 	@param aSmoothVolPos		reference to aSmoothVolPos
 * 	@param aNumElems			reference to aNumElems
**/
void ApplySmoothVolumePosToTopLevel(MultiGrid& mg, MGSubsetHandler& markSH,
									MGSubsetHandler& linearManifoldSH)
{
/*****************************************
 *
 *	(1) SETUP
 *
 *****************************************/

//	Vertex attachments for associated number of elements and smooth position
	AInt aNumElems;
	APosition aSmoothVolPos;

//	attach previously declared vertex attachments with initial value 0
	mg.attach_to_vertices_dv(aNumElems, 0);
	mg.attach_to_vertices_dv(aSmoothVolPos, vector3(0, 0, 0));

//	Define attachment accessors
	Grid::VertexAttachmentAccessor<APosition> aaPos(mg, aPosition);
	Grid::VertexAttachmentAccessor<AInt> aaNumElems(mg, aNumElems);
	Grid::VertexAttachmentAccessor<APosition> aaSmoothVolPos(mg, aSmoothVolPos);

//	Manage vertex attachment communication in parallel case:
//	- Setup communication policy for the above attachment aPosition
//	- Setup interface communicator
//	- Setup distributed grid manager
//	- Setup grid layout map
	#ifdef UG_PARALLEL
	//	Attachment communication policies COPY
		ComPol_CopyAttachment<VertexLayout, AVector3> comPolCopyAPosition(mg, aPosition);

	//	Interface communicators and distributed domain manager
		pcl::InterfaceCommunicator<VertexLayout> com;
		DistributedGridManager& dgm = *mg.distributed_grid_manager();
		GridLayoutMap& glm = dgm.grid_layout_map();
	#endif


/*****************************************
 *
 *	(2) DETERMINE aNumElems
 *
 *****************************************/

	CalculateNumElemsVertexAttachmentInTopLevel(mg, aNumElems);


/*****************************************
 *
 *	(3) CALCULATE
 *
 *****************************************/

//	Calculate aSmoothVolPos
	CalculateSmoothVolumePosInTopLevel(mg, markSH, aSmoothVolPos);


/*****************************************
 *
 *	(4) APPLY
 *
 *****************************************/

//	Move vertices to their smoothed position
	for(VertexIterator vrtIter = mg.begin<Vertex>(mg.top_level()); vrtIter != mg.end<Vertex>(mg.top_level()); ++vrtIter)
	{
		Vertex* vrt = *vrtIter;

		if(aaNumElems[vrt] == 0)
			UG_THROW("ERROR in ApplySmoothVolumePosToTopLevel: grid contains vertex not contained in any volume.");

		if(g_boundaryRefinementRule == SUBDIV_VOL)
		{
		//	Scale smooth vertex position by the number of associated volume elements (SubdivisionVolumes smoothing)
			VecScale(aaSmoothVolPos[vrt],  aaSmoothVolPos[vrt], 1.0/aaNumElems[vrt]);
			VecScale(aaPos[vrt], aaSmoothVolPos[vrt], 1.0);
		}
		else
		{
		//	Only in case of inner vertices
			if(markSH.get_subset_index(vrt) == -1)
			{
			//	Scale smooth vertex position by the number of associated volume elements
				VecScale(aaSmoothVolPos[vrt],  aaSmoothVolPos[vrt], 1.0/aaNumElems[vrt]);
				VecScale(aaPos[vrt], aaSmoothVolPos[vrt], 1.0);
			}
		}
	}


/*****************************************
 *
 *	(5) COMMUNICATE VERTICALLY
 *	    AFTER SUBDIVISION VOLUMES
 *
 *****************************************/

//	Communicate aPosition in parallel case
	#ifdef UG_PARALLEL
	//	copy v_slaves to ghosts = VMASTER
		com.exchange_data(glm, INT_V_SLAVE, INT_V_MASTER, comPolCopyAPosition);
		com.communicate();
	#endif


/*****************************************
 *
 *	(6) CLEAN UP
 *
 *****************************************/

//	detach vertex attachments
	mg.detach_from_vertices(aNumElems);
	mg.detach_from_vertices(aSmoothVolPos);
}


/// Function to create a smooth subdivision volumes hierarchy
/** This function transforms a linearly refined hybrid tetra-/octahedral volume
 * 	grid hierarchy into a hierarchy with smoothed boundary manifold
 * 	(s. Schaefer et al, "Smooth subdivision of tetrahedral meshes", 2004)
 *
 * 	@param mg						reference to MultiGrid
 * 	@param sh						reference to standard SubsetHandler
 * 	@param markSH					reference to SubsetHandler containing marked (inner) boundary manifold
 * 	@param linearManifoldSubsets 	user-specified linearManifoldSubsets
**/
void ApplySmoothSubdivisionSurfacesToTopLevel(MultiGrid& mg, MGSubsetHandler& sh, MGSubsetHandler& markSH, const char* linearManifoldSubsets)
{
/*****************************************
 *
 *	(1) SETUP
 *
 *****************************************/

	PROFILE_FUNC_GROUP("subdivision_volumes");

//	Catch use of procedure for MultiGrids with just one level
	if(mg.num_levels() == 1)
	{
		UG_THROW("Error in ApplySmoothSubdivisionToTopLevel: "
				 "Procedure only to be used for MultiGrids with more than one level.");
	}

//	Init linear boundary manifold subsets SubsetHandler from domain and user-specified subsets
	MGSubsetHandler linearManifoldSH(mg);
	InitLinearManifoldSubsetHandler(mg, sh, linearManifoldSH, linearManifoldSubsets);


/*****************************************
 *
 *	(2) SUBDIVISION SURFACES
 *
 *****************************************/

	if(g_boundaryRefinementRule == SUBDIV_SURF_LOOP_SCHEME)
		ApplySmoothManifoldPosToTopLevelLoopScheme(mg, markSH, linearManifoldSH);
	else if(g_boundaryRefinementRule == SUBDIV_SURF_AVERAGING_SCHEME)
		ApplySmoothManifoldPosToTopLevelAveragingScheme(mg, markSH, linearManifoldSH);
	else if(g_boundaryRefinementRule == SUBDIV_VOL){}
	else if(g_boundaryRefinementRule == LINEAR){}
	else
		UG_THROW("ERROR in ApplySubdivisionSurfacesToTopLevel: Unknown boundary refinement rule. Known rules are 'subdiv_surf_loop_scheme', 'subdiv_surf_averaging_scheme' or 'linear'.");
}


/// Function to create a smooth subdivision volumes hierarchy
/** This function transforms a linearly refined hybrid tetra-/octahedral volume
 * 	grid hierarchy into a smoothed subdivision volumes hierarchy
 * 	(s. Schaefer et al, "Smooth subdivision of tetrahedral meshes", 2004)
 *
 * 	@param mg						reference to MultiGrid
 * 	@param sh						reference to standard SubsetHandler
 * 	@param markSH					reference to SubsetHandler containing marked (inner) boundary manifold
 * 	@param linearManifoldSubsets 	user-specified linearManifoldSubsets
**/
void ApplySmoothSubdivisionVolumesToTopLevel(MultiGrid& mg, MGSubsetHandler& sh, MGSubsetHandler& markSH, const char* linearManifoldSubsets)
{
/*****************************************
 *
 *	(1) SETUP
 *
 *****************************************/

	PROFILE_FUNC_GROUP("subdivision_volumes");

//	Ensure, that hybrid tet-/oct refinement is used as refinement rule for tetrahedrons
	if(tet_rules::GetRefinementRule() != tet_rules::HYBRID_TET_OCT)
		UG_THROW("ERROR in ApplySubdivisionVolumesToTopLevel: Set necessary refinement rule by SetTetRefinementRule('hybrid_tet_oct').");

//	Catch use of procedure for MultiGrids with just one level
	if(mg.num_levels() == 1)
	{
		UG_THROW("Error in ApplySmoothSubdivisionToTopLevel: "
				 "Procedure only to be used for MultiGrids with more than one level.");
	}

//	Init linear boundary manifold subsets SubsetHandler from domain and user-specified subsets
	MGSubsetHandler linearManifoldSH(mg);
	InitLinearManifoldSubsetHandler(mg, sh, linearManifoldSH, linearManifoldSubsets);


/*****************************************
 *
 *	(2) SUBDIVISION SURFACES
 *
 *****************************************/

	if(g_boundaryRefinementRule == SUBDIV_SURF_LOOP_SCHEME)
		ApplySmoothManifoldPosToTopLevelLoopScheme(mg, markSH, linearManifoldSH);
	else if(g_boundaryRefinementRule == SUBDIV_SURF_AVERAGING_SCHEME)
		ApplySmoothManifoldPosToTopLevelAveragingScheme(mg, markSH, linearManifoldSH);
	else if(g_boundaryRefinementRule == SUBDIV_VOL){}
	else if(g_boundaryRefinementRule == LINEAR){}
	else
		UG_THROW("ERROR in ApplySubdivisionVolumesToTopLevel: Unknown boundary refinement rule. Known rules are 'subdiv_surf_loop_scheme', 'subdiv_surf_averaging_scheme', 'linear' or 'subdiv_vol'.");


/*****************************************
 *
 *	(3) SUBDIVISION VOLUMES
 *
 *****************************************/

	ApplySmoothVolumePosToTopLevel(mg, markSH, linearManifoldSH);
}


//	OCT-TET subdivision mask
void mask()
{
//	size_t n = 3;
//
//	double lin[n][n][n];
//	double m[2*n-1][2*n-1][2*n-1];
//
//	for(size_t i = 0; i < n; ++i)
//	{
//		for(size_t j = 0; j < n; ++j)
//		{
//			for(size_t k = 0; k < n; ++k)
//			{
//				lin[i][j][k] = 0.0;
//			}
//		}
//	}
//
//	for(size_t i = 0; i < 2*n-1; ++i)
//	{
//		for(size_t j = 0; j < 2*n-1; ++j)
//		{
//			for(size_t k = 0; k < 2*n-1; ++k)
//			{
//				m[i][j][k] = 0.0;
//			}
//		}
//	}
//
//	lin[0][2][0] = 1.0;
//	lin[1][1][0] = 3.0;
//	lin[1][2][0] = 3.0;
//	lin[2][0][0] = 1.0;
//	lin[2][1][0] = 3.0;
//	lin[2][2][0] = 1.0;
//
//	lin[0][1][1] = 3.0;
//	lin[0][2][1] = 3.0;
//	lin[1][0][1] = 3.0;
//	lin[1][1][1] = 6.0;
//	lin[1][2][1] = 3.0;
//	lin[2][0][1] = 3.0;
//	lin[2][1][1] = 3.0;
//
//	lin[0][0][2] = 1.0;
//	lin[0][1][2] = 3.0;
//	lin[0][2][2] = 1.0;
//	lin[1][0][2] = 3.0;
//	lin[1][1][2] = 3.0;
//	lin[2][0][2] = 1.0;
//
//	for(size_t i = 0; i < n; ++i)
//	{
//		for(size_t j = 0; j < n; ++j)
//		{
//			for(size_t k = 0; k < n; ++k)
//			{
//				for(size_t a = 0; a < n; ++a)
//				{
//					for(size_t b = 0; b < n; ++b)
//					{
//						for(size_t c = 0; c < n; ++c)
//						{
//							m[i+a][j+b][k+c] += lin[i][j][k]*lin[a][b][c];
//						}
//					}
//				}
//			}
//		}
//	}
//
//	for(size_t i = 0; i < 2*n-1; ++i)
//	{
//		for(size_t k = 0; k < 2*n-1; ++k)
//		{
//			for(size_t j = 0; j < 2*n-1; ++j)
//			{
//				UG_LOG(m[i][j][k] << "\t ");
//			}
//
//			UG_LOG(" | ");
//		}
//
//		UG_LOG(std::endl);
//	}

//
// -----------------------------------------------------------------------------
//
	const size_t n = 3;

	double lin[n][n];
	double m[2*n-1][2*n-1];

	for(size_t i = 0; i < n; ++i)
	{
		for(size_t j = 0; j < n; ++j)
		{
			lin[i][j] = 0.0;
		}
	}

	for(size_t i = 0; i < 2*n-1; ++i)
	{
		for(size_t j = 0; j < 2*n-1; ++j)
		{
			m[i][j] = 0.0;
		}
	}

	lin[0][1] = 1.0;
	lin[0][2] = 1.0;
	lin[1][0] = 1.0;
	lin[1][1] = 2.0;
	lin[1][2] = 1.0;
	lin[2][0] = 1.0;
	lin[2][1] = 1.0;

	for(size_t i = 0; i < n; ++i)
	{
		for(size_t j = 0; j < n; ++j)
		{
			for(size_t k = 0; k < n; ++k)
			{
				for(size_t l = 0; l < n; ++l)
				{
					m[i+k][j+l] += lin[i][j]*lin[k][l];
				}
			}
		}
	}

	for(size_t i = 0; i < 2*n-1; ++i)
	{
		for(size_t j = 0; j < 2*n-1; ++j)
		{
			UG_LOG(m[i][j] << "\t");
		}

		UG_LOG(std::endl);
	}
}


}//	end of namespace

#endif
