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
#ifndef FTESTKERNELS_HPP
#define FTESTKERNELS_HPP


#include <iostream>

#include "FAbstractKernels.hpp"
#include "../Containers/FOctree.hpp"
#include "../Utils/FGlobal.hpp"
#include "../Utils/FTrace.hpp"

/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
* @class AbstractKernels
* @brief
* Please read the license
*
* This kernels is a virtual kernels to validate that the fmm algorithm is
* correctly done on particles.
* It should use FTestCell and FTestParticle.
* A the end of a computation, the particles should host then number of particles
* in the simulation (-1).
*/
template< class ParticleClass, class CellClass, class ContainerClass>
class FTestKernels  : public FAbstractKernels<ParticleClass,CellClass,ContainerClass> {
public:
    /** Default destructor */
    virtual ~FTestKernels(){
    }

    /** Before upward */
    void P2M(CellClass* const pole, const ContainerClass* const particles) {
        // the pole represents all particles under
        pole->setDataUp(pole->getDataUp() + particles->getSize());
    }

    /** During upward */
    void M2M(CellClass* const FRestrict pole, const CellClass *const FRestrict *const FRestrict child, const int /*level*/) {
        // A parent represents the sum of the child
        for(int idx = 0 ; idx < 8 ; ++idx){
            if(child[idx]){
                pole->setDataUp(pole->getDataUp() + child[idx]->getDataUp());
            }
        }
    }

    /** Before Downward */
    void M2L(CellClass* const FRestrict pole, const CellClass* distantNeighbors[343], const int /*size*/, const int /*level*/) {
        // The pole is impacted by what represent other poles
        for(int idx = 0 ; idx < 343 ; ++idx){
            if(distantNeighbors[idx]){
                pole->setDataDown(pole->getDataDown() + distantNeighbors[idx]->getDataUp());
            }
        }
    }

    /** During Downward */
    void L2L(const CellClass*const FRestrict local, CellClass* FRestrict *const FRestrict child, const int /*level*/) {
        // Each child is impacted by the father
        for(int idx = 0 ; idx < 8 ; ++idx){
            if(child[idx]){
                child[idx]->setDataDown(local->getDataDown() + child[idx]->getDataDown());
            }
        }

    }

    /** After Downward */
    void L2P(const CellClass* const  local, ContainerClass*const particles){
        // The particles is impacted by the parent cell
        typename ContainerClass::BasicIterator iter(*particles);
        while( iter.hasNotFinished() ){
            iter.data().setDataDown(iter.data().getDataDown() + local->getDataDown());
            iter.gotoNext();
        }

    }


    /** After Downward */
    void P2P(const FTreeCoordinate& ,
                 ContainerClass* const FRestrict targets, const ContainerClass* const FRestrict sources,
                 ContainerClass* const directNeighborsParticles[27], const int ){
        // Each particles targeted is impacted by the particles sources
        long long int inc = sources->getSize();
        if(targets == sources){
            inc -= 1;
        }
        for(int idx = 0 ; idx < 27 ; ++idx){
            if( directNeighborsParticles[idx] ){
                inc += directNeighborsParticles[idx]->getSize();
            }
        }

        typename ContainerClass::BasicIterator iter(*targets);
        while( iter.hasNotFinished() ){
            iter.data().setDataDown(iter.data().getDataDown() + inc);
            iter.gotoNext();
        }

    }

    /** After Downward */
    void P2PRemote(const FTreeCoordinate& ,
                 ContainerClass* const FRestrict targets, const ContainerClass* const FRestrict sources,
                 ContainerClass* const directNeighborsParticles[27], const int ){
        // Each particles targeted is impacted by the particles sources
        long long int inc = 0;
        for(int idx = 0 ; idx < 27 ; ++idx){
            if( directNeighborsParticles[idx] ){
                inc += directNeighborsParticles[idx]->getSize();
            }
        }

        typename ContainerClass::BasicIterator iter(*targets);
        while( iter.hasNotFinished() ){
            iter.data().setDataDown(iter.data().getDataDown() + inc);
            iter.gotoNext();
        }

    }
};


/** This function test the octree to be sure that the fmm algorithm
  * has worked completly.
  */
template< class OctreeClass, class ParticleClass, class CellClass, class ContainerClass, class LeafClass>
void ValidateFMMAlgo(OctreeClass* const tree){
    std::cout << "Check Result\n";
    const int TreeHeight = tree->getHeight();
    long long int NbPart = 0;
    { // Check that each particle has been summed with all other
        typename OctreeClass::Iterator octreeIterator(tree);
        octreeIterator.gotoBottomLeft();
        do{
            if(octreeIterator.getCurrentCell()->getDataUp() != octreeIterator.getCurrentListSrc()->getSize() ){
                    std::cout << "Problem P2M : " << octreeIterator.getCurrentCell()->getDataUp() <<
                                 " (should be " << octreeIterator.getCurrentListSrc()->getSize() << ")\n";
            }
            NbPart += octreeIterator.getCurrentListSrc()->getSize();
        } while(octreeIterator.moveRight());
    }
    { // Ceck if there is number of NbPart summed at level 1
        typename OctreeClass::Iterator octreeIterator(tree);
        octreeIterator.moveDown();
        long long int res = 0;
        do{
            res += octreeIterator.getCurrentCell()->getDataUp();
        } while(octreeIterator.moveRight());
        if(res != NbPart){
            std::cout << "Problem M2M at level 1 : " << res << "\n";
        }
    }
    { // Ceck if there is number of NbPart summed at level 1
        typename OctreeClass::Iterator octreeIterator(tree);
        octreeIterator.gotoBottomLeft();
        for(int idxLevel = TreeHeight - 1 ; idxLevel > 1 ; --idxLevel ){
            long long int res = 0;
            do{
                res += octreeIterator.getCurrentCell()->getDataUp();
            } while(octreeIterator.moveRight());
            if(res != NbPart){
                std::cout << "Problem M2M at level " << idxLevel << " : " << res << "\n";
            }
            octreeIterator.moveUp();
            octreeIterator.gotoLeft();
        }
    }
    { // Check that each particle has been summed with all other
        typename OctreeClass::Iterator octreeIterator(tree);
        octreeIterator.gotoBottomLeft();
        do{
            typename ContainerClass::BasicIterator iter(*octreeIterator.getCurrentListTargets());

            const bool isUsingTsm = (octreeIterator.getCurrentListTargets() != octreeIterator.getCurrentListSrc());

            while( iter.hasNotFinished() ){
                // If a particles has been impacted by less than NbPart - 1 (the current particle)
                // there is a problem
                if( (!isUsingTsm && iter.data().getDataDown() != NbPart - 1) ||
                    (isUsingTsm && iter.data().getDataDown() != NbPart) ){
                    std::cout << "Problem L2P + P2P : " << iter.data().getDataDown() << "(" << octreeIterator.getCurrentGlobalIndex() << ")\n";
                }
                iter.gotoNext();
            }
        } while(octreeIterator.moveRight());
    }

    std::cout << "Done\n";
}



#endif //FTESTKERNELS_HPP


