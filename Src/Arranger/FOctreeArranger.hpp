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
#ifndef FOCTREEARRANGER_HPP
#define FOCTREEARRANGER_HPP

#include "../Utils/FGlobal.hpp"
#include "../Utils/FPoint.hpp"
#include "../Containers/FVector.hpp"
#include "../Utils/FAssert.hpp"

#include "../Utils/FGlobalPeriodic.hpp"
#include "../Utils/FAssert.hpp"
/**
* This example show how to use the FOctreeArranger.
* @example testOctreeRearrange.cpp
*/


/**
* @brief This class is an arranger, it moves the particles that need to be hosted in a different leaf.
*
* For example, if a simulation has been executed and the position
* of the particles have been changed, then it may be better
* to move the particles in the tree instead of building a new
* tree.
*/
template <class OctreeClass, class ContainerClass, class MoverClass >
class FOctreeArranger {
    OctreeClass* const tree; //< The tree to work on

public:
    FReal boxWidth;
    FPoint MinBox;
    FPoint MaxBox;
    MoverClass* interface;

public:
    /** Basic constructor */
    explicit FOctreeArranger(OctreeClass* const inTree) : tree(inTree), boxWidth(tree->getBoxWidth()),
                                                 MinBox(tree->getBoxCenter(),-tree->getBoxWidth()/2),
                                                 MaxBox(tree->getBoxCenter(),tree->getBoxWidth()/2),
                                                 interface(nullptr){
        FAssertLF(tree, "Tree cannot be null" );
        interface = new MoverClass;
    }

    virtual ~FOctreeArranger(){
        delete interface;
    }

    virtual void checkPosition(FPoint& particlePos){
        // Assert
        FAssertLF(   MinBox.getX() < particlePos.getX() && MaxBox.getX() > particlePos.getX()
                  && MinBox.getY() < particlePos.getY() && MaxBox.getY() > particlePos.getY()
                  && MinBox.getZ() < particlePos.getZ() && MaxBox.getZ() > particlePos.getZ());
    }


    void rearrange(){
        typename OctreeClass::Iterator octreeIterator(tree);
        octreeIterator.gotoBottomLeft();
        do{
            const MortonIndex currentMortonIndex = octreeIterator.getCurrentGlobalIndex();
            ContainerClass * particles = octreeIterator.getCurrentLeaf()->getSrc();
            for(int idxPart = 0 ; idxPart < particles->getNbParticles(); /*++idxPart*/){
                FPoint currentPart;
                interface->getParticlePosition(particles,idxPart,&currentPart);
                checkPosition(currentPart);
                const MortonIndex particuleIndex = tree->getMortonFromPosition(currentPart);
                if(particuleIndex != currentMortonIndex){
                    //Need to move this one
                    interface->removeFromLeafAndKeep(particles,currentPart,idxPart);
                }
                else{
                    //Need to increment idx;
                    ++idxPart;
                }
            }
        }while(octreeIterator.moveRight());

        //Insert back the parts that have been removed
        interface->insertAllParticles(tree);

        //Then, remove the empty leaves
        { // Remove empty leaves
            typename OctreeClass::Iterator octreeIterator(tree);
            octreeIterator.gotoBottomLeft();
            bool workOnNext = true;
            do{
                // Empty leaf
                if( octreeIterator.getCurrentListTargets()->getNbParticles() == 0 ){
                    const MortonIndex currentIndex = octreeIterator.getCurrentGlobalIndex();
                    workOnNext = octreeIterator.moveRight();
                    tree->removeLeaf( currentIndex );
                }
                // Not empty, just continue
                else {
                    workOnNext = octreeIterator.moveRight();
                }
            } while( workOnNext );
        }
    }
};

#endif // FOCTREEARRANGER_HPP
