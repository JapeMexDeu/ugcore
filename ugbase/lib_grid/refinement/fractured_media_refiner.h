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

#ifndef __H__UG__fractured_media_refiner__
#define __H__UG__fractured_media_refiner__

#include <queue>
#include "hanging_node_refiner_t.h"

namespace ug
{

/**	This class takes special care for degenerated fractures during refinement.
 * Degenerated faces are refined using anisotropic refinement, so that their
 * degenerated sides are not refined.
 *
 * Currently this only works in 2d.
 *
 * \todo	Add support for degenerated volumes
 * \todo	Use a IsDegenerated callback instead of thresholds
 */
template <class TGrid, class TAPosition>
class FracturedMediaRefiner : public THangingNodeRefiner<TGrid>
{
	typedef THangingNodeRefiner<TGrid>	BaseClass;

	public:
		using BaseClass::mark;

	public:
		FracturedMediaRefiner(SPRefinementProjector projector = SPNULL);
		FracturedMediaRefiner(TGrid& g,
							  SPRefinementProjector projector = SPNULL);

		virtual ~FracturedMediaRefiner();

	///	if the aspect ratio is smaller then the given threshold, the element is considered a fracture element.
		void set_aspect_ratio_threshold(number threshold);

	///	\todo: replace this with a callback
		void set_position_attachment(TAPosition& aPos);

	///	Marks a face for refinement.
	/**	Uses the degenerated-edge-threshold to determine, whether the face
	 * is a fracture-face or not.
	 * Make sure to specify a position attachment before marking any elements.*/
		virtual bool mark(Face* f, RefinementMark refMark = RM_REFINE);

	protected:
		number aspect_ratio(Face* f);
		virtual void collect_objects_for_refine();

	private:
		Grid::VertexAttachmentAccessor<TAPosition>	m_aaPos;
		std::queue<Face*>	m_queDegeneratedFaces;
		number				m_aspectRatioThreshold;
};

}//	end of namespace

#endif
