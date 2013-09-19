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
#ifndef FABSTRACTKERNELS_HPP
#define FABSTRACTKERNELS_HPP


#include "../Utils/FGlobal.hpp"
#include "../Utils/FDebug.hpp"

/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
* @class FAbstractKernels
* @brief
* Please read the license
*
* If you want to create you own kernels you have to inherit from this class.
*/
template< class CellClass, class ContainerClass >
class FAbstractKernels{
public:
    /** Default destructor */
    virtual ~FAbstractKernels(){
    }

    /**
        * P2M
        * particles to multipole
        * @param pole the multipole to fill using the particles
        * @param particles the particles from the same spacial boxe
        */
    virtual void P2M(CellClass* const pole, const ContainerClass* const particles) = 0;

    /**
        * M2M
        * Multipole to multipole
        * @param pole the father (the boxe that contains other ones)
        * @param child the boxe to take values from
        * @param the current computation level
        * the child array has a size of 8 elements (address if exists or 0 otherwise).
        * You must test if a pointer is 0 to know if an element exists inside this array
        */
    virtual void M2M(CellClass* const FRestrict pole, const CellClass*const FRestrict *const FRestrict child, const int inLevel) = 0;

    /**
        * M2L
        * Multipole to local
        * @param local the element to fill using distant neighbors
        * @param distantNeighbors is an array containing fathers's direct neighbors's child - direct neigbors
        * @param size the number of neighbors
        * @param inLevel the current level of the computation
        */
    virtual void M2L(CellClass* const FRestrict local, const CellClass* distantNeighbors[343],
                     const int size, const int inLevel) = 0;


    /** This method can be optionnaly inherited
      * It is called at the end of each computation level during the M2L pass
      * @param level the ending level
      */
    void finishedLevelM2L(const int /*level*/){
    }

    /**
        * L2L
        * Local to local
        * @param local the father to take value from
        * @param child the child to downward values (child may have already been impacted by M2L)
        * @param inLevel the current level of computation
        * the child array has a size of 8 elements (address if exists or 0 otherwise).
        * Children are ordering in the morton index way.
        * You must test if a pointer is 0 to know if an element exists inside this array
        */
    virtual void L2L(const CellClass* const FRestrict local, CellClass* FRestrict * const FRestrict child, const int inLevel) = 0;

    /**
        * L2P
        * Local to particles
        * @param local the leaf element (smaller boxe local element)
        * @param particles the list of particles inside this boxe
        */
    virtual void L2P(const CellClass* const local, ContainerClass* const particles) = 0;

    /**
        * P2P
        * Particles to particles
        * @param inLeafPosition tree coordinate of the leaf
        * @param targets current boxe targets particles
        * @param sources current boxe sources particles (can be == to targets)
        * @param directNeighborsParticles the particles from direct neighbors (this is an array of list)
        * @param size the number of direct neighbors
        */
    virtual void P2P(const FTreeCoordinate& inLeafPosition,
             ContainerClass* const FRestrict targets, const ContainerClass* const FRestrict sources,
             ContainerClass* const directNeighborsParticles[27], const int size) = 0;

    /**
        * P2P
        * Particles to particles
        * @param inLeafPosition tree coordinate of the leaf
        * @param targets current boxe targets particles
        * @param sources current boxe sources particles (can be == to targets)
        * @param directNeighborsParticles the particles from direct neighbors (this is an array of list)
        * @param size the number of direct neighbors
        */
  virtual void P2PRemote(const FTreeCoordinate& /*inLeafPosition*/,
			 ContainerClass* const FRestrict /*targets*/, const ContainerClass* const FRestrict /*sources*/,
                           ContainerClass* const /*directNeighborsParticles*/[27], const int /*size*/) {
        FDEBUG( FDebug::Controller.write("Warning, P2P remote is used but not implemented!").write(FDebug::Flush) );
    }

};


#endif //FABSTRACTKERNELS_HPP


