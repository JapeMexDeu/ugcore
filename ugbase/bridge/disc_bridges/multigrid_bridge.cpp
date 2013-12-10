/*
 * multigrid_bridge.cpp
 *
 *  Created on: 22.09.2010
 *      Author: andreasvogel
 */

// extern headers
#include <iostream>
#include <sstream>
#include <string>

// include bridge
#include "bridge/bridge.h"
#include "bridge/util.h"
#include "bridge/util_domain_algebra_dependent.h"

// lib_disc includes
#include "lib_disc/function_spaces/grid_function.h"
#include "lib_disc/function_spaces/approximation_space.h"

#include "lib_disc/operator/linear_operator/std_injection.h"
#include "lib_disc/operator/linear_operator/transfer_post_process.h"
#include "lib_disc/operator/linear_operator/std_transfer.h"
#include "lib_disc/operator/linear_operator/multi_grid_solver/mg_solver.h"
#include "lib_disc/operator/linear_operator/element_gauss_seidel/element_gauss_seidel.h"
#include "lib_disc/operator/linear_operator/element_gauss_seidel/component_gauss_seidel.h"

using namespace std;

namespace ug{
namespace bridge{
namespace MultiGrid{

/**
 * \defgroup multigrid_bridge Multi Grid Bridge
 * \ingroup disc_bridge
 * \{
 */

/**
 * Class exporting the functionality. All functionality that is to
 * be used in scripts or visualization must be registered here.
 */
struct Functionality
{

/**
 * Function called for the registration of Domain and Algebra dependent parts.
 * All Functions and Classes depending on both Domain and Algebra
 * are to be placed here when registering. The method is called for all
 * available Domain and Algebra types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <typename TDomain, typename TAlgebra>
static void DomainAlgebra(Registry& reg, string grp)
{
	string suffix = GetDomainAlgebraSuffix<TDomain,TAlgebra>();
	string tag = GetDomainAlgebraTag<TDomain,TAlgebra>();

//	typedef
	typedef typename TAlgebra::vector_type vector_type;
	typedef ApproximationSpace<TDomain> approximation_space_type;

	grp.append("/MultiGrid");


//	ITransferOperator
	{
		typedef ITransferOperator<TDomain, TAlgebra> T;
		string name = string("ITransferOperator").append(suffix);
		reg.add_class_<T>(name, grp);
		reg.add_class_to_group(name, "ITransferOperator", tag);
	}

//	ITransferPostProcess
	{
		typedef ITransferPostProcess<TDomain, TAlgebra> T;
		string name = string("ITransferPostProcess").append(suffix);
		reg.add_class_<T>(name, grp);
		reg.add_class_to_group(name, "ITransferPostProcess", tag);
	}

//	Standard Transfer
	{
		typedef StdTransfer<TDomain, TAlgebra> T;
		typedef ITransferOperator<TDomain, TAlgebra> TBase;
		string name = string("StdTransfer").append(suffix);
		reg.add_class_<T, TBase>(name, grp)
			.add_constructor()
			.add_method("set_restriction_damping", &T::set_restriction_damping)
			.add_method("add_constraint", &T::add_constraint)
			.add_method("set_debug", &T::set_debug)
			.add_method("set_use_transposed", &T::set_use_transposed)
			.add_method("enable_p1_lagrange_optimization", &T::enable_p1_lagrange_optimization)
			.add_method("p1_lagrange_optimization_enabled", &T::p1_lagrange_optimization_enabled)
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "StdTransfer", tag);
	}

//	Standard Injection
	{
		typedef StdInjection<TDomain, TAlgebra> T;
		typedef ITransferOperator<TDomain, TAlgebra> TBase;
		string name = string("StdInjection").append(suffix);
		reg.add_class_<T, TBase>(name, grp)
			.add_constructor()
			.template add_constructor<void (*)(SmartPtr<approximation_space_type>)>("Approximation Space")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "StdInjection", tag);
	}

//	Average Transfer Post Process
	{
		typedef AverageComponent<TDomain, TAlgebra> T;
		typedef ITransferPostProcess<TDomain, TAlgebra> TBase;
		string name = string("AverageComponent").append(suffix);
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(const std::string&)>("Components")
			.template add_constructor<void (*)(const std::vector<std::string>&)>("Components")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "AverageComponent", tag);
	}

//	AssembledMultiGridCycle
	{
		typedef AssembledMultiGridCycle<TDomain, TAlgebra> T;
		typedef ILinearIterator<vector_type> TBase;
		string name = string("GeometricMultiGrid").append(suffix);
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(SmartPtr<ApproximationSpace<TDomain> >)>("Approximation Space")
			.add_method("set_discretization", &T::set_discretization, "", "Discretization")
			.add_method("set_base_level", &T::set_base_level, "", "Base Level")
			.add_method("set_surface_level", &T::set_surface_level, "", "Surface Level")
			.add_method("set_gathered_base_solver_if_ambiguous", &T::set_gathered_base_solver_if_ambiguous,"", "Specifies if gathered base solver used in case of Ambiguity")
			.add_method("set_base_solver", &T::set_base_solver,"","Base Solver")
			.add_method("set_smoother", &T::set_smoother,"", "Smoother")
			.add_method("set_presmoother", &T::set_presmoother,"", "Smoother")
			.add_method("set_postsmoother", &T::set_postsmoother,"", "Smoother")
			.add_method("set_cycle_type", static_cast<void (T::*)(int)>(&T::set_cycle_type),"", "Cycle Type")
			.add_method("set_cycle_type", static_cast<void (T::*)(const std::string&)>(&T::set_cycle_type),"", "Cycle Type")
			.add_method("set_num_presmooth", &T::set_num_presmooth,"", "Number PreSmooth Steps")
			.add_method("set_num_postsmooth", &T::set_num_postsmooth,"", "Number PostSmooth Steps")
			.add_method("set_transfer", &T::set_transfer,"", "Transfer")
			.add_method("set_prolongation", &T::set_prolongation,"", "Prolongation")
			.add_method("set_restriction", &T::set_restriction,"", "Restriction")
			.add_method("set_projection", &T::set_projection,"", "Projection")
			.add_method("add_prolongation_post_process", &T::add_prolongation_post_process,"", "Prolongation Post Process")
			.add_method("add_restriction_post_process", &T::add_restriction_post_process,"", "Restriction Post Process")
			.add_method("set_debug", &T::set_debug)
			.add_method("set_emulate_full_refined_grid", &T::set_emulate_full_refined_grid)
			.add_method("set_rap", &T::set_rap)
			.add_method("set_smooth_on_surface_rim", &T::set_smooth_on_surface_rim)
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "GeometricMultiGrid", tag);
	}

	//	ElementGaussSeidel
	{
		typedef ElementGaussSeidel<TDomain, TAlgebra> T;
		typedef IPreconditioner<TAlgebra> TBase;
		string name = string("ElementGaussSeidel").append(suffix);
		reg.add_class_<T,TBase>(name, grp, "Vanka Preconditioner")
		.add_constructor()
		.template add_constructor<void (*)(number)>("relax")
		.template add_constructor<void (*)(const std::string&)>("patch_type")
		.template add_constructor<void (*)(number, const std::string&)>("relax#patch_type")
		.add_method("set_relax", &T::set_relax, "", "relax")
		.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "ElementGaussSeidel", tag);
	}

	//	ComponentGaussSeidel
	{
		typedef ComponentGaussSeidel<TDomain, TAlgebra> T;
		typedef IPreconditioner<TAlgebra> TBase;
		string name = string("ComponentGaussSeidel").append(suffix);
		reg.add_class_<T,TBase>(name, grp, "Vanka Preconditioner")
		.template add_constructor<void (*)(const std::string&)>("Cmps")
		.template add_constructor<void (*)(number, const std::string&)>("relax#Cmps")
		.template add_constructor<void (*)(number, const std::string&, const std::vector<int>&, const std::vector<number>&)>("relax#Cmps")
		.add_method("set_relax", &T::set_relax, "", "relax")
		.add_method("set_cmps", &T::set_relax, "", "Cmps")
		.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "ComponentGaussSeidel", tag);
	}

}

};

// end group multigrid_bridge
/// \}

}// namespace MultiGrid

/// \addtogroup multigrid_bridge
void RegisterBridge_MultiGrid(Registry& reg, string grp)
{
	grp.append("/Discretization");
	typedef MultiGrid::Functionality Functionality;

	try{
		RegisterDomainAlgebraDependent<Functionality>(reg,grp);
	}
	UG_REGISTRY_CATCH_THROW(grp);
}

}//	end of namespace ug
}//	end of namespace interface
