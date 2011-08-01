#ifndef FFMMALGORITHMTHREADPROC_HPP
#define FFMMALGORITHMTHREADPROC_HPP
// /!\ Please, you must read the license at the bottom of this page

#include "../Utils/FAssertable.hpp"
#include "../Utils/FDebug.hpp"
#include "../Utils/FTrace.hpp"
#include "../Utils/FTic.hpp"
#include "../Utils/FGlobal.hpp"

#include "../Containers/FBoolArray.hpp"
#include "../Containers/FOctree.hpp"

#include "../Utils/FMpi.hpp"

#include <omp.h>

template <class CellClass>
class FLightOctree {
    class Node {
        char data[sizeof(CellClass)];
        Node* next[8];
    public:
        Node(){
            memset(next, 0, sizeof(Node*)*8);
        }
        virtual ~Node(){
            for(int idxNext = 0 ; idxNext < 8 ; ++idxNext){
                delete next[idxNext];
            }
        }
        void insert(const MortonIndex& index, const CellClass& cell, const int level){
            if(level){
                const int host = (index >> (3 * (level-1))) & 0x07;
                if(!next[host]){
                    next[host] = new Node();
                }
                next[host]->insert(index, cell, level - 1);
            }
            else{
                memcpy(data, &cell, sizeof(CellClass));
            }
        }
        const CellClass* getCell(const MortonIndex& index, const int level) const {
            if(level){
                const int host = (index >> (3 * (level-1))) & 0x07;
                if(next[host]){
                    return next[host]->getCell(index, level - 1);
                }
                return 0;
            }
            else{
                return (CellClass*) data;
            }
        }
    };

    Node root;

public:
    FLightOctree(){
    }

    void insertCell(const MortonIndex& index, const CellClass& cell, const int level){
        root.insert(index, cell, level);
    }

    const CellClass* getCell(const MortonIndex& index, const int level) const{
        return root.getCell(index, level);
    }
};


template<class OctreeClass>
void print(OctreeClass* const valideTree){
    typename OctreeClass::Iterator octreeIterator(valideTree);
    for(int idxLevel = valideTree->getHeight() - 1 ; idxLevel > 1 ; --idxLevel ){
        do{
            std::cout << "[" << octreeIterator.getCurrentGlobalIndex() << "] up:" << octreeIterator.getCurrentCell()->getDataUp() << " down:" << octreeIterator.getCurrentCell()->getDataDown() << "\t";
        } while(octreeIterator.moveRight());
        std::cout << "\n";
        octreeIterator.gotoLeft();
        octreeIterator.moveDown();
    }
}

/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
* @class FFmmAlgorithmThreadProc
* @brief
* Please read the license
*
* This class is a threaded FMM algorithm with mpi.
* It just iterates on a tree and call the kernels with good arguments.
* It used the inspector-executor model :
* iterates on the tree and builds an array to work in parallel on this array
*
* Of course this class does not deallocate pointer given in arguements.
*
* Threaded & based on the inspector-executor model
* schedule(runtime) export OMP_NUM_THREADS=2
* export OMPI_CXX=`which g++-4.4`
* mpirun -np 2 valgrind --suppressions=/usr/share/openmpi/openmpi-valgrind.supp
* --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes
* ./Tests/testFmmAlgorithmProc ../Data/testLoaderSmall.fma.tmp
*/
template<class OctreeClass, class ParticleClass, class CellClass, class ContainerClass, class KernelClass, class LeafClass>
class FFmmAlgorithmThreadProc : protected FAssertable {

    FMpi& app;                               //< The app to communicate

    OctreeClass* const tree;                 //< The octree to work on
    KernelClass** kernels;                   //< The kernels

    typename OctreeClass::Iterator* iterArray;
    int numberOfLeafs;                          //< To store the size at the previous level

    const int MaxThreads;               //< the max number of thread allowed by openmp

    const int nbProcess;                //< Number of process
    const int idProcess;                //< Id of current process

    const int OctreeHeight;

    struct Interval{
        MortonIndex min;
        MortonIndex max;
    };
    Interval*const intervals;
    Interval*const realIntervalsPerLevel;
    Interval*const workingIntervalsPerLevel;


    static void mpiassert(const int test, const unsigned line, const char* const message = 0){
        if(test != MPI_SUCCESS){
            printf("[ERROR] Test failled at line %d, result is %d", line, test);
            if(message) printf(", message: %s",message);
            printf("\n");
            fflush(stdout);
            MPI_Abort(MPI_COMM_WORLD, int(line) );
        }
    }

    enum MpiTags{
        TAG_P2P_PART = 99,
    };

public:
    /** The constructor need the octree and the kernels used for computation
      * @param inTree the octree to work on
      * @param inKernels the kernels to call
      * An assert is launched if one of the arguments is null
      */
    FFmmAlgorithmThreadProc(FMpi& inApp, OctreeClass* const inTree, KernelClass* const inKernels)
            : app(inApp), tree(inTree) , kernels(0), numberOfLeafs(0),
            MaxThreads(omp_get_max_threads()), nbProcess(inApp.processCount()), idProcess(inApp.processId()),
            OctreeHeight(tree->getHeight()),intervals(new Interval[inApp.processCount()]),
            realIntervalsPerLevel(new Interval[inApp.processCount() * tree->getHeight()]),
            workingIntervalsPerLevel(new Interval[inApp.processCount() * tree->getHeight()]){

        fassert(tree, "tree cannot be null", __LINE__, __FILE__);

        this->kernels = new KernelClass*[MaxThreads];
        for(int idxThread = 0 ; idxThread < MaxThreads ; ++idxThread){
            this->kernels[idxThread] = new KernelClass(*inKernels);
        }

        FDEBUG(FDebug::Controller << "FFmmAlgorithmThreadProc\n");
        FDEBUG(FDebug::Controller << "Max threads = "  << MaxThreads << ", Procs = " << nbProcess << ".\n");
    }

    /** Default destructor */
    virtual ~FFmmAlgorithmThreadProc(){
        for(int idxThread = 0 ; idxThread < MaxThreads ; ++idxThread){
            delete this->kernels[idxThread];
        }
        delete [] this->kernels;

        delete [] intervals;
        delete [] realIntervalsPerLevel;
        delete [] workingIntervalsPerLevel;
    }

    /**
      * To execute the fmm algorithm
      * Call this function to run the complete algorithm
      */
    void execute(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );

        // Count leaf
        this->numberOfLeafs = 0;
        typename OctreeClass::Iterator octreeIterator(tree);
        octreeIterator.gotoBottomLeft();
        intervals[idProcess].min = octreeIterator.getCurrentGlobalIndex();
        do{
            ++this->numberOfLeafs;
        } while(octreeIterator.moveRight());
        intervals[idProcess].max = octreeIterator.getCurrentGlobalIndex();

        iterArray = new typename OctreeClass::Iterator[numberOfLeafs];
        fassert(iterArray, "iterArray bad alloc", __LINE__, __FILE__);

        mpiassert( MPI_Allgather( &intervals[idProcess], sizeof(Interval), MPI_BYTE, intervals, sizeof(Interval), MPI_BYTE, MPI_COMM_WORLD),  __LINE__ );
        for(int idxLevel = 0 ; idxLevel < OctreeHeight ; ++idxLevel){
            const int offset = idxLevel * nbProcess;
            for(int idxProc = 0 ; idxProc < nbProcess ; ++idxProc){
                realIntervalsPerLevel[offset + idxProc].max = intervals[idxProc].max >> (3 * (OctreeHeight - idxLevel - 1));
                realIntervalsPerLevel[offset + idxProc].min = intervals[idxProc].min >> (3 * (OctreeHeight - idxLevel - 1));
            }

            workingIntervalsPerLevel[offset + 0] = realIntervalsPerLevel[offset + 0];
            for(int idxProc = 1 ; idxProc < nbProcess ; ++idxProc){
                workingIntervalsPerLevel[offset + idxProc].min = FMath::Max( realIntervalsPerLevel[offset + idxProc].min,
                                                                          realIntervalsPerLevel[offset + idxProc - 1].max + 1);
                workingIntervalsPerLevel[offset + idxProc].max = realIntervalsPerLevel[offset + idxProc].max;
            }
        }

        // run;
        bottomPass();

        upwardPass();

        downardPass();

        directPass();

        // delete array
        delete [] iterArray;
        iterArray = 0;

        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }


    /////////////////////////////////////////////////////////////////////////////
    // Utils functions
    /////////////////////////////////////////////////////////////////////////////

    int getLeft(const int inSize) const {
        const float step = (float(inSize) / nbProcess);
        return int(FMath::Ceil(step * idProcess));
    }

    int getRight(const int inSize) const {
        const float step = (float(inSize) / nbProcess);
        const int res = int(FMath::Ceil(step * (idProcess+1)));
        if(res > inSize) return inSize;
        else return res;
    }

    int getOtherRight(const int inSize,const int other) const {
        const float step = (float(inSize) / nbProcess);
        const int res = int(FMath::Ceil(step * (other+1)));
        if(res > inSize) return inSize;
        else return res;
    }

    int getProc(const int position, const int inSize) const {
        const float step = (float(inSize) / nbProcess);
        return int(position/step);
    }

    /////////////////////////////////////////////////////////////////////////////
    // P2M
    /////////////////////////////////////////////////////////////////////////////

    /** P2M Bottom Pass */
    void bottomPass(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );
        FDEBUG( FDebug::Controller.write("\tStart Bottom Pass\n").write(FDebug::Flush) );
        FDEBUG(FTic counterTime);

        typename OctreeClass::Iterator octreeIterator(tree);

        // Iterate on leafs
        octreeIterator.gotoBottomLeft();
        int leafs = 0;
        do{
            iterArray[leafs++] = octreeIterator;
        } while(octreeIterator.moveRight());

        FDEBUG(FTic computationCounter);
        #pragma omp parallel
        {
            KernelClass * const myThreadkernels = kernels[omp_get_thread_num()];
            #pragma omp for nowait
            for(int idxLeafs = 0 ; idxLeafs < leafs ; ++idxLeafs){
                // We need the current cell that represent the leaf
                // and the list of particles
                myThreadkernels->P2M( iterArray[idxLeafs].getCurrentCell() , iterArray[idxLeafs].getCurrentListSrc());
            }
        }
        FDEBUG(computationCounter.tac());


        FDEBUG( FDebug::Controller << "\tFinished (@Bottom Pass (P2M) = "  << counterTime.tacAndElapsed() << "s)\n" );
        FDEBUG( FDebug::Controller << "\t\t Computation : " << computationCounter.elapsed() << " s\n" );
        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }

    /////////////////////////////////////////////////////////////////////////////
    // Upward
    /////////////////////////////////////////////////////////////////////////////

    /** M2M */
    void upwardPass(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );
        FDEBUG( FDebug::Controller.write("\tStart Upward Pass\n").write(FDebug::Flush); );
        FDEBUG(FTic counterTime);
        FDEBUG(FTic computationCounter);
        FDEBUG(FTic sendCounter);
        FDEBUG(FTic receiveCounter);

        // Start from leal level - 1
        typename OctreeClass::Iterator octreeIterator(tree);
        octreeIterator.gotoBottomLeft();
        octreeIterator.moveUp();
        typename OctreeClass::Iterator avoidGotoLeftIterator(octreeIterator);

        int sendToProc = idProcess;

        MPI_Request requests[14];

        KernelClass& myThreadkernels = (*kernels[omp_get_thread_num()]);

        const int recvBufferOffset = 8 * sizeof(CellClass) + 1;
        char sendBuffer[recvBufferOffset];
        char recvBuffer[nbProcess * recvBufferOffset];

        // for each levels
        for(int idxLevel = OctreeHeight - 2 ; idxLevel > 1 ; --idxLevel ){
            // We do not touche cells that we are not responsible
            printf("at level %d I go from %lld to %lld and but my data goes from %lld to %lld\n",
                   idxLevel,workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].min,
                   workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].max ,
                   realIntervalsPerLevel[idxLevel  * nbProcess + idProcess].min,
                   realIntervalsPerLevel[idxLevel  * nbProcess + idProcess].max);

            // copy cells to work with
            int numberOfCells = 0;
            // for each cells
            do{
                iterArray[numberOfCells++] = octreeIterator;
            } while(octreeIterator.moveRight());
            avoidGotoLeftIterator.moveUp();
            octreeIterator = avoidGotoLeftIterator;

            int iterRequests = 0;
            bool cellsToSend = false;

            // find cells to send
            if(idProcess != 0
                    && iterArray[0].getCurrentGlobalIndex() < workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].min){
                cellsToSend = true;
                char state = 0;
                int idxBuff = 1;

                const CellClass* const* const child = iterArray[0].getCurrentChild();
                for(int idxChild = 0 ; idxChild < 8 ; ++idxChild){
                    if( child[idxChild] && workingIntervalsPerLevel[(idxLevel+1) * nbProcess + idProcess].min <= child[idxChild]->getMortonIndex() ){
                        memcpy(&sendBuffer[idxBuff], child[idxChild], sizeof(CellClass));
                        idxBuff += sizeof(CellClass);
                        state |= (0x1 << idxChild);
                    }
                }
                sendBuffer[0] = state;

                while( sendToProc && iterArray[0].getCurrentGlobalIndex() < workingIntervalsPerLevel[idxLevel * nbProcess + sendToProc].min){
                    --sendToProc;
                }

                printf("I send %d bytes to proc id %d \n", idxBuff, sendToProc);
                MPI_Isend(sendBuffer, idxBuff, MPI_BYTE, sendToProc, 0, MPI_COMM_WORLD, &requests[iterRequests++]);
            }


            bool hasToReceive = false;
            int firstProcThatSend = idProcess + 1;
            int endProcThatSend = firstProcThatSend;

            if(idProcess != nbProcess - 1){
                while(firstProcThatSend < nbProcess &&
                      workingIntervalsPerLevel[idxLevel * nbProcess + firstProcThatSend].max < workingIntervalsPerLevel[idxLevel * nbProcess + firstProcThatSend].min ){
                    ++firstProcThatSend;
                }
                endProcThatSend = firstProcThatSend;
                while( endProcThatSend < nbProcess &&
                        realIntervalsPerLevel[idxLevel * nbProcess + endProcThatSend].min <= workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].max){
                    ++endProcThatSend;
                }
                if(firstProcThatSend != endProcThatSend){
                    hasToReceive = true;

                    for(int idxProc = firstProcThatSend ; idxProc < endProcThatSend ; ++idxProc ){
                        printf("I irecv from proc id %d \n", idxProc);
                        MPI_Irecv(&recvBuffer[idxProc * recvBufferOffset], recvBufferOffset, MPI_BYTE, idxProc, 0, MPI_COMM_WORLD, &requests[iterRequests++]);
                    }
                }
            }

            for( int idxCell = cellsToSend ; idxCell < (hasToReceive?numberOfCells-1:numberOfCells) ; ++idxCell){
                myThreadkernels.M2M( iterArray[idxCell].getCurrentCell() , iterArray[idxCell].getCurrentChild(), idxLevel);
            }

            if(iterRequests){
                MPI_Waitall( iterRequests, requests, 0);

                printf("Send/Receive what I need\n");

                if( hasToReceive ){
                    CellClass* currentChild[8];
                    memcpy(currentChild, iterArray[numberOfCells - 1].getCurrentChild(), 8 * sizeof(CellClass*));

                    for(int idxProc = firstProcThatSend ; idxProc < endProcThatSend ; ++idxProc){
                        char state = recvBuffer[idxProc * recvBufferOffset];

                        int position = 0;
                        int bufferIndex = 1;
                        while( state && position < 8){
                            while(!(state & 0x1)){
                                state >>= 1;
                                ++position;
                            }

                            printf("Receive Index is %lld child position is %d\n", iterArray[numberOfCells - 1].getCurrentGlobalIndex(), position);

                            fassert(!currentChild[position], "Already has a cell here", __LINE__, __FILE__);
                            currentChild[position] = (CellClass*) &recvBuffer[idxProc * recvBufferOffset + bufferIndex];
                            bufferIndex += sizeof(CellClass);

                            state >>= 1;
                            ++position;
                        }
                    }

                    printf("Before M2M\n");
                    myThreadkernels.M2M( iterArray[numberOfCells - 1].getCurrentCell() , currentChild, idxLevel);
                    printf("After M2M\n");
                }
            }
        }


        FDEBUG( FDebug::Controller << "\tFinished (@Upward Pass (M2M) = "  << counterTime.tacAndElapsed() << "s)\n" );
        FDEBUG( FDebug::Controller << "\t\t Computation : " << computationCounter.cumulated() << " s\n" );
        FDEBUG( FDebug::Controller << "\t\t Send : " << sendCounter.cumulated() << " s\n" );
        FDEBUG( FDebug::Controller << "\t\t Receive : " << receiveCounter.cumulated() << " s\n" );
        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }

    /////////////////////////////////////////////////////////////////////////////
    // Downard
    /////////////////////////////////////////////////////////////////////////////

    /** M2L L2L */
    void downardPass(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );

        { // first M2L
            FDEBUG( FDebug::Controller.write("\tStart Downward Pass (M2L)\n").write(FDebug::Flush); );
            FDEBUG(FTic counterTime);
            FDEBUG(FTic computationCounter);
            FDEBUG(FTic sendCounter);
            FDEBUG(FTic receiveCounter);
            FDEBUG(FTic waitingToReceiveCounter);
            FDEBUG(FTic waitSendCounter);
            FDEBUG(FTic findCounter);


            // pointer to send
            typename OctreeClass::Iterator* toSend[nbProcess * OctreeHeight];
            memset(toSend, 0, sizeof(typename OctreeClass::Iterator*) * nbProcess * OctreeHeight );
            int sizeToSend[nbProcess * OctreeHeight];
            memset(sizeToSend, 0, sizeof(int) * nbProcess * OctreeHeight);
            // index
            int indexToSend[nbProcess * OctreeHeight];
            memset(indexToSend, 0, sizeof(int) * nbProcess * OctreeHeight);

            // To know if a leaf has been already sent to a proc
            bool alreadySent[nbProcess];

            FBoolArray* leafsNeedOther[OctreeHeight];
            memset(leafsNeedOther, 0, sizeof(FBoolArray) * OctreeHeight);

            {
                typename OctreeClass::Iterator octreeIterator(tree);
                octreeIterator.moveDown();
                typename OctreeClass::Iterator avoidGotoLeftIterator(octreeIterator);
                // for each levels
                for(int idxLevel = 2 ; idxLevel < OctreeHeight ; ++idxLevel ){
                    int numberOfCells = 0;

                    while(octreeIterator.getCurrentGlobalIndex() <  workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].min){
                        octreeIterator.moveRight();
                    }

                    // for each cells
                    do{
                        iterArray[numberOfCells] = octreeIterator;
                        ++numberOfCells;
                    } while(octreeIterator.getCurrentGlobalIndex() <  workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].max && octreeIterator.moveRight());
                    avoidGotoLeftIterator.moveDown();
                    octreeIterator = avoidGotoLeftIterator;

                    leafsNeedOther[idxLevel] = new FBoolArray(numberOfCells);


                    // Which cell potentialy need other data and in the same time
                    // are potentialy needed by other
                    MortonIndex neighborsIndexes[208];
                    for(int idxCell = 0 ; idxCell < numberOfCells ; ++idxCell){
                        // Find the M2L neigbors of a cell
                        const int counter = getDistantNeighbors(iterArray[idxCell].getCurrentGlobalCoordinate(),idxLevel,neighborsIndexes);

                        memset(alreadySent, false, sizeof(bool) * nbProcess);
                        bool needOther = false;
                        // Test each negibors to know which one do not belong to us
                        for(int idxNeigh = 0 ; idxNeigh < counter ; ++idxNeigh){
                            if(neighborsIndexes[idxNeigh] < workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].min
                                    || workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].max < neighborsIndexes[idxNeigh]){
                                int procToReceive = idProcess;
                                while( 0 != procToReceive && neighborsIndexes[idxNeigh] < workingIntervalsPerLevel[idxLevel * nbProcess + procToReceive].min ){
                                    --procToReceive;
                                }
                                while( procToReceive != nbProcess -1 && workingIntervalsPerLevel[idxLevel * nbProcess + procToReceive].max < neighborsIndexes[idxNeigh]){
                                    ++procToReceive;
                                }
                                // Maybe already sent to that proc?
                                if( !alreadySent[procToReceive]
                                    && workingIntervalsPerLevel[idxLevel * nbProcess + procToReceive].min <= neighborsIndexes[idxNeigh]
                                    && neighborsIndexes[idxNeigh] <= workingIntervalsPerLevel[idxLevel * nbProcess + procToReceive].max){

                                    alreadySent[procToReceive] = true;

                                    needOther = true;

                                    if(indexToSend[idxLevel * nbProcess + procToReceive] ==  sizeToSend[idxLevel * nbProcess + procToReceive]){
                                        const int previousSize = sizeToSend[idxLevel * nbProcess + procToReceive];
                                        sizeToSend[idxLevel * nbProcess + procToReceive] = FMath::Max(int(10*sizeof(typename OctreeClass::Iterator)), int(sizeToSend[idxLevel * nbProcess + procToReceive] * 1.5));
                                        typename OctreeClass::Iterator* temp = toSend[idxLevel * nbProcess + procToReceive];
                                        toSend[idxLevel * nbProcess + procToReceive] = reinterpret_cast<typename OctreeClass::Iterator*>(new char[sizeof(typename OctreeClass::Iterator) * sizeToSend[idxLevel * nbProcess + procToReceive]]);
                                        memcpy(toSend[idxLevel * nbProcess + procToReceive], temp, previousSize * sizeof(typename OctreeClass::Iterator));
                                        delete[] reinterpret_cast<char*>(temp);
                                    }

                                    toSend[idxLevel * nbProcess + procToReceive][indexToSend[idxLevel * nbProcess + procToReceive]++] = iterArray[idxCell];
                                }
                            }
                        }
                        if(needOther){
                            leafsNeedOther[idxLevel]->set(idxCell,true);
                        }

                    }

                }
            }

            printf("All gather...\n");

            // All process say to each others
            // what the will send to who
            int globalReceiveMap[nbProcess * nbProcess * OctreeHeight];
            memset(globalReceiveMap, 0, sizeof(int) * nbProcess * nbProcess * OctreeHeight);

            mpiassert( MPI_Allgather( indexToSend, nbProcess * OctreeHeight, MPI_INT, globalReceiveMap, nbProcess * OctreeHeight, MPI_INT, MPI_COMM_WORLD),  __LINE__ );

            printf("Will send ...\n");

            // Then they can send and receive (because they know what they will receive)
            // To send in asynchrone way
            MPI_Request requests[2 * nbProcess * OctreeHeight];
            int iterRequest = 0;

            CellClass* sendBuffer[nbProcess * OctreeHeight];
            memset(sendBuffer, 0, sizeof(CellClass*) * nbProcess * OctreeHeight);

            CellClass* recvBuffer[nbProcess * OctreeHeight];
            memset(recvBuffer, 0, sizeof(CellClass*) * nbProcess * OctreeHeight);


            for(int idxLevel = 2 ; idxLevel < OctreeHeight ; ++idxLevel ){
                for(int idxProc = 0 ; idxProc < nbProcess ; ++idxProc){
                    const int toSendAtProcAtLevel = indexToSend[idxLevel * nbProcess + idxProc];
                    if(toSendAtProcAtLevel != 0){
                        printf("Send %d to %d\n", toSendAtProcAtLevel, idxProc);
                        sendBuffer[idxLevel * nbProcess + idxProc] = reinterpret_cast<CellClass*>(new char[sizeof(CellClass) * toSendAtProcAtLevel]);

                        for(int idxLeaf = 0 ; idxLeaf < toSendAtProcAtLevel; ++idxLeaf){
                            memcpy(&sendBuffer[idxLevel * nbProcess + idxProc][idxLeaf], toSend[idxLevel * nbProcess + idxProc][idxLeaf].getCurrentCell(),
                                   sizeof(CellClass) );
                        }

                        mpiassert( MPI_Isend( sendBuffer[idxLevel * nbProcess + idxProc], toSendAtProcAtLevel * sizeof(CellClass) , MPI_BYTE ,
                                             idxProc, idxLevel, MPI_COMM_WORLD, &requests[iterRequest++]) , __LINE__ );
                    }

                    const int toReceiveFromProcAtLevel = globalReceiveMap[(idxProc * nbProcess * OctreeHeight) + idxLevel * nbProcess + idProcess];
                    if(toReceiveFromProcAtLevel){
                        printf("Receive %d from %d\n", toReceiveFromProcAtLevel, idxProc);
                        recvBuffer[idxLevel * nbProcess + idxProc] = reinterpret_cast<CellClass*>(new char[sizeof(CellClass) * toReceiveFromProcAtLevel]);

                        mpiassert( MPI_Irecv(recvBuffer[idxLevel * nbProcess + idxProc], toReceiveFromProcAtLevel * sizeof(CellClass), MPI_BYTE,
                                            idxProc, idxLevel, MPI_COMM_WORLD, &requests[iterRequest++]) , __LINE__ );
                    }
                }
            }

            printf("Compute ...\n");

            {
                typename OctreeClass::Iterator octreeIterator(tree);
                octreeIterator.moveDown();
                typename OctreeClass::Iterator avoidGotoLeftIterator(octreeIterator);
                // Now we can compute all the data
                // for each levels
                for(int idxLevel = 2 ; idxLevel < OctreeHeight ; ++idxLevel ){
                    int numberOfCells = 0;
                    while(octreeIterator.getCurrentGlobalIndex() <  workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].min){
                        octreeIterator.moveRight();
                    }
                    // for each cells
                    do{
                        iterArray[numberOfCells] = octreeIterator;
                        ++numberOfCells;
                    } while(octreeIterator.getCurrentGlobalIndex() <  workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].max && octreeIterator.moveRight());
                    avoidGotoLeftIterator.moveDown();
                    octreeIterator = avoidGotoLeftIterator;

                    FDEBUG(computationCounter.tic());
                    #pragma omp parallel
                    {
                        KernelClass * const myThreadkernels = kernels[omp_get_thread_num()];
                        const CellClass* neighbors[208];

                        #pragma omp for  schedule(dynamic) nowait
                        for(int idxCell = 0 ; idxCell < numberOfCells ; ++idxCell){
                            const int counter = tree->getDistantNeighbors(neighbors,  iterArray[idxCell].getCurrentGlobalCoordinate(),idxLevel);
                            if(counter) myThreadkernels->M2L( iterArray[idxCell].getCurrentCell() , neighbors, counter, idxLevel);
                        }
                    }
                    FDEBUG(computationCounter.tac());
                }
            }


            printf("Wait ...\n");
            // Wait to receive every things (and send every things)
            MPI_Waitall(iterRequest, requests, 0);

            printf("Now process other...\n");

            {
                typename OctreeClass::Iterator octreeIterator(tree);
                octreeIterator.moveDown();
                typename OctreeClass::Iterator avoidGotoLeftIterator(octreeIterator);
                // compute the second time
                // for each levels
                for(int idxLevel = 2 ; idxLevel < OctreeHeight ; ++idxLevel ){
                    // put the received data into a temporary tree
                    FLightOctree<CellClass> tempTree;
                    for(int idxProc = 0 ; idxProc < nbProcess ; ++idxProc){
                        const int toReceiveFromProcAtLevel = globalReceiveMap[(idxProc * nbProcess * OctreeHeight) + idxLevel * nbProcess + idProcess];
                        const CellClass* const cells = reinterpret_cast<CellClass*>(recvBuffer[idxLevel * nbProcess + idxProc]);
                        for(int idxCell = 0 ; idxCell < toReceiveFromProcAtLevel ; ++idxCell){
                            tempTree.insertCell(cells[idxCell].getMortonIndex(), cells[idxCell], idxLevel);
                        }
                    }

                    // take cells from our octree only if they are
                    // linked to received data
                    int numberOfCells = 0;
                    int realCellId = 0;

                    while(octreeIterator.getCurrentGlobalIndex() <  workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].min){
                        octreeIterator.moveRight();
                    }
                    // for each cells
                    do{
                        if(leafsNeedOther[idxLevel]->get(realCellId++)){
                            iterArray[numberOfCells++] = octreeIterator;
                        }
                    } while(octreeIterator.getCurrentGlobalIndex() <  workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].max && octreeIterator.moveRight());
                    avoidGotoLeftIterator.moveDown();
                    octreeIterator = avoidGotoLeftIterator;

                    delete leafsNeedOther[idxLevel];
                    leafsNeedOther[idxLevel] = 0;

                    // Compute this cells
                    FDEBUG(computationCounter.tic());
                    #pragma omp parallel
                    {
                        KernelClass * const myThreadkernels = kernels[omp_get_thread_num()];
                        MortonIndex neighborsIndex[208];
                        const CellClass* neighbors[208];

                        #pragma omp for  schedule(dynamic) nowait
                        for(int idxCell = 0 ; idxCell < numberOfCells ; ++idxCell){
                            // compute indexes
                            const int counterNeighbors = getDistantNeighbors(iterArray[idxCell].getCurrentGlobalCoordinate(), idxLevel, neighborsIndex);

                            int counter = 0;
                            // does we receive this index from someone?
                            for(int idxNeig = 0 ;idxNeig < counterNeighbors ; ++idxNeig){
                                const CellClass* const cellFromOtherProc = tempTree.getCell(neighborsIndex[idxNeig], idxLevel);
                                if(cellFromOtherProc){
                                    neighbors[counter++] = cellFromOtherProc;
                                }
                            }
                            // need to compute
                            if(counter){
                                myThreadkernels->M2L( iterArray[idxCell].getCurrentCell() , neighbors, counter, idxLevel);
                            }
                        }
                    }
                    FDEBUG(computationCounter.tac());
                }
            }

            FDEBUG( FDebug::Controller << "\tFinished (@Downward Pass (M2L) = "  << counterTime.tacAndElapsed() << "s)\n" );
            FDEBUG( FDebug::Controller << "\t\t Computation : " << computationCounter.cumulated() << " s\n" );
            FDEBUG( FDebug::Controller << "\t\t Send : " << sendCounter.cumulated() << " s\n" );
            FDEBUG( FDebug::Controller << "\t\t Receive : " << receiveCounter.cumulated() << " s\n" );
            FDEBUG( FDebug::Controller << "\t\t Wait data to Receive : " << waitingToReceiveCounter.cumulated() << " s\n" );
            FDEBUG( FDebug::Controller << "\t\t\tTotal time to send in mpi "  << waitSendCounter.cumulated() << " s.\n" );
            FDEBUG( FDebug::Controller << "\t\t\tTotal time to find "  << findCounter.cumulated() << " s.\n" );
        }

        { // second L2L
            FDEBUG( FDebug::Controller.write("\tStart Downward Pass (L2L)\n").write(FDebug::Flush); );
            FDEBUG(FTic counterTime);
            FDEBUG(FTic computationCounter);
            FDEBUG(FTic sendCounter);
            FDEBUG(FTic receiveCounter);

            // Start from leal level - 1
            typename OctreeClass::Iterator octreeIterator(tree);
            octreeIterator.moveDown();
            typename OctreeClass::Iterator avoidGotoLeftIterator(octreeIterator);

            KernelClass& myThreadkernels = (*kernels[omp_get_thread_num()]);

            MPI_Request requests[nbProcess];

            const int heightMinusOne = OctreeHeight - 1;

            // for each levels exepted leaf level
            for(int idxLevel = 2 ; idxLevel < heightMinusOne ; ++idxLevel ){
                // copy cells to work with
                int numberOfCells = 0;
                // for each cells
                do{
                    iterArray[numberOfCells++] = octreeIterator;
                } while(octreeIterator.moveRight());
                avoidGotoLeftIterator.moveDown();
                octreeIterator = avoidGotoLeftIterator;


                bool needToRecv = false;
                int iterRequests = 0;

                // do we need to receive one or zeros cell
                if(idProcess != 0
                        && realIntervalsPerLevel[idxLevel * nbProcess + idProcess].min != workingIntervalsPerLevel[idxLevel * nbProcess + idProcess].min){
                    needToRecv = true;
                    printf("I recv one cell at index %lld\n", iterArray[0].getCurrentGlobalIndex() );
                    MPI_Irecv( iterArray[0].getCurrentCell(), sizeof(CellClass), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &requests[iterRequests++]);
                }


                if(idProcess != nbProcess - 1){
                    int firstProcThatRecv = idProcess + 1;
                    while( firstProcThatRecv < nbProcess &&
                            workingIntervalsPerLevel[idxLevel * nbProcess + firstProcThatRecv].max < workingIntervalsPerLevel[idxLevel * nbProcess + firstProcThatRecv].min){
                        ++firstProcThatRecv;
                    }

                    int endProcThatRecv = firstProcThatRecv;
                    while( endProcThatRecv < nbProcess &&
                            (realIntervalsPerLevel[(idxLevel+1) * nbProcess + endProcThatRecv].min >> 3) == iterArray[numberOfCells - 1].getCurrentGlobalIndex()){
                        ++endProcThatRecv;
                    }

                    if(endProcThatRecv != idProcess + 1){
                        for(int idxProc = firstProcThatRecv ; idxProc < endProcThatRecv ; ++idxProc ){
                            printf("I send 1 to proc %d at index %lld\n", idxProc, iterArray[numberOfCells - 1].getCurrentGlobalIndex());
                            MPI_Isend(iterArray[numberOfCells - 1].getCurrentCell(), sizeof(CellClass), MPI_BYTE, idxProc, 0, MPI_COMM_WORLD, &requests[iterRequests++]);
                        }
                    }
                }

                for(int idxCell = (needToRecv ? 1 : 0) ; idxCell < numberOfCells ; ++idxCell){
                    myThreadkernels.L2L( iterArray[idxCell].getCurrentCell() , iterArray[idxCell].getCurrentChild(), idxLevel);
                }

                if(iterRequests){
                    MPI_Waitall( iterRequests, requests, 0);
                    if(needToRecv){
                        myThreadkernels.L2L( iterArray[0].getCurrentCell() , iterArray[0].getCurrentChild(), idxLevel);
                    }
                }
            }

            FDEBUG( FDebug::Controller << "\tFinished (@Downward Pass (L2L) = "  << counterTime.tacAndElapsed() << "s)\n" );
            FDEBUG( FDebug::Controller << "\t\t Computation : " << computationCounter.cumulated() << " s\n" );
            FDEBUG( FDebug::Controller << "\t\t Send : " << sendCounter.cumulated() << " s\n" );
            FDEBUG( FDebug::Controller << "\t\t Receive : " << receiveCounter.cumulated() << " s\n" );
        }

        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }

    /////////////////////////////////////////////////////////////////////////////
    // Direct
    /////////////////////////////////////////////////////////////////////////////

    /** P2P */
    void directPass(){
        FTRACE( FTrace::Controller.enterFunction(FTrace::FMM, __FUNCTION__ , __FILE__ , __LINE__) );
        FDEBUG( FDebug::Controller.write("\tStart Direct Pass\n").write(FDebug::Flush); );
        FDEBUG(FTic counterTime);

        ///////////////////////////////////////////////////
        // Precompute
        ///////////////////////////////////////////////////
        FBoolArray leafsNeedOther(this->numberOfLeafs);
        OctreeClass otherP2Ptree( tree->getHeight(), tree->getSubHeight(), tree->getBoxWidth(), tree->getBoxCenter() );

        {
            // Copy leafs
            {
                typename OctreeClass::Iterator octreeIterator(tree);
                octreeIterator.gotoBottomLeft();
                int idxLeaf = 0;
                do{
                    this->iterArray[idxLeaf++] = octreeIterator;
                } while(octreeIterator.moveRight());
            }

            printf("Prepare P2P\n");

            // Box limite
            const long limite = 1 << (this->OctreeHeight - 1);
            // pointer to send
            typename OctreeClass::Iterator* toSend[nbProcess];
            memset(toSend, 0, sizeof(typename OctreeClass::Iterator*) * nbProcess );
            int sizeToSend[nbProcess];
            memset(sizeToSend, 0, sizeof(int) * nbProcess);
            // index
            int indexToSend[nbProcess];
            memset(indexToSend, 0, sizeof(int) * nbProcess);
            // index
            int partsToSend[nbProcess];
            memset(partsToSend, 0, sizeof(int) * nbProcess);

            // To know if a leaf has been already sent to a proc
            bool alreadySent[nbProcess];

            MortonIndex indexesNeighbors[26];

            for(int idxLeaf = 0 ; idxLeaf < this->numberOfLeafs ; ++idxLeaf){
                FTreeCoordinate center;
                center.setPositionFromMorton(iterArray[idxLeaf].getCurrentGlobalIndex(), OctreeHeight - 1);

                memset(alreadySent, false, sizeof(bool) * nbProcess);
                bool needOther = false;

                const int neighCount = getNeighborsIndexes(iterArray[idxLeaf].getCurrentGlobalIndex(), limite, indexesNeighbors);

                for(int idxNeigh = 0 ; idxNeigh < neighCount ; ++idxNeigh){
                    if(indexesNeighbors[idxNeigh] < intervals[idProcess].min || intervals[idProcess].max < indexesNeighbors[idxNeigh]){
                        needOther = true;

                        // find the proc that need this information
                        int procToReceive = idProcess;
                        while( procToReceive != 0 && indexesNeighbors[idxNeigh] < intervals[procToReceive].min){
                            --procToReceive;
                        }

                        while( procToReceive != nbProcess - 1 && intervals[procToReceive].max < indexesNeighbors[idxNeigh]){
                            ++procToReceive;
                        }

                        if( !alreadySent[procToReceive] && intervals[procToReceive].min <= indexesNeighbors[idxNeigh] && indexesNeighbors[idxNeigh] <= intervals[procToReceive].max){
                            alreadySent[procToReceive] = true;
                            if(indexToSend[procToReceive] ==  sizeToSend[procToReceive]){
                                const int previousSize = sizeToSend[procToReceive];
                                sizeToSend[procToReceive] = FMath::Max(10*int(sizeof(typename OctreeClass::Iterator)), int(sizeToSend[procToReceive] * 1.5));
                                typename OctreeClass::Iterator* temp = toSend[procToReceive];
                                toSend[procToReceive] = reinterpret_cast<typename OctreeClass::Iterator*>(new char[sizeof(typename OctreeClass::Iterator) * sizeToSend[procToReceive]]);
                                memcpy(toSend[procToReceive], temp, previousSize * sizeof(typename OctreeClass::Iterator));
                                delete[] reinterpret_cast<char*>(temp);
                            }
                            toSend[procToReceive][indexToSend[procToReceive]++] = iterArray[idxLeaf];
                            partsToSend[procToReceive] += iterArray[idxLeaf].getCurrentListSrc()->getSize();
                        }
                    }
                }

                if(needOther){
                    leafsNeedOther.set(idxLeaf,true);
                }
            }

            int globalReceiveMap[nbProcess * nbProcess];
            memset(globalReceiveMap, 0, sizeof(int) * nbProcess * nbProcess);

            mpiassert( MPI_Allgather( partsToSend, nbProcess, MPI_INT, globalReceiveMap, nbProcess, MPI_INT, MPI_COMM_WORLD),  __LINE__ );

            for(int idxProc = 0 ; idxProc < nbProcess ; ++idxProc){
                printf("indexToSend[%d] = %d leafs %d parts\n", idxProc, indexToSend[idxProc], partsToSend[idxProc]);
            }

            printf("Will send ...\n");

            // To send in asynchrone way
            MPI_Request requests[2 * nbProcess];
            int iterRequest = 0;

            ParticleClass* sendBuffer[nbProcess];
            memset(sendBuffer, 0, sizeof(ParticleClass*) * nbProcess);

            ParticleClass* recvBuffer[nbProcess];
            memset(recvBuffer, 0, sizeof(ParticleClass*) * nbProcess);

            for(int idxProc = 0 ; idxProc < nbProcess ; ++idxProc){
                if(indexToSend[idxProc] != 0){
                    printf("Send %d to %d\n", indexToSend[idxProc], idxProc);
                    sendBuffer[idxProc] = reinterpret_cast<ParticleClass*>(new char[sizeof(ParticleClass) * partsToSend[idxProc]]);

                    int currentIndex = 0;
                    for(int idxLeaf = 0 ; idxLeaf < indexToSend[idxProc] ; ++idxLeaf){
                        memcpy(&sendBuffer[idxProc][currentIndex], toSend[idxProc][idxLeaf].getCurrentListSrc()->data(),
                               sizeof(ParticleClass) * toSend[idxProc][idxLeaf].getCurrentListSrc()->getSize() );
                        currentIndex += toSend[idxProc][idxLeaf].getCurrentListSrc()->getSize();
                    }

                    mpiassert( MPI_Isend( sendBuffer[idxProc], sizeof(ParticleClass) * partsToSend[idxProc] , MPI_BYTE ,
                                         idxProc, TAG_P2P_PART, MPI_COMM_WORLD, &requests[iterRequest++]) , __LINE__ );

                }
                if(globalReceiveMap[idxProc * nbProcess + idProcess]){
                    printf("Receive %d from %d\n", globalReceiveMap[idxProc * nbProcess + idProcess], idxProc);
                    recvBuffer[idxProc] = reinterpret_cast<ParticleClass*>(new char[sizeof(ParticleClass) * globalReceiveMap[idxProc * nbProcess + idProcess]]);

                    mpiassert( MPI_Irecv(recvBuffer[idxProc], globalReceiveMap[idxProc * nbProcess + idProcess]*sizeof(ParticleClass), MPI_BYTE,
                                        idxProc, TAG_P2P_PART, MPI_COMM_WORLD, &requests[iterRequest++]) , __LINE__ );
                }
            }

            printf("Wait ...\n");
            MPI_Waitall(iterRequest, requests, 0);

            printf("Put data in the tree\n");
            for(int idxProc = 0 ; idxProc < nbProcess ; ++idxProc){
                for(int idxPart = 0 ; idxPart < globalReceiveMap[idxProc * nbProcess + idProcess] ; ++idxPart){
                    otherP2Ptree.insert(recvBuffer[idxProc][idxPart]);
                }
            }


            printf("Delete array\n");
            for(int idxProc = 0 ; idxProc < nbProcess ; ++idxProc){
                delete [] reinterpret_cast<char*>(sendBuffer[idxProc]);
                delete [] reinterpret_cast<char*>(recvBuffer[idxProc]);
                delete [] reinterpret_cast<char*>(toSend[idxProc]);
            }
            printf("prep2p is finished\n");
        }

        ///////////////////////////////////////////////////
        // Direct Computation
        ///////////////////////////////////////////////////

        // init
        const int LeafIndex = OctreeHeight - 1;
        const int SizeShape = 3*3*3;

        int shapeLeaf[SizeShape];
        memset(shapeLeaf,0,SizeShape*sizeof(int));

        struct LeafData{
            MortonIndex index;
            CellClass* cell;
            ContainerClass* targets;
            ContainerClass* sources;
        };
        LeafData* const leafsDataArray = new LeafData[this->numberOfLeafs];

        // split data
        {
            typename OctreeClass::Iterator octreeIterator(tree);
            octreeIterator.gotoBottomLeft();

            // to store which shape for each leaf
            typename OctreeClass::Iterator* const myLeafs = new typename OctreeClass::Iterator[this->numberOfLeafs];
            int*const shapeType = new int[this->numberOfLeafs];

            for(int idxLeaf = 0 ; idxLeaf < this->numberOfLeafs ; ++idxLeaf){
                myLeafs[idxLeaf] = octreeIterator;

                const FTreeCoordinate& coord = octreeIterator.getCurrentCell()->getCoordinate();
                const int shape = (coord.getX()%3)*9 + (coord.getY()%3)*3 + (coord.getZ()%3);
                shapeType[idxLeaf] = shape;

                ++shapeLeaf[shape];

                octreeIterator.moveRight();
            }

            int startPosAtShape[SizeShape];
            startPosAtShape[0] = 0;
            for(int idxShape = 1 ; idxShape < SizeShape ; ++idxShape){
                startPosAtShape[idxShape] = startPosAtShape[idxShape-1] + shapeLeaf[idxShape-1];
            }

            int idxInArray = 0;
            for(int idxLeaf = 0 ; idxLeaf < this->numberOfLeafs ; ++idxLeaf, ++idxInArray){
                const int shapePosition = shapeType[idxInArray];

                leafsDataArray[startPosAtShape[shapePosition]].index = myLeafs[idxInArray].getCurrentGlobalIndex();
                leafsDataArray[startPosAtShape[shapePosition]].cell = myLeafs[idxInArray].getCurrentCell();
                leafsDataArray[startPosAtShape[shapePosition]].targets = myLeafs[idxInArray].getCurrentListTargets();
                leafsDataArray[startPosAtShape[shapePosition]].sources = myLeafs[idxInArray].getCurrentListSrc();

                ++startPosAtShape[shapePosition];
            }

            delete[] shapeType;
            delete[] myLeafs;
        }

        FDEBUG(FTic computationCounter);

        #pragma omp parallel
        {
            KernelClass& myThreadkernels = (*kernels[omp_get_thread_num()]);
            // There is a maximum of 26 neighbors
            ContainerClass* neighbors[26];
            MortonIndex neighborsIndex[26];
            int previous = 0;
            // Box limite
            const long limite = 1 << (this->OctreeHeight - 1);

            for(int idxShape = 0 ; idxShape < SizeShape ; ++idxShape){
                const int endAtThisShape = shapeLeaf[idxShape] + previous;

                #pragma omp for schedule(dynamic)
                for(int idxLeafs = previous ; idxLeafs < endAtThisShape ; ++idxLeafs){
                    LeafData& currentIter = leafsDataArray[idxLeafs];
                    myThreadkernels.L2P(currentIter.cell, currentIter.targets);

                    // need the current particles and neighbors particles
                    const int counter = tree->getLeafsNeighborsWithIndex(neighbors, neighborsIndex, currentIter.index,LeafIndex);

                    if(leafsNeedOther.get(idxLeafs)){
                        MortonIndex indexesNeighbors[26];
                        const int neighCount = getNeighborsIndexes(currentIter.index, limite, indexesNeighbors);
                        int counterAll = counter;
                        for(int idxNeigh = 0 ; idxNeigh < neighCount ; ++idxNeigh){
                            if(indexesNeighbors[idxNeigh] < intervals[idProcess].min || intervals[idProcess].max < indexesNeighbors[idxNeigh]){
                                ContainerClass*const hypotheticNeighbor = otherP2Ptree.getLeafSrc(indexesNeighbors[idxNeigh]);
                                if(hypotheticNeighbor){
                                    neighbors[counterAll] = hypotheticNeighbor;
                                    neighborsIndex[counterAll] = indexesNeighbors[idxNeigh];
                                    ++counterAll;
                                }
                            }
                        }
                        myThreadkernels.P2P( currentIter.index,currentIter.targets, currentIter.sources , neighbors, neighborsIndex, counterAll);
                    }
                    else{
                        myThreadkernels.P2P( currentIter.index,currentIter.targets, currentIter.sources , neighbors, neighborsIndex, counter);
                    }
                }

                previous = endAtThisShape;
            }
        }
        FDEBUG(computationCounter.tac());

        FDEBUG( FDebug::Controller << "\tFinished (@Direct Pass (L2P + P2P) = "  << counterTime.tacAndElapsed() << "s)\n" );
        FDEBUG( FDebug::Controller << "\t\t Computation L2P + P2P : " << computationCounter.elapsed() << " s\n" );
        FTRACE( FTrace::Controller.leaveFunction(FTrace::FMM) );
    }


    int getNeighborsIndexes(const MortonIndex centerIndex, const long limite, MortonIndex indexes[26]) const{
        FTreeCoordinate center;
        center.setPositionFromMorton(centerIndex, OctreeHeight - 1);
        int idxNeig = 0;
        // We test all cells around
        for(long idxX = -1 ; idxX <= 1 ; ++idxX){
            if(!FMath::Between(center.getX() + idxX,0l, limite)) continue;

            for(long idxY = -1 ; idxY <= 1 ; ++idxY){
                if(!FMath::Between(center.getY() + idxY,0l, limite)) continue;

                for(long idxZ = -1 ; idxZ <= 1 ; ++idxZ){
                    if(!FMath::Between(center.getZ() + idxZ,0l, limite)) continue;

                    // if we are not on the current cell
                    if( idxX || idxY || idxZ ){
                        const FTreeCoordinate other(center.getX() + idxX,center.getY() + idxY,center.getZ() + idxZ);
                        indexes[idxNeig++] = other.getMortonIndex(this->OctreeHeight - 1);
                    }
                }
            }
        }
        return idxNeig;
    }

    int getDistantNeighbors(const FTreeCoordinate& workingCell,const int inLevel, MortonIndex inNeighbors[208]) const{

        // Then take each child of the parent's neighbors if not in directNeighbors
        // Father coordinate
        const FTreeCoordinate parentCell(workingCell.getX()>>1,workingCell.getY()>>1,workingCell.getZ()>>1);

        // Limite at parent level number of box (split by 2 by level)
        const long limite = FMath::pow(2,inLevel-1);

        int idxNeighbors = 0;
        // We test all cells around
        for(long idxX = -1 ; idxX <= 1 ; ++idxX){
            if(!FMath::Between(parentCell.getX() + idxX,0l,limite)) continue;

            for(long idxY = -1 ; idxY <= 1 ; ++idxY){
                if(!FMath::Between(parentCell.getY() + idxY,0l,limite)) continue;

                for(long idxZ = -1 ; idxZ <= 1 ; ++idxZ){
                    if(!FMath::Between(parentCell.getZ() + idxZ,0l,limite)) continue;

                    // if we are not on the current cell
                    if( idxX || idxY || idxZ ){
                        const FTreeCoordinate other(parentCell.getX() + idxX,parentCell.getY() + idxY,parentCell.getZ() + idxZ);
                        const MortonIndex mortonOther = other.getMortonIndex(inLevel-1);

                        // For each child
                        for(int idxCousin = 0 ; idxCousin < 8 ; ++idxCousin){
                            const FTreeCoordinate potentialNeighbor((other.getX()<<1) | (idxCousin>>2 & 1),
                                                                   (other.getY()<<1) | (idxCousin>>1 & 1),
                                                                   (other.getZ()<<1) | (idxCousin&1));
                            // Test if it is a direct neighbor
                            if(FMath::Abs(workingCell.getX() - potentialNeighbor.getX()) > 1 ||
                                    FMath::Abs(workingCell.getY() - potentialNeighbor.getY()) > 1 ||
                                    FMath::Abs(workingCell.getZ() - potentialNeighbor.getZ()) > 1){
                                // add to neighbors
                                inNeighbors[idxNeighbors++] = (mortonOther << 3) | idxCousin;
                            }
                        }
                    }
                }
            }
        }

        return idxNeighbors;
    }
};






#endif //FFMMALGORITHMTHREAD_HPP

// [--LICENSE--]
