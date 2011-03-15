// /!\ Please, you must read the license at the bottom of this page

#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include <omp.h>

#include "../Sources/Containers/FOctree.hpp"
#include "../Sources/Containers/FList.hpp"

#include "../Sources/Utils/FAssertable.hpp"
#include "../Sources/Utils/F3DPosition.hpp"

#include "../Sources/Core/FBasicParticule.hpp"
#include "../Sources/Core/FBasicCell.hpp"

#include "../Sources/Core/FFMMAlgorithm.hpp"
#include "../Sources/Core/FFmbKernels.hpp"

// We use openmp to count time (portable and easy to manage)
// Compile by : g++ testFMMAlgorithm.cpp ../Sources/Utils/FAssertable.cpp -lgomp -fopenmp -O2 -o testFMMAlgorithm.exe

/** This program show an example of use of
  * the fmm basic algo
  * it also check that eachh particules is little or longer
  * related that each other
  */


/** Custom particule class */
class MyTestParticule : public FBasicParticule{
protected:
    // To store data during downard pass
    long dataDown;
public:
    MyTestParticule(const double inX, const double inY, const double inZ) : FBasicParticule(inX,inY,inZ) {
        this->dataDown = 0;
    }
    MyTestParticule(const F3DPosition& inPos) : FBasicParticule(inPos) {
        this->dataDown = 0;
    }
    MyTestParticule(){
        this->dataDown = 0;
    }
    long getDataDown() const {
        return this->dataDown;
    }
    void setDataDown(const long inData){
        this->dataDown = inData;
    }
};

/** Custom cell */
class MyTestCell : public FBasicCell {
    // To store data during upward and downward pass
    long dataUp, dataDown;
public:
    MyTestCell(const MortonIndex inIndex) : FBasicCell(inIndex) {
        this->dataUp = 0;
        this->dataDown = 0;
    }
    MyTestCell(){
        this->dataUp = 0;
        this->dataDown = 0;
    }
    long getDataUp() const {
        return this->dataUp;
    }
    void setDataUp(const long inData){
        this->dataUp = inData;
    }
    long getDataDown() const {
        return this->dataDown;
    }
    void setDataDown(const long inData){
        this->dataDown = inData;
    }
};

/** Custom Kernel
  * This kernel simply store in each element the number of elements
  * it represents
  */
template< class ParticuleClass, class CellClass>
class MyTestKernels : public FAbstractKernels<ParticuleClass,CellClass> {
public:
    // Before upward
    virtual void P2M(CellClass* const pole, FList<ParticuleClass*>* const particules) {
        // the pole represents all particules under
        pole->setDataUp(particules->getSize());
    }
    // During upward
    virtual void M2M(CellClass* const pole, CellClass** const child) {
        // A parent represents the sum of the child
        for(int idx = 0 ; idx < 8 ; ++idx){
            if(child[idx]){
                pole->setDataUp(pole->getDataUp() + child[idx]->getDataUp());
            }
        }
    }
    // Before Downward
    virtual void M2L(CellClass* const pole, CellClass** const distantNeighbors, const int size) {
        // The pole is impacted by what represent other poles
        for(int idx = 0 ; idx < size ; ++idx){
            pole->setDataDown(pole->getDataDown() + distantNeighbors[idx]->getDataUp());
        }
    }
    // During Downward
    virtual void L2L(CellClass* const local, CellClass** const child) {
        // Each child is impacted by the father
        for(int idx = 0 ; idx < 8 ; ++idx){
            if(child[idx]){
                child[idx]->setDataDown(local->getDataDown() + child[idx]->getDataDown());
            }
        }
    }
    // After Downward
    virtual void L2P(CellClass* const local, FList<ParticuleClass*>* const particules){
        // The particules is impacted by the parent cell
        typename FList<ParticuleClass*>::BasicIterator iter(*particules);
        while( iter.isValide() ){
            iter.value()->setDataDown(iter.value()->getDataDown() + local->getDataDown());
            iter.progress();
        }
    }
    // After Downward
    virtual void P2P(FList<ParticuleClass*>* const currentBox, FList<ParticuleClass*>** directNeighbors, const int size) {
        // Each particules targeted is impacted by the particules sources
        long inc = currentBox->getSize() - 1;
        for(int idx = 0 ; idx < size ; ++idx){
            inc += directNeighbors[idx]->getSize();
        }

        typename FList<ParticuleClass*>::BasicIterator iter(*currentBox);
        while( iter.isValide() ){
            iter.value()->setDataDown(iter.value()->getDataDown() + inc);
            iter.progress();
        }
    }
};

// Simply create particules and try the kernels
int main(int , char ** ){
        const int NbLevels = 10;//10;
        const int NbSubLevels = 3;//3
        const long NbPart = 20000;//2E6;
        const int BoxWidth = 2;//1
        MyTestParticule* particules = new MyTestParticule[NbPart];

        srand ( 1 ); // volontary set seed to constant

        // -----------------------------------------------------
        std::cout << "Creating " << NbPart << " particules ..." << std::endl;
        const double CreatingStartTime = omp_get_wtime();
        for(long idxPart = 0 ; idxPart < NbPart ; ++idxPart){
            new (&particules[idxPart]) MyTestParticule(double(rand())/RAND_MAX,double(rand())/RAND_MAX,double(rand())/RAND_MAX);
        }

        const double CreatingEndTime = omp_get_wtime();
        std::cout << "Done  " << "(" << (CreatingEndTime-CreatingStartTime) << "s)." << std::endl;
        // -----------------------------------------------------

        FOctree<MyTestParticule, MyTestCell, NbLevels, NbSubLevels> tree(BoxWidth,F3DPosition(0.0,0.0,0.0));

        // -----------------------------------------------------
        std::cout << "Inserting particules ..." << std::endl;
        const double InsertingStartTime = omp_get_wtime();
        for(long idxPart = 0 ; idxPart < NbPart ; ++idxPart){
            tree.insert(&particules[idxPart]);
        }
        const double InsertingEndTime = omp_get_wtime();
        std::cout << "Done  " << "(" << (InsertingEndTime-InsertingStartTime) << "s)." << std::endl;
        // -----------------------------------------------------

        std::cout << "Working on particules ..." << std::endl;
        const double WorkingStartTime = omp_get_wtime();

        FFmbKernels<MyTestParticule, MyTestCell> kernels(NbLevels,BoxWidth);
        //FFMMAlgorithm<MyTestParticule, MyTestCell, NbLevels, NbSubLevels> algo(&tree,&kernels);
        //algo.execute();

        const double WorkingEndTime = omp_get_wtime();
        std::cout << "Done  " << "(" << (WorkingEndTime-WorkingStartTime) << "s)." << std::endl;

        // -----------------------------------------------------
        std::cout << "Deleting particules ..." << std::endl;
        const double DeletingStartTime = omp_get_wtime();
        for(long idxPart = 0 ; idxPart < NbPart ; ++idxPart){
            particules[idxPart].~MyTestParticule();
        }
        delete [] particules;
        const double DeletingEndTime = omp_get_wtime();
        std::cout << "Done  " << "(" << (DeletingEndTime-DeletingStartTime) << "s)." << std::endl;
        // -----------------------------------------------------

	return 0;
}


// [--LICENSE--]
