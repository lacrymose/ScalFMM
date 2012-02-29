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

#include <iostream>

#include <cstdio>
#include <cstdlib>

#include "../Src/Utils/FTic.hpp"
#include "../Src/Utils/FParameters.hpp"

#include "../Src/Containers/FOctree.hpp"
#include "../Src/Containers/FVector.hpp"


#include "../Src/Files/FFmaLoader.hpp"
#include "../Src/Files/FTreeIO.hpp"

#include "../Src/Kernels/Spherical/FSphericalCell.hpp"
#include "../Src/Kernels/Spherical/FSphericalParticle.hpp"
#include "../Src/Components/FSimpleLeaf.hpp"



// Simply create particles and try the kernels
int main(int argc, char ** argv){
    typedef FSphericalParticle             ParticleClass;
    typedef FSphericalCell                 CellClass;
    typedef FVector<ParticleClass>         ContainerClass;

    typedef FSimpleLeaf<ParticleClass, ContainerClass >                     LeafClass;
    typedef FOctree<ParticleClass, CellClass, ContainerClass , LeafClass >  OctreeClass;
    ///////////////////////What we do/////////////////////////////
    std::cout << ">> This executable has to be used to compare two trees.\n";
    //////////////////////////////////////////////////////////////
    const int DevP = FParameters::getValue(argc,argv,"-p", 8);

    // -----------------------------------------------------
    CellClass::Init(DevP, true);
    OctreeClass tree1(5, 3, 0, F3DPosition());
    OctreeClass tree2(5, 3, 0, F3DPosition());

    // -----------------------------------------------------
    const char* const filename1 = FParameters::getStr(argc,argv,"-f1", "tree.data");
    const char* const filename2 = FParameters::getStr(argc,argv,"-f1", "dtree.data");
    std::cout << "Compare tree " << filename1 << " and " << filename2 << std::endl;

    FTreeIO::Load<OctreeClass, CellClass, ParticleClass, ContainerClass >(filename1, tree1);
    FTreeIO::Load<OctreeClass, CellClass, ParticleClass, ContainerClass >(filename2, tree2);

    // -----------------------------------------------------
    std::cout << "Check Result\n";
    { // Check that each particle has been summed with all other
        typename OctreeClass::Iterator octreeIterator1(&tree1);
        octreeIterator1.gotoBottomLeft();

        typename OctreeClass::Iterator octreeIterator2(&tree2);
        octreeIterator2.gotoBottomLeft();

        int nbLeaves = 0;

        do{
            if( octreeIterator1.getCurrentGlobalIndex() != octreeIterator2.getCurrentGlobalIndex()){
                std::cout << "Index is different\n";
                break;
            }

            if( octreeIterator1.getCurrentListSrc()->getSize() != octreeIterator2.getCurrentListSrc()->getSize()){
                std::cout << "Number of particles different on leaf " << octreeIterator1.getCurrentGlobalIndex() <<
                             " tree1 " << octreeIterator1.getCurrentListSrc()->getSize() <<
                             " tree2 " << octreeIterator2.getCurrentListSrc()->getSize() << std::endl;
            }

            nbLeaves += 1;

            if( octreeIterator1.moveRight() ){
                if( !octreeIterator2.moveRight() ){
                    std::cout << "Not the same number of leaf, tree2 end before tree1\n";
                    break;
                }
            }
            else {
                if( octreeIterator2.moveRight() ){
                    std::cout << "Not the same number of leaf, tree1 end before tree2\n";
                }
                break;
            }

        } while(true);

        std::cout << "There are " << nbLeaves << " leaves ...\n";
    }
    { // Ceck if there is number of NbPart summed at level 1
        typename OctreeClass::Iterator octreeIterator1(&tree1);
        octreeIterator1.gotoBottomLeft();

        typename OctreeClass::Iterator octreeIterator2(&tree2);
        octreeIterator2.gotoBottomLeft();

        for(int idxLevel = tree1.getHeight() - 1 ; idxLevel > 1 ; --idxLevel ){
            int nbCells = 0;
            if( idxLevel == 2 ){
                do{
                    if( octreeIterator1.getCurrentGlobalIndex() != octreeIterator2.getCurrentGlobalIndex()){
                        std::cout << "Index is different\n";
                        break;
                    }

                    const CellClass*const cell1 = octreeIterator1.getCurrentCell();
                    const CellClass*const cell2 = octreeIterator2.getCurrentCell();

                    FReal cumul = 0;
                    for(int idx = 0; idx < FSphericalCell::GetPoleSize(); ++idx){
                        cumul += FMath::Abs( cell1->getMultipole()[idx].getImag() - cell2->getMultipole()[idx].getImag() );
                        cumul += FMath::Abs( cell1->getMultipole()[idx].getReal() - cell2->getMultipole()[idx].getReal() );
                    }
                    if( cumul > 0.00001 || FMath::IsNan(cumul)){
                        std::cout << "Pole Data are different. Cumul " << cumul << " at level " << idxLevel
                                  << " index is " << octreeIterator1.getCurrentGlobalIndex() << std::endl;
                    }
                    cumul = 0;
                    for(int idx = 0; idx < FSphericalCell::GetLocalSize(); ++idx){
                        cumul += FMath::Abs( cell1->getLocal()[idx].getImag() - cell2->getLocal()[idx].getImag() );
                        cumul += FMath::Abs( cell1->getLocal()[idx].getReal() - cell2->getLocal()[idx].getReal() );
                    }
                    if( cumul > 0.00001 || FMath::IsNan(cumul)){
                        std::cout << "Local Data are different. Cumul " << cumul << " at level " << idxLevel
                                  << " index is " << octreeIterator1.getCurrentGlobalIndex() << std::endl;
                    }

                    nbCells += 1;
                    if( octreeIterator1.moveRight() ){
                        if( !octreeIterator2.moveRight() ){
                            std::cout << "Not the same number of leaf, tree2 end before tree1\n";
                            break;
                        }
                    }
                    else {
                        if( octreeIterator2.moveRight() ){
                            std::cout << "Not the same number of leaf, tree1 end before tree2\n";
                        }
                        break;
                    }
                } while(true);
            }
            octreeIterator1.moveUp();
            octreeIterator1.gotoLeft();

            octreeIterator2.moveUp();
            octreeIterator2.gotoLeft();

            std::cout << "There are " << nbCells << " cells at level " << idxLevel << " ...\n";
        }
    }

    std::cout << "Done\n";

    return 0;
}




