/*
 * Copyright (c) 2011-2015:  G-CSC, Goethe University Frankfurt
 * Author: Sebastian Reiter
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

#ifndef __H__UG__HANGING_NODE_REFINER_GRID__
#define __H__UG__HANGING_NODE_REFINER_GRID__

#include "hanging_node_refiner_base.h"

namespace ug
{

///	\addtogroup lib_grid_algorithms_refinement
///	@{

///	Specialization of ug::HangingNodeRefiner for ug::Grid
/**	This class should be used, if hanging node refinement shall be
 * applied on a flat grid (ug::Grid).
 *
 * Marked elements will be replaced by their newly created children.
 *
 * Take a look at ug::HangingNodeRefinerBase for a more in-depth documentation.
 *
 * \sa ug::HangingNodeRefinerBase, ug::HangingNodeRefiner_Grid
 */
class HangingNodeRefiner_Grid : public HangingNodeRefinerBase<Selector>
{
	public:
		typedef HangingNodeRefinerBase<Selector> BaseClass;
		using HangingNodeRefinerBase<Selector>::mark;

	public:
		HangingNodeRefiner_Grid(SPRefinementProjector projector = SPNULL);
		HangingNodeRefiner_Grid(Grid& grid,
								SPRefinementProjector projector = SPNULL);

		virtual ~HangingNodeRefiner_Grid();

		virtual void grid_to_be_destroyed(Grid* grid);

		virtual void assign_grid(Grid& grid);
		virtual Grid* get_associated_grid()		{return m_pGrid;}
		virtual Grid* grid()					{return m_pGrid;}

		virtual bool adaptivity_supported() const	{return true;}
		virtual bool coarsening_supported() const	{return false;}

	///	Marks a vertex for refinement (ignores RM_COARSEN).
		virtual bool mark(Vertex* v, RefinementMark refMark = RM_REFINE);

	///	Marks an edge for refinement (ignores RM_COARSEN).
		virtual bool mark(Edge* e, RefinementMark refMark = RM_REFINE);

	///	Marks a face for refinement (ignores RM_COARSEN).
		virtual bool mark(Face* f, RefinementMark refMark = RM_REFINE);

	///	Marks a volume for refinement (ignores RM_COARSEN).
		virtual bool mark(Volume* v, RefinementMark refMark = RM_REFINE);


		virtual bool local_marks_supported() const 	{return true;}

		virtual void mark_local(Face* f, int localMark);
		virtual void mark_local(Volume* f, int localMark);

		virtual int get_local_mark(Face* f) const;
		virtual int get_local_mark(Volume* f) const;

	protected:
		void attach_local_marks();

	///	returns the number of (globally) marked edges on this level of the hierarchy
		virtual void num_marked_edges_local(std::vector<int>& numMarkedEdgesOut);
	///	returns the number of (globally) marked faces on this level of the hierarchy
		virtual void num_marked_faces_local(std::vector<int>& numMarkedFacesOut);
	///	returns the number of (globally) marked volumes on this level of the hierarchy
		virtual void num_marked_volumes_local(std::vector<int>& numMarkedVolsOut);

		template <class TElem>
		void num_marked_elems(std::vector<int>& numMarkedElemsOut);

	///	performs registration and deregistration at a grid.
	/**	Initializes all grid related variables.
	 *  call set_grid(NULL) to unregister the observer from a grid.
	 *
	 * 	Please note that though the base grid features a set_grid method,
	 *  it is not declared virtual. This is because we want to call it
	 *  during construction and destruction.*/
		void set_grid(Grid* grid);

		virtual void collect_objects_for_refine();

	///	erases unused refined elements
		virtual void post_refine();

		virtual void process_constraining_edge(ConstrainingEdge* cge);
		virtual void refine_edge_with_normal_vertex(Edge* e,
											Vertex** newCornerVrts = NULL);

		virtual void refine_face_with_normal_vertex(Face* f,
											Vertex** newCornerVrts = NULL);
		virtual void process_constraining_face(ConstrainingFace* cgf);

		virtual void refine_volume_with_normal_vertex(Volume* v,
											Vertex** newVolumeVrts = NULL);

	///	Returns the vertex associated with the edge
		virtual Vertex* get_center_vertex(Edge* e);

	///	Associates a vertex with the edge.
		virtual void set_center_vertex(Edge* e, Vertex* v);

	///	Returns the vertex associated with the face
		virtual Vertex* get_center_vertex(Face* f);

	///	Associates a vertex with the face.
		virtual void set_center_vertex(Face* f, Vertex* v);

	private:
		Grid* 			m_pGrid;
		AVertex			m_aVertex;
		AInt			m_aLocalMark;
		Grid::EdgeAttachmentAccessor<AVertex>		m_aaVertexEDGE;
		Grid::FaceAttachmentAccessor<AVertex>		m_aaVertexFACE;
		MultiElementAttachmentAccessor<AInt>		m_aaLocalMark;
};

/// @}	// end of add_to_group command

}//	end of namespace

#endif
