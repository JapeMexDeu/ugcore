/**
 * \file lapack_lu.h
 *
 * \author Martin Rupp
 *
 * \date 26.11.2009
 *
 * Goethe-Center for Scientific Computing 2010.
 */

#ifndef __H__UG__CPU_ALGEBRA__LAPACK_LU__
#define __H__UG__CPU_ALGEBRA__LAPACK_LU__

#include "algebra_misc.h"
#include "sparsematrix.h"
#include "vector.h"


namespace ug{

/**
 * \brief solves linear equation systems with dense LU decomposition
 *
 */
class LapackLU
{
public:
	LapackLU()
	{
		densemat = NULL;
		interchange = NULL;
	}

	~LapackLU()
	{
		if(densemat) delete[] densemat;
		if(interchange) delete[] interchange;
	}


	template<typename matrix_type>
	bool init(const matrix_type &A);

	template<typename vec_type>
	bool prepare(const vec_type &b, vec_type &x) {return true;}
	template<typename vec_type>
	bool apply(const vec_type &b, vec_type &x);

private:
	size_t size;
	double *densemat;
	int *interchange;
};

} // namespace ug
#include "lapack_lu_impl.h"

#endif /* __H__UG__CPU_ALGEBRA__LAPACK_LU__ */
