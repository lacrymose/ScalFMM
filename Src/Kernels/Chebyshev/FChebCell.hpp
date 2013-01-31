// ===================================================================================
// Copyright ScalFmm 2011 INRIA, Olivier Coulaud, Bérenger Bramas, Matthias Messner
// olivier.coulaud@inria.fr, berenger.bramas@inria.fr
// This software is a computer program whose purpose is to compute the FMM.
//
// This software is governed by the CeCILL-C and LGPL licenses and
// abiding by the rules of distribution of free software.  
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public and CeCILL-C Licenses for more details.
// "http://www.cecill.info". 
// "http://www.gnu.org/licenses".
// ===================================================================================
#ifndef FCHEBCELL_HPP
#define FCHEBCELL_HPP


#include "../../Extensions/FExtendMortonIndex.hpp"
#include "../../Extensions/FExtendCoordinate.hpp"

#include "./FChebTensor.hpp"

/**
* @author Matthias Messner (matthias.messner@inria.fr)
* @class FChebCell
* Please read the license
*
* This class defines a cell used in the Chebyshev based FMM.
*/
template <int ORDER>
class FChebCell : public FExtendMortonIndex,
									public FExtendCoordinate
{
	FReal multipole_exp[TensorTraits<ORDER>::nnodes * 2]; //< Multipole expansion
	FReal     local_exp[TensorTraits<ORDER>::nnodes * 2]; //< Local expansion
	
public:
	~FChebCell() {}
	
	/** Get Multipole */
	const FReal* getMultipole() const
	{	return this->multipole_exp;	}
	/** Get Local */
	const FReal* getLocal() const
	{	return this->local_exp;	}
	
	/** Get Multipole */
	FReal* getMultipole()
	{	return this->multipole_exp;	}
	/** Get Local */
	FReal* getLocal()
	{	return this->local_exp;	}
	
};


#endif //FCHEBCELL_HPP


