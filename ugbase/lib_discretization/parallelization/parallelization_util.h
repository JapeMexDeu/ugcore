/*
 * parallelization.h
 *
 *  Created on: 21.5.2010
 *      Author: A. Vogel, S.Reiter
 */

#ifndef __H__LIB_DISCRETIZATION__PARALLELIZATION__PARALLELIZATION_UTIL__
#define __H__LIB_DISCRETIZATION__PARALLELIZATION__PARALLELIZATION_UTIL__

#include "lib_algebra/parallelization/parallel_index_layout.h"
#include "lib_discretization/lib_discretization.h"
#include "lib_grid/parallelization/parallelization.h"
#include "lib_discretization/dof_manager/dof_distribution.h"
#include "lib_algebra/parallelization/communication_policies.h"

namespace ug
{

///	Adds dof-indices of elements in elemLayout to the specified IndexLayout.
/**	Make sure that TLayout holds elements of type VertexBase*, EdgeBase*,
 *  Face* or Volume*.
 *
 *  \todo: replace IndexLayout with TDoFManager::IndexLayout.
 */
template <class TDoFDistr, class TLayout>
bool AddEntriesToIndexLayout(IndexLayout& indexLayoutOut,
							TDoFDistr& dofDistr,
							TLayout& elemLayout)
{
	typedef typename TLayout::iterator InterfaceIterator;
	typedef typename TLayout::Interface ElemInterface;
	typedef typename ElemInterface::iterator ElemIterator;

	typedef IndexLayout::Interface IndexInterface;

//	iterate over all interfaces
	for(InterfaceIterator iIter = elemLayout.begin();
		iIter != elemLayout.end(); ++iIter)
	{
		ElemInterface& elemInterface = elemLayout.interface(iIter);
		IndexInterface& indexInterface = indexLayoutOut.interface(
											elemLayout.proc_id(iIter));

	//	iterate over entries in the elemInterface and add associated
	//	dofs to the indexInterface
		for(ElemIterator eIter = elemInterface.begin();
			eIter != elemInterface.end(); ++eIter)
		{
			typename ElemInterface::Element elem = elemInterface.get_element(eIter);
			typename TDoFDistr::algebra_index_vector_type indices;
			dofDistr.get_inner_algebra_indices(elem, indices);
			for(size_t i = 0; i < indices.size(); ++i)
			{
				indexInterface.push_back(indices[i]);
			}
		}
	}
	return true;
}


template <class TDoFDistribution>
bool CreateIndexLayout(	IndexLayout& layoutOut,
						TDoFDistribution& dofDistr,
						GridLayoutMap& layoutMap,
						int keyType, int level)
{
//TODO: clear the layout!
	bool bRetVal = true;
	if(layoutMap.has_layout<VertexBase>(keyType)){
		bRetVal &= AddEntriesToIndexLayout(layoutOut, dofDistr,
								layoutMap.get_layout<VertexBase>(keyType).layout_on_level(level));
	}
/*
	if(layoutMap.has_layout<EdgeBase>(keyType)){
		bRetVal &= AddEntriesToIndexLayout(layoutOut, dofManager,
								layoutMap.get_layout<EdgeBase>(keyType).layout_on_level(level));
	}
	if(layoutMap.has_layout<Face>(keyType)){
		bRetVal &= AddEntriesToIndexLayout(layoutOut, dofManager,
								layoutMap.get_layout<Face>(keyType).layout_on_level(level));
	}
	if(layoutMap.has_layout<Volume>(keyType)){
		bRetVal &= AddEntriesToIndexLayout(layoutOut, dofManager,
								layoutMap.get_layout<Volume>(keyType).layout_on_level(level));
	}
*/
	return bRetVal;
}

template <typename TMatrix, typename TDoFDistr>
void CopyLayoutsAndCommunicatorIntoMatrix(TMatrix& mat, IDoFDistribution<TDoFDistr>& dofDistr)
{
	mat.set_layouts(dofDistr.get_master_layout(), dofDistr.get_slave_layout());

	mat.set_communicator(dofDistr.get_communicator());
	mat.set_process_communicator(dofDistr.get_process_communicator());
}


/**
 *
 */
template <class TDoFDistr, class TLayout>
bool AddEntriesToIndexLayout_DomainDecomposition(
							IndexLayout& processLayoutOut,
							IndexLayout& subdomainLayoutOut,
							TDoFDistr& dofDistr,
							TLayout& elemLayout,
							pcl::IDomainDecompositionInfo* ddInfoIn)
{
	typedef typename TLayout::iterator InterfaceIterator;
	typedef typename TLayout::Interface ElemInterface;
	typedef typename ElemInterface::iterator ElemIterator;

	typedef IndexLayout::Interface IndexInterface;

	int localProc = pcl::GetProcRank();
	int localSubdom = ddInfoIn->map_proc_id_to_subdomain_id(localProc);

//	iterate over all interfaces
	for(InterfaceIterator iIter = elemLayout.begin();
		iIter != elemLayout.end(); ++iIter)
	{
		ElemInterface& elemInterface = elemLayout.interface(iIter);
		int targetProc = elemLayout.proc_id(iIter);
		int targetSubdom = ddInfoIn->map_proc_id_to_subdomain_id(targetProc);

		if(targetSubdom == localSubdom){
		//	create a process interface
			IndexInterface& indexInterface = processLayoutOut.interface(targetProc);

		//	iterate over entries in the elemInterface and add associated
		//	dofs to the indexInterface
			for(ElemIterator eIter = elemInterface.begin();
				eIter != elemInterface.end(); ++eIter)
			{
				typename ElemInterface::Element elem = elemInterface.get_element(eIter);
				typename TDoFDistr::algebra_index_vector_type indices;
				dofDistr.get_inner_algebra_indices(elem, indices);
				for(size_t i = 0; i < indices.size(); ++i)
				{
					indexInterface.push_back(indices[i]);
				}
			}
		}
		else{
		//	create a subdomain interface
			IndexInterface& indexInterface = subdomainLayoutOut.interface(targetProc);

		//	iterate over entries in the elemInterface and add associated
		//	dofs to the indexInterface
			for(ElemIterator eIter = elemInterface.begin();
				eIter != elemInterface.end(); ++eIter)
			{
				typename ElemInterface::Element elem = elemInterface.get_element(eIter);
				typename TDoFDistr::algebra_index_vector_type indices;
				dofDistr.get_inner_algebra_indices(elem, indices);
				for(size_t i = 0; i < indices.size(); ++i)
				{
					indexInterface.push_back(indices[i]);
				}
			}
		}
	}
	return true;
}


template <class TDoFDistribution>
bool CreateIndexLayouts_DomainDecomposition(
						IndexLayout& processLayoutOut,
						IndexLayout& subdomainLayoutOut,
						TDoFDistribution& dofDistr,
						GridLayoutMap& layoutMap,
						int keyType, int level,
						pcl::IDomainDecompositionInfo* ddInfoIn)
{
//TODO: clear the layout!
	bool bRetVal = true;
	if(layoutMap.has_layout<VertexBase>(keyType)){
		bRetVal &= AddEntriesToIndexLayout_DomainDecomposition(
								processLayoutOut,
								subdomainLayoutOut,
								dofDistr,
								layoutMap.get_layout<VertexBase>(keyType).
								layout_on_level(level),
								ddInfoIn); /*(cb_ProcIDToSubdomID)*/
	}
/*
	if(layoutMap.has_layout<EdgeBase>(keyType)){
		bRetVal &= AddEntriesToIndexLayout(layoutOut, dofManager,
								layoutMap.get_layout<EdgeBase>(keyType).layout_on_level(level));
	}
	if(layoutMap.has_layout<Face>(keyType)){
		bRetVal &= AddEntriesToIndexLayout(layoutOut, dofManager,
								layoutMap.get_layout<Face>(keyType).layout_on_level(level));
	}
	if(layoutMap.has_layout<Volume>(keyType)){
		bRetVal &= AddEntriesToIndexLayout(layoutOut, dofManager,
								layoutMap.get_layout<Volume>(keyType).layout_on_level(level));
	}
*/
	return bRetVal;
}

/// returns in a vector all appearencies of an index in a layout
inline void FindPositionInInterfaces(std::vector<std::pair<int, size_t> >& vIndexInterface,
                                     IndexLayout& layout, size_t index)
{
	for(IndexLayout::iterator interface_iter = layout.begin();
				interface_iter != layout.end(); ++interface_iter)
	{
	//	get interface
		IndexLayout::Interface& interface = layout.interface(interface_iter);

		int targetProc   = layout.proc_id(interface_iter);

	//	loop over indices
		int i = 0;
		for( IndexLayout::Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter, ++i)
		{
			size_t currIndex = interface.get_element(iter);

			if(currIndex == index)
				vIndexInterface.push_back(std::pair<int, size_t>(targetProc, i));
		}
	}
}

inline bool AddExtraProcessEntriesToSubdomainLayout(
								size_t numIDs,
								IndexLayout& processMasterLayoutIn,
								IndexLayout& processSlaveLayoutIn,
								IndexLayout& subdomainMasterLayoutInOut,
								IndexLayout& subdomainSlaveLayoutInOut)
{
	std::vector<int> vMultiplicity;
//	generate an id for each entry.
	vMultiplicity.clear();
	vMultiplicity.resize(numIDs, 0);

	int localProc = pcl::GetProcRank();

	for(IndexLayout::iterator interface_iter = processMasterLayoutIn.begin();
			interface_iter != processMasterLayoutIn.end(); ++interface_iter)
	{
	//	get interface
		IndexLayout::Interface& interface = processMasterLayoutIn.interface(interface_iter);
		int targetProc = processMasterLayoutIn.proc_id(interface_iter);

	//	loop over indices
		for( IndexLayout::Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
		{
		//  get index
			const size_t index = interface.get_element(iter);

			UG_LOG("Checking index " << index << std::endl);

			std::vector<std::pair<int, size_t> > vIndexAppear;
			FindPositionInInterfaces(vIndexAppear, subdomainMasterLayoutInOut, index);

			if(vIndexAppear.size() > 0)
			{
			//	 flag
				vMultiplicity[index] = 1;

				UG_LOG("Flagging index=" << index << " on Proc " << localProc << " to target proc "<< targetProc<<std::endl);

			//	add to subdomain interface
			//	get interface
				IndexLayout::Interface& subdomInterface = subdomainMasterLayoutInOut.interface(targetProc);

				subdomInterface.push_back(index);
			}
		}
	}

//	Communicate flagged vector to slaves
	ComPol_VecCopy<std::vector<int> >	copyPol(&vMultiplicity);

	pcl::ParallelCommunicator<IndexLayout> communicator;
	communicator.send_data(processMasterLayoutIn, copyPol);
	communicator.receive_data(processSlaveLayoutIn, copyPol);
	communicator.communicate();

//	add slaves
	for(IndexLayout::iterator interface_iter = processSlaveLayoutIn.begin();
			interface_iter != processSlaveLayoutIn.end(); ++interface_iter)
	{
	//	get interface
		IndexLayout::Interface& interface = processSlaveLayoutIn.interface(interface_iter);
		int targetProc = processSlaveLayoutIn.proc_id(interface_iter);

	//	loop over indices
		for( IndexLayout::Interface::iterator iter = interface.begin();
				iter != interface.end(); ++iter)
		{
		//  get index
			const size_t index = interface.get_element(iter);

		//	is flagged?
			if(vMultiplicity[index] > 0)
			{
			//	add to subdomain interface
			//	get interface
				IndexLayout::Interface& subdomInterface = subdomainSlaveLayoutInOut.interface(targetProc);

				UG_LOG("Adding index " << index << " on Proc "<< localProc<<" to target Proc interface "<<targetProc << "\n");

				subdomInterface.push_back(index);
			}
		}
	}

	return true;
}

}//	end of namespace

#endif
