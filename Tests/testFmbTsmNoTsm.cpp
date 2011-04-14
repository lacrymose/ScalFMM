// /!\ Please, you must read the license at the bottom of this page

#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include "../Sources/Utils/FTic.hpp"

#include "../Sources/Containers/FOctree.hpp"
#include "../Sources/Containers/FList.hpp"

#include "../Sources/Components/FFmaParticule.hpp"
#include "../Sources/Extenssions/FExtendForces.hpp"
#include "../Sources/Extenssions/FExtendPotential.hpp"

#include "../Sources/Extenssions/FExtendParticuleType.hpp"
#include "../Sources/Extenssions/FExtendCellType.hpp"


#include "../Sources/Components/FBasicCell.hpp"
#include "../Sources/Fmb/FExtendFmbCell.hpp"

#include "../Sources/Core/FFmmAlgorithm.hpp"
#include "../Sources/Core/FFmmAlgorithmTsm.hpp"
#include "../Sources/Core/FFmmAlgorithmArray.hpp"
#include "../Sources/Core/FFmmAlgorithmArrayTsm.hpp"

#include "../Sources/Components/FSimpleLeaf.hpp"
#include "../Sources/Components/FTypedLeaf.hpp"

#include "../Sources/Fmb/FFmbKernelsPotentialForces.hpp"
#include "../Sources/Fmb/FFmbKernelsForces.hpp"
#include "../Sources/Fmb/FFmbKernelsPotential.hpp"


// With openmp : g++ testFmbTsmAlgorithm.cpp ../Sources/Utils/FAssertable.cpp ../Sources/Utils/FDebug.cpp ../Sources/Utils/FTrace.cpp -lgomp -fopenmp -O2 -o testFmbTsmAlgorithm.exe
// icpc -openmp -openmp-lib=compat testFmbTsmAlgorithm.cpp ../Sources/Utils/FAssertable.cpp ../Sources/Utils/FDebug.cpp -O2 -o testFmbTsmAlgorithm.exe

/** This program show an example of use of
  * the fmm basic algo
  * it also check that eachh particules is little or longer
  * related that each other
  */


/** Fmb class has to extend {FExtendForces,FExtendPotential,FExtendPhysicalValue}
  * Because we use fma loader it needs {FFmaParticule}
  */
class FmbParticule : public FFmaParticule, public FExtendForces, public FExtendPotential {
public:
};

class FmbParticuleTyped : public FmbParticule, public FExtendParticuleType {
public:
};

/** Custom cell
  *
  */
class FmbCell : public FBasicCell, public FExtendFmbCell {
public:
};

class FmbCellTyped : public FmbCell, public FExtendCellType {
public:
};

// Simply create particules and try the kernels
int main(int argc, char ** argv){
    ///////////////////////What we do/////////////////////////////
    std::cout << ">> This executable has to be used to test Fmb on a Tsm system.\n";
    std::cout << ">> It compares the results between Tms and no Tms (except P2P & L2P).\n";
    //////////////////////////////////////////////////////////////

    const int NbLevels = 9;//10;
    const int SizeSubLevels = 3;//3
    FTic counter;
    const long NbPart = 200000;//2000000
    const double BoxWidth = 1.0;
    const F3DPosition CenterOfBox(0.5,0.5,0.5);

    // -----------------------------------------------------

    std::cout << "Creating " << NbPart << " particules ..." << std::endl;
    counter.tic();

    FmbParticule* particules = new FmbParticule[NbPart];
    FmbParticuleTyped* particulesTyped = new FmbParticuleTyped[NbPart*2];

    for(int idxPart = 0 ; idxPart < NbPart ; ++idxPart){
        const double x = FReal(rand())/RAND_MAX;
        const double y = FReal(rand())/RAND_MAX;
        const double z = FReal(rand())/RAND_MAX;
        // Particule for standart model
        particules[idxPart].setPosition(x,y,z);
        particules[idxPart].setPhysicalValue(1);

        // Create a clone for typed (Tsm) version
        particulesTyped[idxPart*2].setPosition(x,y,z);
        particulesTyped[idxPart*2+1].setPosition(x,y,z);

        particulesTyped[idxPart*2].setPhysicalValue(1);
        particulesTyped[idxPart*2+1].setPhysicalValue(1);

        particulesTyped[idxPart*2].setAsSource();
        particulesTyped[idxPart*2+1].setAsTarget();
    }

    counter.tac();
    std::cout << "Done  " << "(" << counter.elapsed() << "s)." << std::endl;

    // -----------------------------------------------------
    FOctree<FmbParticule, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels> tree(BoxWidth,CenterOfBox);
    FOctree<FmbParticuleTyped, FmbCellTyped, FTypedLeaf, NbLevels, SizeSubLevels> treeTyped(BoxWidth,CenterOfBox);


    std::cout << "Inserting particules ..." << std::endl;
    counter.tic();
    for(long idxPart = 0 ; idxPart < NbPart ; ++idxPart){
        tree.insert(&particules[idxPart]);
        treeTyped.insert(&particulesTyped[idxPart*2]);
        treeTyped.insert(&particulesTyped[idxPart*2+1]);
    }
    counter.tac();
    std::cout << "Done  " << "(" << counter.elapsed() << "s)." << std::endl;

    // -----------------------------------------------------

    std::cout << "Working on particules ..." << std::endl;
    counter.tic();

    //FFmbKernelsPotentialForces FFmbKernelsForces FFmbKernelsPotential
    FFmbKernelsPotentialForces<FmbParticule, FmbCell, NbLevels> kernels(BoxWidth);
    FFmbKernelsPotentialForces<FmbParticuleTyped, FmbCellTyped, NbLevels> kernelsTyped(BoxWidth);
    //FFmmAlgorithm FFmmAlgorithmArray
    FFmmAlgorithmArray<FFmbKernelsPotentialForces, FmbParticule, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels> algo(&tree,&kernels);
    FFmmAlgorithmArrayTsm<FFmbKernelsPotentialForces, FmbParticuleTyped, FmbCellTyped, FTypedLeaf, NbLevels, SizeSubLevels> algoTyped(&treeTyped,&kernelsTyped);
    algo.execute();
    algoTyped.execute();

    counter.tac();
    std::cout << "Done  " << "(" << counter.elapsed() << "s)." << std::endl;

    // -----------------------------------------------------

    // Here we compare the cells of the trees that must contains the same values

    std::cout << "Start checking ..." << std::endl;
    {
        FOctree<FmbParticule, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels>::Iterator octreeIterator(&tree);
        octreeIterator.gotoBottomLeft();

        FOctree<FmbParticuleTyped, FmbCellTyped, FTypedLeaf, NbLevels, SizeSubLevels>::Iterator octreeIteratorTyped(&treeTyped);
        octreeIteratorTyped.gotoBottomLeft();

        for(int idxLevel = NbLevels - 1 ; idxLevel > 1 ; --idxLevel ){
            std::cout << "\t test level " << idxLevel << "\n";

            do{
                bool poleDiff = false;
                bool localDiff = false;
                for(int idxValues = 0 ; idxValues < FExtendFmbCell::MultipoleSize && !(poleDiff && localDiff); ++idxValues){
                    const FComplexe pole = octreeIterator.getCurrentCell()->getMultipole()[idxValues];
                    const FComplexe poleTyped = octreeIteratorTyped.getCurrentCell()->getMultipole()[idxValues];
                    if(!FMath::LookEqual(pole.getImag(),poleTyped.getImag()) || !FMath::LookEqual(pole.getReal(),poleTyped.getReal())){
                        poleDiff = true;
                        printf("Pole diff imag( %.15e , %.15e ) real( %.15e , %.15e)\n",
                               pole.getImag(),poleTyped.getImag(),pole.getReal(),poleTyped.getReal());
                    }
                    const FComplexe local = octreeIterator.getCurrentCell()->getLocal()[idxValues];
                    const FComplexe localTyped = octreeIteratorTyped.getCurrentCell()->getLocal()[idxValues];
                    if(!FMath::LookEqual(local.getImag(),localTyped.getImag()) || !FMath::LookEqual(local.getReal(),localTyped.getReal())){
                        localDiff = true;
                        printf("Pole diff imag( %.15e , %.15e ) real( %.15e , %.15e)\n",
                               local.getImag(),localTyped.getImag(),local.getReal(),localTyped.getReal());
                    }
                }
                if(poleDiff){
                    std::cout << "Multipole error at level " << idxLevel << "\n";
                }
                if(localDiff){
                    std::cout << "Locale error at level " << idxLevel << "\n";
                }
            } while(octreeIterator.moveRight() && octreeIteratorTyped.moveRight());

            octreeIterator.moveUp();
            octreeIterator.gotoLeft();

            octreeIteratorTyped.moveUp();
            octreeIteratorTyped.gotoLeft();
        }
    }

    std::cout << "Done ..." << std::endl;

    delete [] particules;
    delete [] particulesTyped;

    return 0;
}


// [--LICENSE--]
