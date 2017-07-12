/*
 * Copyright (c) 2012-2015:  G-CSC, Goethe University Frankfurt
 * Author: Andreas Vogel
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
#include "lib_disc/function_spaces/grid_function_util.h"
#include "lib_disc/function_spaces/grid_function_user_data.h"
#include "lib_disc/function_spaces/dof_position_util.h"
#include "lib_disc/function_spaces/grid_function_global_user_data.h"
#include "lib_disc/function_spaces/grid_function_user_data_explicit.h"
#include "lib_disc/function_spaces/grid_function_coordinate_util.h"

using namespace std;

namespace ug{
namespace bridge{
namespace GridFunction{

/**
 * \defgroup gridfnct_bridge Grid Function Bridge
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
	static const int dim = TDomain::dim;
	typedef typename TAlgebra::vector_type vector_type;
	typedef ApproximationSpace<TDomain> approximation_space_type;
	typedef ug::GridFunction<TDomain, TAlgebra> TFct;

//	group string
	grp.append("/ApproximationSpace");

//	GridFunction
	{
		string name = string("GridFunction").append(suffix);
		reg.add_class_<TFct, vector_type>(name, grp)
			.template add_constructor<void (*)(SmartPtr<approximation_space_type>)>("ApproximationSpace")
			.template add_constructor<void (*)(SmartPtr<approximation_space_type>, int)>("ApproximationSpace#Level")
			.add_method("assign", static_cast<void (TFct::*)(const vector_type&)>(&TFct::assign),
						"Success", "Vector")
			.add_method("clone", &TFct::clone)
			.add_method("grid_level", &TFct::grid_level)
			.add_method("num_dofs", static_cast<size_t (TFct::*)() const>(&TFct::num_dofs))
			.add_method("redistribution_enabled", &TFct::redistribution_enabled)
			.add_method("enable_redistribution", &TFct::enable_redistribution)
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "GridFunction", tag);
	}

//	ExplicitGridFunctionValue
	{
		string name = string("ExplicitGridFunctionValue").append(suffix);
		typedef ExplicitGridFunctionValue<TFct> T;
		typedef CplUserData<number, dim> TBase;
		reg.add_class_<T, TBase>(name, grp)
		   .template add_constructor<void (*)(SmartPtr<TFct>, const char*)>("ExplicitGridFunctionValue#Component")
		   .set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "ExplicitGridFunctionValue", tag);
	}

	//	ExplicitGridFunctionGradient
	{
		string name = string("ExplicitGridFunctionGradient").append(suffix);
		typedef ExplicitGridFunctionGradient<TFct> T;
		typedef CplUserData<MathVector<dim>, dim> TBase;
		reg.add_class_<T, TBase>(name, grp)
		   .template add_constructor<void (*)(SmartPtr<TFct>, const char*)>("ExplicitGridFunctionGradient#Component")
            .add_method("add_subset_coeff", &T::add_subset_coeff)
			.add_method("get_subset_coeff", &T::get_subset_coeff)
		   .set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "ExplicitGridFunctionGradient", tag);
	}

//	GridFunctionNumberData
	{
		string name = string("GridFunctionNumberData").append(suffix);
		typedef GridFunctionNumberData<TFct> T;
		typedef CplUserData<number, dim> TBase;
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(SmartPtr<TFct>, const char*)>("GridFunction#Component")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "GridFunctionNumberData", tag);
	}

//	GridFunctionVectorData
	{
		string name = string("GridFunctionVectorData").append(suffix);
		typedef GridFunctionVectorData<TFct> T;
		typedef CplUserData<MathVector<dim>, dim> TBase;
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(SmartPtr<TFct>, const char*)>("GridFunction#Components")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "GridFunctionVectorData", tag);
	}

//	GridFunctionGradientData
	{
		string name = string("GridFunctionGradientData").append(suffix);
		typedef GridFunctionGradientData<TFct> T;
		typedef CplUserData<MathVector<dim>, dim> TBase;
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(SmartPtr<TFct>, const char*)>("GridFunction#Component")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "GridFunctionGradientData", tag);
	}

//	GridFunctionGradientComponentData
	{
		string name = string("GridFunctionGradientComponentData").append(suffix);
		typedef GridFunctionGradientComponentData<TFct> T;
		typedef CplUserData<number, dim> TBase;
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(SmartPtr<TFct>, const char*, size_t)>("GridFunction#Components")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "GridFunctionGradientComponentData", tag);
	}
//	GlobalGridFunctionNumberData
	{
		string name = string("GlobalGridFunctionNumberData").append(suffix);
		typedef GlobalGridFunctionNumberData<TFct> T;
		typedef CplUserData<number, dim> TBase;
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(SmartPtr<TFct>, const char*)>("GridFunction#Component")
			.add_method("evaluate", static_cast<number (T::*)(std::vector<number>)>(&T::evaluate))
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "GlobalGridFunctionNumberData", tag);
	}

//	GlobalGridFunctionNumberData for lower-dim elem geometries (here: edges)
	if (dim > EDGE)
	{
		string name = string("GlobalEdgeGridFunctionNumberData").append(suffix);
		typedef GlobalGridFunctionNumberData<TFct, 1> T;
		typedef CplUserData<number, dim> TBase;
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(SmartPtr<TFct>, const char*)>("GridFunction#Component")
			.add_method("evaluate", static_cast<number (T::*)(std::vector<number>)>(&T::evaluate))
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "GlobalEdgeGridFunctionNumberData", tag);
	}


//	GlobalGridFunctionGradientData
	{
		string name = string("GlobalGridFunctionGradientData").append(suffix);
		typedef GlobalGridFunctionGradientData<TFct> T;
		typedef CplUserData<MathVector<dim>, dim> TBase;
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(SmartPtr<TFct>, const char*)>("GridFunction#Component")
			.add_method("evaluate", static_cast<std::vector<number> (T::*)(std::vector<number>)>(&T::evaluate))
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "GlobalGridFunctionGradientData", tag);
	}

//	AverageFunctionDifference
	{
		string name = string("AverageFunctionDifference");
		typedef ug::GridFunction<TDomain, TAlgebra> grid_function;
		typedef SmartPtr< grid_function > function_pointer;
		reg.add_function(name, static_cast<number (*)(function_pointer, std::string, std::string, std::string)>(&AverageFunctionDifference<TDomain, TAlgebra>), grp);
	}

//	CheckDoFPositions
	{
		reg.add_function("CheckDoFPositions", static_cast<bool (*)(const TFct&)>(CheckDoFPositions<TFct>), grp);
	}

//	AverageFunctionDifference
	{
		typedef ug::GridFunction<TDomain, TAlgebra> GF;
		reg.add_function("AdjustMeanValue", static_cast<void (*)(SmartPtr<GF>, const std::vector<std::string>&, number)>(&AdjustMeanValue<GF>), grp);
		reg.add_function("AdjustMeanValue", static_cast<void (*)(SmartPtr<GF>, const std::vector<std::string>&)>(&AdjustMeanValue<GF>), grp);
		reg.add_function("AdjustMeanValue", static_cast<void (*)(SmartPtr<GF>, const std::string&, number)>(&AdjustMeanValue<GF>), grp);
		reg.add_function("AdjustMeanValue", static_cast<void (*)(SmartPtr<GF>, const std::string&)>(&AdjustMeanValue<GF>), grp);
	}
	
//	SumGFValuesAt
	{
		typedef ug::GridFunction<TDomain, TAlgebra> GF;
		reg.add_function ("SumGFValuesAtVertices", static_cast<number (*) (GF*, const char *, const char *)> (&SumGFValuesAt<GF,Vertex>), grp);
	}
	
//	CheckGFforNaN
	{
		typedef ug::GridFunction<TDomain, TAlgebra> GF;
		reg.add_function ("CheckGFValuesAtVertices", static_cast<bool (*) (const GF*, const char *)> (&CheckGFforNaN<GF,Vertex>), grp);
		reg.add_function ("CheckGFValuesAtEdges", static_cast<bool (*) (const GF*, const char *)> (&CheckGFforNaN<GF,Edge>), grp);
		reg.add_function ("CheckGFValuesAtFaces", static_cast<bool (*) (const GF*, const char *)> (&CheckGFforNaN<GF,Face>), grp);
		reg.add_function ("CheckGFValuesAtVolumes", static_cast<bool (*) (const GF*, const char *)> (&CheckGFforNaN<GF,Volume>), grp);
	}

//	Move Domain by GridFunction
	{
		typedef ug::GridFunction<TDomain, TAlgebra> GF;
		reg.add_function (
			"AddFunctionValuesToGridCoordinatesP1", static_cast<void (*) (SmartPtr<GF>, const char*, size_t)>
				(&AddFunctionValuesToGridCoordinatesP1<GF>), grp);
		reg.add_function (
			"AddFunctionValuesToGridCoordinatesP1", static_cast<void (*) (SmartPtr<GF>, const char*, size_t, number)>
				(&AddFunctionValuesToGridCoordinatesP1<GF>), grp);
	}
}

/**
 * Function called for the registration of Domain dependent parts.
 * All Functions and Classes depending on the Domain
 * are to be placed here when registering. The method is called for all
 * available Domain types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <typename TDomain>
static void Domain(Registry& reg, string grp)
{
	string suffix = GetDomainSuffix<TDomain>();
	string tag = GetDomainTag<TDomain>();

//	group string
	grp.append("/ApproximationSpace");

//  ApproximationSpace
	{
		typedef ApproximationSpace<TDomain> T;
		typedef IApproximationSpace TBase;
		string name = string("ApproximationSpace").append(suffix);
		reg.add_class_<T, TBase>(name, grp)
			.template add_constructor<void (*)(SmartPtr<TDomain>)>("Domain")
			.template add_constructor<void (*)(SmartPtr<TDomain>, const AlgebraType&)>("Domain#AlgebraType")
			.add_method("domain", static_cast<SmartPtr<TDomain> (T::*)()>(&T::domain))
			.add_method("surface_view", static_cast<ConstSmartPtr<SurfaceView> (T::*)() const>(&T::surface_view))
			.add_method("get_dim", &T::get_dim)
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "ApproximationSpace", tag);
	}
}

/**
 * Function called for the registration of Dimension dependent parts.
 * All Functions and Classes depending on the Dimension
 * are to be placed here when registering. The method is called for all
 * available Dimension types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <int dim>
static void Dimension(Registry& reg, string grp)
{
	string suffix = GetDimensionSuffix<dim>();
	string tag = GetDimensionTag<dim>();

}

/**
 * Function called for the registration of Algebra dependent parts.
 * All Functions and Classes depending on Algebra
 * are to be placed here when registering. The method is called for all
 * available Algebra types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <typename TAlgebra>
static void Algebra(Registry& reg, string grp)
{
	string suffix = GetAlgebraSuffix<TAlgebra>();
	string tag = GetAlgebraTag<TAlgebra>();

}

/**
 * Function called for the registration of Domain and Algebra independent parts.
 * All Functions and Classes not depending on Domain and Algebra
 * are to be placed here when registering.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
static void Common(Registry& reg, string grp)
{
//	GridLevel
	reg.add_class_<GridLevel>("GridLevel", grp)
		.add_constructor()
		.add_constructor<void (*)(int)>("Level")
		.add_constructor<void (*)(int, std::string)>("Level#Type")
		.set_construct_as_smart_pointer(true);

//	LFEID
	{
		typedef LFEID T;
		reg.add_class_<T>("LFEID", grp)
			.add_method("order", &T::order)
			.add_method("dim", &T::dim);
	}

//	DoFDistributionInfoProvider
	{
	typedef DoFDistributionInfoProvider T;
	reg.add_class_<T>("DoFDistributionInfoProvider", grp)
		.add_method("print_local_dof_statistic", static_cast<void (T::*)(int) const>(&T::print_local_dof_statistic))
		.add_method("print_local_dof_statistic", static_cast<void (T::*)() const>(&T::print_local_dof_statistic))
		.add_method("num_fct", static_cast<size_t (T::*)() const>(&T::num_fct))
		.add_method("name", &T::name)
		.add_method("names", &T::names)
		.add_method("dim", &T::dim)
		.add_method("lfeid", &T::lfeid);
	}

//	IApproximationSpace
	{
	typedef IApproximationSpace T;
	typedef DoFDistributionInfoProvider TBase;
	reg.add_class_<T, TBase>("IApproximationSpace", grp)
		.add_method("print_statistic", static_cast<void (T::*)(std::string) const>(&T::print_statistic))
		.add_method("print_statistic", static_cast<void (T::*)() const>(&T::print_statistic))
		.add_method("print_layout_statistic", static_cast<void (T::*)() const>(&T::print_layout_statistic))
		.add_method("num_levels", &T::num_levels)
		.add_method("init_levels", &T::init_levels)
		.add_method("init_surfaces", &T::init_surfaces)
		.add_method("init_top_surface", &T::init_top_surface)

		.add_method("clear", &T::clear)
		.add_method("add_fct", static_cast<void (T::*)(const char*, const char*, int, const char*)>(&T::add),
					"", "Name#Type|selection|value=[\"Lagrange\",\"DG\"]#Order#Subsets", "Adds a function to the Function Pattern",
					"currently no help available")
		.add_method("add_fct", static_cast<void (T::*)(const char*, const char*, int)>(&T::add),
					"", "Name#Type|selection|value=[\"Lagrange\",\"DG\"]#Order", "Adds a function to the Function Pattern",
					"currently no help available")
		.add_method("add_fct", static_cast<void (T::*)(const char*, const char*)>(&T::add),
					"", "Name#Type|selection|value=[\"crouzeix-raviart\",\"piecewise-constant\"] ", "Adds a function to the Function Pattern",
					"currently no help available")
		.add_method("add_fct", static_cast<void (T::*)(const std::vector<std::string>&, const char*, int, const std::vector<std::string>&)>(&T::add),
					"", "Name#Type|selection|value=[\"Lagrange\",\"DG\"]#Order#Subsets", "Adds a function to the Function Pattern",
					"currently no help available")
		.add_method("add_fct", static_cast<void (T::*)(const std::vector<std::string>&, const char*, int)>(&T::add),
					"", "Name#Type|selection|value=[\"Lagrange\",\"DG\"]#Order", "Adds a function to the Function Pattern",
					"currently no help available")
		.add_method("add_fct", static_cast<void (T::*)(const std::vector<std::string>&, const char*)>(&T::add),
					"", "Name#Type|selection|value=[\"crouzeix-raviart\",\"piecewise-constant\"]", "Adds a function to the Function Pattern",
					"currently no help available");

	}
}

}; // end Functionality

// end group gridfnct_bridge
/// \}

}// namespace GridFunction

/// \addtogroup gridfnct_bridge
void RegisterBridge_GridFunction(Registry& reg, string grp)
{
	grp.append("/Discretization");
	typedef GridFunction::Functionality Functionality;

	try{
		RegisterCommon<Functionality>(reg,grp);
		//RegisterDimensionDependent<Functionality>(reg,grp);
		RegisterDomainDependent<Functionality>(reg,grp);
		//RegisterAlgebraDependent<Functionality>(reg,grp);
		RegisterDomainAlgebraDependent<Functionality>(reg,grp);
	}
	UG_REGISTRY_CATCH_THROW(grp);
}

}//	end of namespace bridge
}//	end of namespace ug
