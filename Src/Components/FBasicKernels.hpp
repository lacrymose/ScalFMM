// ===================================================================================
// Logiciel initial: ScalFmm Version 0.5
// Co-auteurs : Olivier Coulaud, Bérenger Bramas.
// Propriétaires : INRIA.
// Copyright © 2011-2012, diffusé sous les termes et conditions d’une licence propriétaire.
// Initial software: ScalFmm Version 0.5
// Co-authors: Olivier Coulaud, Bérenger Bramas.
// Owners: INRIA.
// Copyright © 2011-2012, spread under the terms and conditions of a proprietary license.
// ===================================================================================
#ifndef FBASICKERNELS_HPP
#define FBASICKERNELS_HPP


#include "FAbstractKernels.hpp"

#include "../Utils/FGlobal.hpp"
#include "../Utils/FTrace.hpp"

/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
* @class AbstractKernels
* @brief
* Please read the license
*
* This kernels simply shows the details of the information
* it receives (in debug)
*/
template< class ParticleClass, class CellClass, class ContainerClass>
class FBasicKernels : public FAbstractKernels<ParticleClass,CellClass,ContainerClass> {
public:
    /** Default destructor */
    virtual ~FBasicKernels(){
    }

    /** Do nothing */
    virtual void P2M(CellClass* const , const ContainerClass* const ) {

    }

    /** Do nothing */
    virtual void M2M(CellClass* const FRestrict , const CellClass*const FRestrict *const FRestrict , const int ) {

    }

    /** Do nothing */
    virtual void M2L(CellClass* const FRestrict , const CellClass* [], const int , const int ) {

    }

    /** Do nothing */
    virtual void L2L(const CellClass* const FRestrict , CellClass* FRestrict *const FRestrict  , const int ) {

    }

    /** Do nothing */
    virtual void L2P(const CellClass* const , ContainerClass* const ){

    }

    /** Do nothing */
    virtual void P2P(ContainerClass* const FRestrict , const ContainerClass* const FRestrict ,
                     const ContainerClass* const [26], const int ) {

    }

    /** Do nothing */
    virtual void P2P(const MortonIndex ,
                     ContainerClass* const FRestrict , const ContainerClass* const FRestrict ,
                     ContainerClass* const [26], const MortonIndex [26], const int ){

    }

    // ------------------- Periodic  --------------------


    /** After Downward */
    void P2P(const MortonIndex ,
             ContainerClass* const FRestrict , const ContainerClass* const FRestrict ,
             ContainerClass* const [26], const FTreeCoordinate [26], const int ) {
    }
};


#endif //FBASICKERNELS_HPP


