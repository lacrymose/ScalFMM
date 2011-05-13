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
#include "../Src/Extensions/FExtendForces.hpp"
#include "../Src/Extensions/FExtendPotential.hpp"

#include "../Src/Components/FBasicCell.hpp"
#include "../Src/Fmb/FExtendFmbCell.hpp"

#include "../Src/Core/FFmmAlgorithmThreadProc.hpp"
#include "../Src/Core/FFmmAlgorithm.hpp"

#include "../Src/Components/FSimpleLeaf.hpp"

#include "../Src/Fmb/FFmbKernels.hpp"

#include "../Src/Files/FFmaLoader.hpp"



//#define VALIDATE_FMM

// With openmp : mpicxx -g testFmbAlgorithmProc.cpp ../Src/Utils/FDebug.cpp ../Src/Utils/FTrace.cpp -lgomp -fopenmp -O2 -o testFmbAlgorithmProc.exe
// icpc -openmp -openmp-lib=compat testFmbAlgorithm.cpp ../Src/Utils/FAssertable.cpp ../Src/Utils/FDebug.cpp -O2 -o testFmbAlgorithm.exe

// mpirun -np 3 `eztrace -e ./Tests/Debug/testFmbAlgorithmProc ../Data/testFMAlgorithm.fma.tmp `
// eztrace_convert -o my_paje /tmp/berenger_eztrace_log_rank_0 /tmp/berenger_eztrace_log_rank_1 /tmp/berenger_eztrace_log_rank_2

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
    ///////////////////////////////////////////////////////
    // to extend FAbstractSendable
    ///////////////////////////////////////////////////////
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
        FComplexe*const otherLocal = static_cast<FComplexe*>(buffer);
        for(int idx = 0 ; idx < MultipoleSize ; ++idx){
            local_exp[idx] += otherLocal[idx];
        }
        return bytesToReceiveDown();
    }

    ///////////////////////////////////////////////////////
    // to test equality between good and potentialy bad solution
    ///////////////////////////////////////////////////////
    /** To compare data */
    bool isEqualPole(const FmbCell& other, FReal*const cumul){
        //return memcmp(multipole_exp, other.multipole_exp, sizeof(FComplexe)*MultipoleSize) == 0 &&
        //        memcmp(local_exp, other.local_exp, sizeof(FComplexe)*MultipoleSize) == 0;
        *cumul = 0.0;
        for(int idx = 0; idx < MultipoleSize; ++idx){
            *cumul += FMath::Abs( multipole_exp[idx].getImag() - other.multipole_exp[idx].getImag() );
            *cumul += FMath::Abs( multipole_exp[idx].getReal() - other.multipole_exp[idx].getReal() );
        }

        return *cumul == 0.0;//FMath::LookEqual(cumul,FReal(0.0));
    }

    /** To compare data */
    bool isEqualLocal(const FmbCell& other, FReal*const cumul){
        //return memcmp(multipole_exp, other.multipole_exp, sizeof(FComplexe)*MultipoleSize) == 0 &&
        //        memcmp(local_exp, other.local_exp, sizeof(FComplexe)*MultipoleSize) == 0;
        *cumul = 0.0;
        for(int idx = 0; idx < MultipoleSize; ++idx){
            *cumul += FMath::Abs( local_exp[idx].getImag() - other.local_exp[idx].getImag() );
            *cumul += FMath::Abs( local_exp[idx].getReal() - other.local_exp[idx].getReal() );
        }

        return *cumul < 0.0001;//FMath::LookEqual(cumul,FReal(0.0));
    }
};


#ifdef VALIDATE_FMM
template<template< class ParticleClass, class CellClass, int OctreeHeight> class KernelClass,
        class ParticleClass, class CellClass,
        template<class ParticleClass> class LeafClass,
        int OctreeHeight, int SubtreeHeight>
void ValidateFMMAlgoProc(FOctree<ParticleClass, CellClass, LeafClass, OctreeHeight, SubtreeHeight>* const badTree,
                         FOctree<ParticleClass, CellClass, LeafClass, OctreeHeight, SubtreeHeight>* const valideTree,
                         FFmmAlgorithmThreadProc<FFmbKernels, ParticleClass, CellClass, LeafClass, OctreeHeight, SubtreeHeight>*const fmm){
    std::cout << "Check Result\n";
    {
        typename FOctree<ParticleClass, CellClass,LeafClass, OctreeHeight, SubtreeHeight>::Iterator octreeIterator(badTree);
        octreeIterator.gotoBottomLeft();

        typename FOctree<ParticleClass, CellClass,LeafClass, OctreeHeight, SubtreeHeight>::Iterator octreeIteratorValide(valideTree);
        octreeIteratorValide.gotoBottomLeft();

        for(int level = OctreeHeight - 1 ; level >= 1 ; --level){
            int NbLeafs = 0;
            do{
                ++NbLeafs;
            } while(octreeIterator.moveRight());
            octreeIterator.gotoLeft();

            const int startIdx = fmm->getLeft(NbLeafs);
            const int endIdx = fmm->getRight(NbLeafs);
            // Check that each particle has been summed with all other

            for(int idx = 0 ; idx < startIdx ; ++idx){
                octreeIterator.moveRight();
                octreeIteratorValide.moveRight();
            }

            for(int idx = startIdx ; idx < endIdx ; ++idx){
                if(octreeIterator.getCurrentGlobalIndex() != octreeIteratorValide.getCurrentGlobalIndex()){
                    std::cout << "Error index are not equal!" << std::endl;
                }
                else{
                    FReal cumul;
                    if( !octreeIterator.getCurrentCell()->isEqualPole(*octreeIteratorValide.getCurrentCell(),&cumul) ){
                        std::cout << "Pole Data are different." << idx << " Cumul " << cumul << std::endl;
                    }
                    if( !octreeIterator.getCurrentCell()->isEqualLocal(*octreeIteratorValide.getCurrentCell(),&cumul) ){
                        std::cout << "Local Data are different." << idx << " Cumul " << cumul << std::endl;
                    }
                }

                octreeIterator.moveRight();
                octreeIteratorValide.moveRight();
            }

            octreeIterator.moveUp();
            octreeIterator.gotoLeft();

            octreeIteratorValide.moveUp();
            octreeIteratorValide.gotoLeft();
        }
    }
    {
        int NbLeafs = 0;
        { // Check that each particle has been summed with all other
            typename FOctree<ParticleClass, CellClass,LeafClass, OctreeHeight, SubtreeHeight>::Iterator octreeIterator(badTree);
            octreeIterator.gotoBottomLeft();
            do{
                ++NbLeafs;
            } while(octreeIterator.moveRight());
            std::cout << "There is " << NbLeafs << " Leafs" << std::endl;
        }
        {
            const int startIdx = fmm->getLeft(NbLeafs);
            const int endIdx = fmm->getRight(NbLeafs);
            // Check that each particle has been summed with all other
            typename FOctree<ParticleClass, CellClass,LeafClass, OctreeHeight, SubtreeHeight>::Iterator octreeIterator(badTree);
            octreeIterator.gotoBottomLeft();

            typename FOctree<ParticleClass, CellClass,LeafClass, OctreeHeight, SubtreeHeight>::Iterator octreeIteratorValide(valideTree);
            octreeIteratorValide.gotoBottomLeft();

            for(int idx = 0 ; idx < startIdx ; ++idx){
                octreeIterator.moveRight();
                octreeIteratorValide.moveRight();
            }

            for(int idx = startIdx ; idx < endIdx ; ++idx){
                typename FList<ParticleClass*>::BasicIterator iter(*octreeIterator.getCurrentListTargets());

                typename FList<ParticleClass*>::BasicIterator iterValide(*octreeIteratorValide.getCurrentListTargets());

                if( octreeIterator.getCurrentListSrc()->getSize() != octreeIteratorValide.getCurrentListSrc()->getSize()){
                    std::cout << idx << " Particules numbers is different " << std::endl;
                }
                if( octreeIterator.getCurrentGlobalIndex() != octreeIteratorValide.getCurrentGlobalIndex()){
                    std::cout << idx << " Index are differents " << std::endl;
                }

                while( iter.isValide() && iterValide.isValide() ){
                    // If a particles has been impacted by less than NbPart - 1 (the current particle)
                    // there is a problem

                    if( iterValide.value()->getPotential() != iter.value()->getPotential() ){
                        std::cout << idx << " Potential error : " << iterValide.value()->getPotential()  << " " << iter.value()->getPotential() << "\n";
                    }
                    if( !FMath::LookEqual(iterValide.value()->getForces().getX(),iter.value()->getForces().getX())
                            || !FMath::LookEqual(iterValide.value()->getForces().getY(),iter.value()->getForces().getY())
                            || !FMath::LookEqual(iterValide.value()->getForces().getZ(),iter.value()->getForces().getZ()) ){
                        /*std::cout << idx << " Forces error : " << (iterValide.value()->getForces().getX() - iter.value()->getForces().getX())
                                  << " " << (iterValide.value()->getForces().getY() - iter.value()->getForces().getY())
                                  << " " << (iterValide.value()->getForces().getZ() - iter.value()->getForces().getZ()) << "\n";*/
                        std::cout << idx << " Forces error : x " << iterValide.value()->getForces().getX() << " " << iter.value()->getForces().getX()
                                  << " y " << iterValide.value()->getForces().getY()  << " " << iter.value()->getForces().getY()
                                  << " z " << iterValide.value()->getForces().getZ()  << " " << iter.value()->getForces().getZ() << "\n";
                    }
                    iter.progress();
                    iterValide.progress();
                }

                octreeIterator.moveRight();
                octreeIteratorValide.moveRight();
            }
        }
    }

    std::cout << "Done\n";
}
#endif

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
#ifdef VALIDATE_FMM
    FOctree<FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels> treeValide(loader.getBoxWidth(),loader.getCenterOfBox());
#endif
    // -----------------------------------------------------

    std::cout << "Creating " << loader.getNumberOfParticles() << " particles ..." << std::endl;
    counter.tic();

    FmbParticle*const particles = new FmbParticle[loader.getNumberOfParticles()];
#ifdef VALIDATE_FMM
    FmbParticle*const particlesValide = new FmbParticle[loader.getNumberOfParticles()];
#endif
    for(int idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
        loader.fillParticle(&particles[idxPart]);
#ifdef VALIDATE_FMM
        particlesValide[idxPart] = particles[idxPart];
#endif
    }

    counter.tac();
    std::cout << "Done  " << "(" << counter.elapsed() << "s)." << std::endl;

    // -----------------------------------------------------

    std::cout << "Inserting particles ..." << std::endl;
    counter.tic();
    for(long idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
        tree.insert(&particles[idxPart]);
#ifdef VALIDATE_FMM
        treeValide.insert(&particlesValide[idxPart]);
#endif
    }
    counter.tac();
    std::cout << "Done  " << "(@Inserting Particles = " << counter.elapsed() << "s)." << std::endl;

    // -----------------------------------------------------

    std::cout << "Working on particles ..." << std::endl;
    counter.tic();

    FFmbKernels<FmbParticle, FmbCell, NbLevels> kernels(loader.getBoxWidth());

    FFmmAlgorithmThreadProc<FFmbKernels, FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels> algo(app,&tree,&kernels);
    algo.execute();
#ifdef VALIDATE_FMM
    FFmmAlgorithm<FFmbKernels, FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels> algoValide(&treeValide,&kernels);
    algoValide.execute();
#endif
    counter.tac();
    std::cout << "Done  " << "(@Algorithm = " << counter.elapsed() << "s)." << std::endl;


    { // get sum forces&potential
        FReal potential = 0;
        F3DPosition forces;
#ifdef VALIDATE_FMM
        FReal potentialValide = 0;
        F3DPosition forcesValide;
#endif
        FOctree<FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels>::Iterator octreeIterator(&tree);
        octreeIterator.gotoBottomLeft();
#ifdef VALIDATE_FMM
        FOctree<FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels>::Iterator octreeIteratorValide(&treeValide);
        octreeIteratorValide.gotoBottomLeft();
#endif
        FOctree<FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels>::Iterator countLeafsIterator(octreeIterator);
        int NbLeafs = 0;
        do{
            ++NbLeafs;
        } while(countLeafsIterator.moveRight());

        const int startIdx = algo.getLeft(NbLeafs);
        const int endIdx = algo.getRight(NbLeafs);

        std::cout <<"From " << startIdx << " to " << endIdx << "  NbLeafs is " << NbLeafs << std::endl;

        for(int idxLeaf = 0 ; idxLeaf < startIdx ; ++idxLeaf){
            octreeIterator.moveRight();
#ifdef VALIDATE_FMM
            octreeIteratorValide.moveRight();
#endif
        }

        for(int idxLeaf = startIdx ; idxLeaf < endIdx ; ++idxLeaf){
            FList<FmbParticle*>::ConstBasicIterator iter(*octreeIterator.getCurrentListTargets());
#ifdef VALIDATE_FMM
            FList<FmbParticle*>::ConstBasicIterator iterValide(*octreeIteratorValide.getCurrentListTargets());
#endif
            while( iter.isValide()
#ifdef VALIDATE_FMM
                  && iterValide.isValide()
#endif
                  ){
                potential += iter.value()->getPotential() * iter.value()->getPhysicalValue();
                forces += iter.value()->getForces();
#ifdef VALIDATE_FMM
                potentialValide += iterValide.value()->getPotential() * iterValide.value()->getPhysicalValue();
                forcesValide += iterValide.value()->getForces();
                iterValide.progress();
#endif
                iter.progress();
            }

            octreeIterator.moveRight();
#ifdef VALIDATE_FMM
            octreeIteratorValide.moveRight();
#endif
        }


#ifdef VALIDATE_FMM
        std::cout << "MPI Foces Sum  x = " << forces.getX() << " y = " << forces.getY() << " z = " << forces.getZ() << std::endl;
        std::cout << "Valide Foces Sum  x = " << forcesValide.getX() << " y = " << forcesValide.getY() << " z = " << forcesValide.getZ() << std::endl;
        std::cout << "MPI Potential = " << potential << std::endl;
        std::cout << "Valide Potential = " << potentialValide << std::endl;
#endif
        potential = app.reduceSum(potential);
        forces.setX(app.reduceSum(forces.getX()));
        forces.setY(app.reduceSum(forces.getY()));
        forces.setZ(app.reduceSum(forces.getZ()));
        if(app.isMaster()){
            std::cout << "Foces Sum  x = " << forces.getX() << " y = " << forces.getY() << " z = " << forces.getZ() << std::endl;
            std::cout << "Potential = " << potential << std::endl;
        }
    }

#ifdef VALIDATE_FMM
    ValidateFMMAlgoProc<FFmbKernels, FmbParticle, FmbCell, FSimpleLeaf, NbLevels, SizeSubLevels>(&tree,&treeValide,&algo);
#endif
    // -----------------------------------------------------

    std::cout << "Deleting particles ..." << std::endl;
    counter.tic();
    delete [] particles;
#ifdef VALIDATE_FMM
    delete [] particlesValide;
#endif
    counter.tac();
    std::cout << "Done  " << "(" << counter.elapsed() << "s)." << std::endl;

    // -----------------------------------------------------

    return 0;
}


// [--LICENSE--]
