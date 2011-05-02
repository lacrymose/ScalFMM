// /!\ Please, you must read the license at the bottom of this page

#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include "../Src/Utils/FTic.hpp"
#include "../Src/Utils/FMpi.hpp"
#include "../Src/Utils/FAbstractSendable.hpp"

#include "../Src/Containers/FOctree.hpp"
#include "../Src/Containers/FList.hpp"

#include "../Src/Components/FFmaParticle.hpp"
#include "../Src/Extenssions/FExtendForces.hpp"
#include "../Src/Extenssions/FExtendPotential.hpp"

#include "../Src/Components/FBasicCell.hpp"
#include "../Src/Fmb/FExtendFmbCell.hpp"

#include "../Src/Core/FFmmAlgorithmThreadProc.hpp"

#include "../Src/Components/FSimpleLeaf.hpp"

#include "../Src/Fmb/FFmbKernels.hpp"

#include "../Src/Files/FFmaLoader.hpp"

// With openmp : g++ testFmbAlgorithm.cpp ../Src/Utils/FAssertable.cpp ../Src/Utils/FDebug.cpp ../Src/Utils/FTrace.cpp -lgomp -fopenmp -O2 -o testFmbAlgorithm.exe
// icpc -openmp -openmp-lib=compat testFmbAlgorithm.cpp ../Src/Utils/FAssertable.cpp ../Src/Utils/FDebug.cpp -O2 -o testFmbAlgorithm.exe

/** This program show an example of use of
  * the fmm basic algo
  * it also check that eachh particles is little or longer
  * related that each other
  */


/** Fmb class has to extend {FExtendForces,FExtendPotential,FExtendPhysicalValue}
  * Because we use fma loader it needs {FFmaParticle}
  */
class FmbParticle : public FFmaParticle, public FExtendForces, public FExtendPotential {
public:
};

/** Custom cell
  *
  */
class FmbCell : public FBasicCell, public FExtendFmbCell , public FAbstractSendable{
public:
    int bytesToSendUp() const{
        return sizeof(FComplexe)*MultipoleSize;
    }
    int writeUp(void* const buffer, const int) const {
        memcpy(buffer,multipole_exp,bytesToSendUp());
        return bytesToSendUp();
    }
    int bytesToReceiveUp() const{
        return sizeof(FComplexe)*MultipoleSize;
    }
    int readUp(void* const buffer, const int) {
        memcpy(multipole_exp,buffer,bytesToSendUp());
        return bytesToReceiveUp();
    }

    int bytesToSendDown() const{
        return sizeof(FComplexe)*MultipoleSize;
    }
    int writeDown(void* const buffer, const int) const {
        memcpy(buffer,local_exp,bytesToSendDown());
        return bytesToSendDown();
    }
    int bytesToReceiveDown() const{
        return sizeof(FComplexe)*MultipoleSize;
    }
    int readDown(void* const buffer, const int) {
        memcpy(local_exp,buffer,bytesToSendDown());
        return bytesToReceiveDown();
    }
};


// Simply create particles and try the kernels
int main(int argc, char ** argv){
    ///////////////////////What we do/////////////////////////////
    std::cout << ">> This executable has to be used to test fmb algorithm.\n";
    //////////////////////////////////////////////////////////////

    FMpi app( argc, argv);

    const int NbLevels = 9;//10;
    const int SizeSubLevels = 3;//3
    FTic counter;
    const char* const defaultFilename = "testLoaderFMA.fma"; //../../Data/ "testLoaderFMA.fma" "testFMAlgorithm.fma" Sphere.fma
    const char* filename;

    if(argc == 1){
        std::cout << "You have to give a .fma file in argument.\n";
        std::cout << "The program will try a default file : " << defaultFilename << "\n";
        filename = defaultFilename;
    }
    else{
        filename = argv[1];
        std::cout << "Opening : " << filename << "\n";
    }

    FFmaLoader<FmbParticle> loader(filename);
    if(!loader.isValide()){
        std::cout << "Loader Error, " << filename << " is missing\n";
        return 1;
    }

    // -----------------------------------------------------

    FOctree<FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels> tree(loader.getBoxWidth(),loader.getCenterOfBox());

    // -----------------------------------------------------

    std::cout << "Creating " << loader.getNumberOfParticles() << " particles ..." << std::endl;
    counter.tic();

    FmbParticle* particles = new FmbParticle[loader.getNumberOfParticles()];

    for(int idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
        loader.fillParticle(&particles[idxPart]);
    }

    counter.tac();
    std::cout << "Done  " << "(" << counter.elapsed() << "s)." << std::endl;

    // -----------------------------------------------------

    std::cout << "Inserting particles ..." << std::endl;
    counter.tic();
    for(long idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
        tree.insert(&particles[idxPart]);
    }
    counter.tac();
    std::cout << "Done  " << "(" << counter.elapsed() << "s)." << std::endl;

    // -----------------------------------------------------

    std::cout << "Working on particles ..." << std::endl;
    counter.tic();

    FFmbKernels<FmbParticle, FmbCell, NbLevels> kernels(loader.getBoxWidth());

    FFmmAlgorithmThreadProc<FFmbKernels, FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels> algo(app,&tree,&kernels);
    algo.execute();

    counter.tac();
    std::cout << "Done  " << "(" << counter.elapsed() << "s)." << std::endl;


    { // get sum forces&potential
        FReal potential = 0;
        F3DPosition forces;
        FOctree<FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels>::Iterator octreeIterator(&tree);
        octreeIterator.gotoBottomLeft();

        FOctree<FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels>::Iterator countLeafsIterator(octreeIterator);
        int NbLeafs = 0;
        do{
            ++NbLeafs;
        } while(countLeafsIterator.moveRight());

        const int startIdx = algo.getLeft(NbLeafs);
        const int endIdx = algo.getRight(NbLeafs);

        for(int idxLeaf = 0 ; idxLeaf < startIdx ; ++idxLeaf){
            octreeIterator.moveRight();
        }

        for(int idxLeaf = startIdx ; idxLeaf < endIdx ; ++idxLeaf){
            FList<FmbParticle*>::ConstBasicIterator iter(*octreeIterator.getCurrentListTargets());
            while( iter.isValide() ){
                potential += iter.value()->getPotential() * iter.value()->getPhysicalValue();
                forces += iter.value()->getForces();

                iter.progress();
            }
            octreeIterator.moveRight();
        }

        potential = app.reduceSum(potential);
        forces.setX(app.reduceSum(forces.getX()));
        forces.setY(app.reduceSum(forces.getY()));
        forces.setZ(app.reduceSum(forces.getZ()));
        if(app.isMaster()){
            std::cout << "Foces Sum  x = " << forces.getX() << " y = " << forces.getY() << " z = " << forces.getZ() << std::endl;
            std::cout << "Potential = " << potential << std::endl;
        }
    }


    // -----------------------------------------------------

    std::cout << "Deleting particles ..." << std::endl;
    counter.tic();
    for(long idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
        particles[idxPart].~FmbParticle();
    }
    delete [] particles;
    counter.tac();
    std::cout << "Done  " << "(" << counter.elapsed() << "s)." << std::endl;

    // -----------------------------------------------------

    return 0;
}


// [--LICENSE--]
