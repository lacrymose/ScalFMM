#ifndef FFMMALGORITHMTASK_HPP
#define FFMMALGORITHMTASK_HPP
// /!\ Please, you must read the license at the bottom of this page

#include "../Utils/FAssertable.hpp"
#include "../Utils/FDebug.hpp"
#include "../Utils/FTrace.hpp"
#include "../Utils/FTic.hpp"

#include "../Containers/FOctree.hpp"


/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
* @class FFMMAlgorithmTask
* @brief
* Please read the license
*
* This class is a threaded FMM algorithm
* It just iterates on a tree and call the kernels with good arguments.
* It used openMP task to do so, each calculus is a task
*
* @warning This class does not work with intel compiler (segmentation fault)
*
* Of course this class does not deallocate pointer given in arguements.
*/
template<template< class ParticuleClass, class CellClass, int OctreeHeight> class KernelClass, class ParticuleClass, class CellClass, int OctreeHeight, int SubtreeHeight>
class FFMMAlgorithmTask : protected FAssertable{
    // To reduce the size of variable type based on foctree in this file
    typedef FOctree<ParticuleClass, CellClass, OctreeHeight, SubtreeHeight> Octree;
    typedef typename FOctree<ParticuleClass, CellClass, OctreeHeight, SubtreeHeight>::Iterator FOctreeIterator;

    static const int NbThreads = 4;                                             //< Number of threads (currently a static number)

    Octree* const tree;                                                         //< The octree to work on
    KernelClass<ParticuleClass, CellClass, OctreeHeight>* kernels[NbThreads];   //< The kernels

    FDEBUG(FTic counter);                                                       //< In case of debug: to count the elapsed time

public:	
    /** The constructor need the octree and the kernels used for computation
      * @param inTree the octree to work on
      * @param inKernels the kernels to call
      * An assert is launched if one of the arguments is null
      */
    FFMMAlgorithmTask(Octree* const inTree, KernelClass<ParticuleClass,CellClass,OctreeHeight>* const inKernels)
                      : tree(inTree) {

        assert(tree, "tree cannot be null", __LINE__, __FILE__);
        assert(kernels, "kernels cannot be null", __LINE__, __FILE__);

        for(int idxThread = 0 ; idxThread < NbThreads ; ++idxThread){
            this->kernels[idxThread] = new KernelClass<ParticuleClass, CellClass, OctreeHeight>(*inKernels);
        }

        FDEBUG(FDebug::Controller << "FFMMAlgorithmTask\n");
    }

    /** Default destructor */
    virtual ~FFMMAlgorithmTask(){
        for(int idxThread = 0 ; idxThread < NbThreads ; ++idxThread){
            delete this->kernels[idxThread];
        }
    }

    /**
      * To execute the fmm algorithm
      * Call this function to run the complete algorithm
      */
    void execute(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );

        for(int idxThread = 0 ; idxThread < NbThreads ; ++idxThread){
            this->kernels[idxThread]->init();
        }

        bottomPass();
        upwardPass();

        downardPass();

        directPass();
        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }

    /** P2M */
    void bottomPass(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );
        FDEBUG( FDebug::Controller.write("\tStart Bottom Pass\n").write(FDebug::Flush) );
        FDEBUG( counter.tic() );

        #pragma omp parallel num_threads(NbThreads)
        {
            #pragma omp single
            {
                FOctreeIterator octreeIterator(tree);
                // Iterate on leafs
                octreeIterator.gotoBottomLeft();
                do{
                    #pragma omp task //default(shared) //private(octreeIterator) //untied
                    {
                        // We need the current cell that represent the leaf
                        // and the list of particules
                        kernels[omp_get_thread_num()]->P2M( octreeIterator.getCurrentCell() , octreeIterator.getCurrentList());
                    }
                } while(octreeIterator.moveRight());

            }
        }
        FDEBUG( counter.tac() );
        FDEBUG( FDebug::Controller << "\tFinished ("  << counter.elapsed() << "s)\n" );
        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }

    /** M2M */
    void upwardPass(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );
        FDEBUG( FDebug::Controller.write("\tStart Upward Pass\n").write(FDebug::Flush); );
        FDEBUG( counter.tic() );


        #pragma omp parallel num_threads(NbThreads)
        {
            #pragma omp single
            {
                // Start from leal level - 1
                FOctreeIterator octreeIterator(tree);
                octreeIterator.gotoBottomLeft();
                octreeIterator.moveUp();

                FOctreeIterator avoidGotoLeftIterator(octreeIterator);

                int idxLevel = 0;

                // for each levels
                for(idxLevel = OctreeHeight - 2 ; idxLevel > 1 ; --idxLevel ){
                    // for each cells
                    do{
                        // We need the current cell and the child
                        // child is an array (of 8 child) that may be null
                        #pragma omp task
                        {
                            kernels[omp_get_thread_num()]->M2M( octreeIterator.getCurrentCell() , octreeIterator.getCurrentChild(), idxLevel);
                        }
                    } while(octreeIterator.moveRight());

                    #pragma omp taskwait

                    avoidGotoLeftIterator.moveUp();
                    octreeIterator = avoidGotoLeftIterator;// equal octreeIterator.moveUp(); octreeIterator.gotoLeft();
                }
            }
        }

        FDEBUG( counter.tac() );
        FDEBUG( FDebug::Controller << "\tFinished ("  << counter.elapsed() << "s)\n" );
        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }

    /** M2L L2L */
    void downardPass(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );
        { // first M2L
            FDEBUG( FDebug::Controller.write("\tStart Downward Pass (M2L)\n").write(FDebug::Flush); );
            FDEBUG( counter.tic() );

            #pragma omp parallel num_threads(NbThreads)
            {
                #pragma omp single
                {
                    FOctreeIterator octreeIterator(tree);
                    octreeIterator.moveDown();

                    FOctreeIterator avoidGotoLeftIterator(octreeIterator);

                    // for each levels
                    for(int idxLevel = 2 ; idxLevel < OctreeHeight ; ++idxLevel ){
                        // for each cells
                        do{
                            #pragma omp task
                            {
                                CellClass* neighbors[208];
                                const int counter = tree->getDistantNeighbors(neighbors, octreeIterator.getCurrentGlobalIndex(),idxLevel);
                                if(counter) kernels[omp_get_thread_num()]->M2L( octreeIterator.getCurrentCell() , neighbors, counter, idxLevel);
                            }
                        } while(octreeIterator.moveRight());

                        // not needed #pragma omp taskwait
                        // we can work on several levels at the same time

                        avoidGotoLeftIterator.moveDown();
                        octreeIterator = avoidGotoLeftIterator;
                    }
                }
            }
            FDEBUG( counter.tac() );
            FDEBUG( FDebug::Controller << "\tFinished ("  << counter.elapsed() << "s)\n" );
        }

        { // second L2L
            FDEBUG( FDebug::Controller.write("\tStart Downward Pass (L2L)\n").write(FDebug::Flush); );
            FDEBUG( counter.tic() );
            #pragma omp parallel num_threads(NbThreads)
            {
                #pragma omp single
                {
                    FOctreeIterator octreeIterator(tree);
                    octreeIterator.moveDown();

                    FOctreeIterator avoidGotoLeftIterator(octreeIterator);

                    const int heightMinusOne = OctreeHeight - 1;

                    // for each levels exepted leaf level
                    for(int idxLevel = 2 ; idxLevel < heightMinusOne ; ++idxLevel ){
                        // for each cells
                        do{
                            #pragma omp task
                            {
                                kernels[omp_get_thread_num()]->L2L( octreeIterator.getCurrentCell() , octreeIterator.getCurrentChild(), idxLevel);
                            }
                        } while(octreeIterator.moveRight());

                        #pragma omp taskwait

                        avoidGotoLeftIterator.moveDown();
                        octreeIterator = avoidGotoLeftIterator;
                    }
                }
            }
            FDEBUG( counter.tac() );
            FDEBUG( FDebug::Controller << "\tFinished ("  << counter.elapsed() << "s)\n" );
        }

        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }

    /** P2P */
    void directPass(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );
        FDEBUG( FDebug::Controller.write("\tStart Direct Pass\n").write(FDebug::Flush); );
        FDEBUG( counter.tic() );

        #pragma omp parallel num_threads(NbThreads)
        {
            #pragma omp single
            {
                const int heightMinusOne = OctreeHeight - 1;

                FOctreeIterator octreeIterator(tree);
                octreeIterator.gotoBottomLeft();
                // for each leafs
                do{
                    #pragma omp task
                    {
                        // There is a maximum of 26 neighbors
                        FList<ParticuleClass*>* neighbors[26];
                        kernels[omp_get_thread_num()]->L2P(octreeIterator.getCurrentCell(), octreeIterator.getCurrentList());
                        // need the current particules and neighbors particules
                        const int counter = tree->getLeafsNeighbors(neighbors, octreeIterator.getCurrentGlobalIndex(),heightMinusOne);
                        kernels[omp_get_thread_num()]->P2P( octreeIterator.getCurrentList() , neighbors, counter);
                    }
                } while(octreeIterator.moveRight());
            }
        }
        FDEBUG( counter.tac() );
        FDEBUG( FDebug::Controller << "\tFinished ("  << counter.elapsed() << "s)\n" );
        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }

};


#endif //FFMMALGORITHM_HPP

// [--LICENSE--]
