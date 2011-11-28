/*
 * approximation_space_impl.h
 *
 *  Created on: 19.02.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__FUNCTION_SPACE__APPROXIMATION_SPACE_IMPL__
#define __H__UG__LIB_DISC__FUNCTION_SPACE__APPROXIMATION_SPACE_IMPL__

#include "approximation_space.h"
#include "../../common/common.h"

namespace ug{


template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::ApproximationSpace(domain_type& domain)
	: IApproximationSpace<domain_type>(domain), m_bInit(false),
	  m_bLevelDoFInit(false), m_bSurfDoFInit(false),
	  m_MGDoFManager(*(this->m_pMGSH), *this) {};

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
void
ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::init(bool bInitDoFs)
{
//	check if already init
	if(m_bInit) return;

	try{
	//	lock function pattern
		this->lock();

		#ifdef UG_PARALLEL
	//	set distributed grid manager
		try{
			m_MGDoFManager.set_distributed_grid_manager(
					*this->m_pDomain->distributed_grid_manager());
		}UG_CATCH_THROW(" Cannot assign Function Pattern.");
		#endif

	//	init dofs
		try{
			if(bInitDoFs)
				m_MGDoFManager.enable_indices();
		}UG_CATCH_THROW(" Cannot distribute dofs.");
	}
	UG_CATCH_THROW("Cannot init ApproximationSpace");

//	remember init flag
	m_bInit = true;
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
typename ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::function_type*
ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::create_level_function(size_t level)
{
//	init space
	init();

//	enable level dofs
	if(!m_bLevelDoFInit)
	{
		try{
			m_MGDoFManager.enable_level_indices();
		}
		UG_CATCH_THROW("Cannot distribute level dofs.");

		m_bLevelDoFInit = true;
	}

//	get level dof distribution
	dof_distribution_type* dofDistr = m_MGDoFManager.level_dof_distribution(level);

//	check distribution
	if(dofDistr == NULL)
		UG_THROW_FATAL( "ApproximationSpace: No level DoFDistribution created.");

//	create new function
	return new function_type(*this, *dofDistr);
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
typename ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::function_type*
ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::create_surface_function()
{
//	init space
	init();

//	enable surface dofs
	if(!m_bSurfDoFInit)
	{
		try{
			m_MGDoFManager.enable_surface_indices();
		}
		UG_CATCH_THROW("Cannot distribute surface dofs.");

		m_bSurfDoFInit = true;
	}

//	get surface dof distribution
	dof_distribution_type* dofDistr = m_MGDoFManager.surface_dof_distribution();

//	check distribution
	if(dofDistr == NULL)
		UG_THROW_FATAL( "ApproximationSpace: No surface DoFDistribution created.");

//	create new function
	return new function_type(*this, *dofDistr);
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
typename ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::dof_distribution_type&
ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::surface_dof_distribution()
{
//	init space
	init();

//	enable surface dofs
	if(!m_bSurfDoFInit)
	{
		try{
			m_MGDoFManager.enable_surface_indices();
		}
		UG_CATCH_THROW("Cannot distribute surface dofs.");

		m_bSurfDoFInit = true;
	}

	dof_distribution_type* dofDistr = m_MGDoFManager.surface_dof_distribution();

	if(dofDistr == NULL)
		UG_THROW_FATAL( "ApproximationSpace: No surface DoFDistribution created.");

	return *dofDistr;
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
const typename ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::dof_distribution_type&
ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::surface_dof_distribution() const
{
	ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>* This =
			const_cast<ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>*>(this);

	return This->surface_dof_distribution();
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
typename ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::dof_distribution_type&
ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::level_dof_distribution(size_t level)
{
//	init space
	init();

//	enable surface dofs
	if(!m_bLevelDoFInit)
	{
		try{
			m_MGDoFManager.enable_level_indices();
		}
		UG_CATCH_THROW("Cannot distribute level dofs.");

		m_bLevelDoFInit = true;
	}

	return *(m_MGDoFManager.level_dof_distribution(level));
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
const typename ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::dof_distribution_type&
ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>::level_dof_distribution(size_t level) const
{
	ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>* This =
			const_cast<ApproximationSpace<TDomain, TDoFDistribution, TAlgebra>*>(this);

	return This->level_dof_distribution(level);
}

}


#endif /* __H__UG__LIB_DISC__FUNCTION_SPACE__APPROXIMATION_SPACE_IMPL__ */
