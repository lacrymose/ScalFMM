#ifndef FMPITREEBUILDER_H
#define FMPITREEBUILDER_H

#include "../Utils/FMpi.hpp"
#include "../Utils/FQuickSort.hpp"
#include "../Utils/FBitonicSort.hpp"

#include "../Utils/FMemUtils.hpp"
#include "../Utils/FTrace.hpp"

/** This class manage the loading of particles for the mpi version.
  * It use a BinLoader and then sort the data with a parallel quick sort
  * or bitonic sort.
  * After it needs to merge the leaves to finally equalize the data.
  */
template<class ParticleClass>
class FMpiTreeBuilder{
public:
    /** What sorting algorithm to use */
    enum SortingType{
        QuickSort,
        BitonicSort,
    };

private:
    /** This method has been tacken from the octree
      * it computes a tree coordinate (x or y or z) from real position
      */
    static long getTreeCoordinate(const FReal inRelativePosition, const FReal boxWidthAtLeafLevel) {
            const FReal indexFReal = inRelativePosition / boxWidthAtLeafLevel;
            const long index = long(FMath::dfloor(indexFReal));
            if( index && FMath::LookEqual(inRelativePosition, boxWidthAtLeafLevel * FReal(index) ) ){
                    return index - 1;
            }
            return index;
    }


    /** A particle may not have a MortonIndex Method
      * but they are sorted on their morton index
      * so this struct store a particle + its index
      */
    struct IndexedParticle{
        MortonIndex index;
        ParticleClass particle;

        operator MortonIndex(){
            return this->index;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // To sort tha particles we hold
    //////////////////////////////////////////////////////////////////////////


    template <class LoaderClass>
    static void SortParticles( const FMpi::FComm& communicator, LoaderClass& loader, const SortingType type,
                        const int TreeHeight, IndexedParticle**const outputArray, FSize* const outputSize){
        FTRACE( FTrace::FFunction functionTrace(__FUNCTION__ , "Loader to Tree" , __FILE__ , __LINE__) );

        // create particles
        IndexedParticle*const realParticlesIndexed = new IndexedParticle[loader.getNumberOfParticles()];
        FMemUtils::memset(realParticlesIndexed, 0, sizeof(IndexedParticle) * loader.getNumberOfParticles());
        F3DPosition boxCorner(loader.getCenterOfBox() - (loader.getBoxWidth()/2));
        FTreeCoordinate host;

        const FReal boxWidthAtLeafLevel = loader.getBoxWidth() / FReal(1 << (TreeHeight - 1) );
        for(long idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
            loader.fillParticle(realParticlesIndexed[idxPart].particle);
            host.setX( getTreeCoordinate( realParticlesIndexed[idxPart].particle.getPosition().getX() - boxCorner.getX(), boxWidthAtLeafLevel ));
            host.setY( getTreeCoordinate( realParticlesIndexed[idxPart].particle.getPosition().getY() - boxCorner.getY(), boxWidthAtLeafLevel ));
            host.setZ( getTreeCoordinate( realParticlesIndexed[idxPart].particle.getPosition().getZ() - boxCorner.getZ(), boxWidthAtLeafLevel ));

            realParticlesIndexed[idxPart].index = host.getMortonIndex(TreeHeight - 1);
        }

        // sort particles
        if(type == QuickSort){
            FQuickSort<IndexedParticle,MortonIndex, FSize>::QsMpi(realParticlesIndexed, loader.getNumberOfParticles(), *outputArray, *outputSize,communicator);
            delete [] (realParticlesIndexed);
        }
        else {
            FBitonicSort<IndexedParticle,MortonIndex, FSize>::Sort( realParticlesIndexed, loader.getNumberOfParticles(), communicator );
            *outputArray = realParticlesIndexed;
            *outputSize = loader.getNumberOfParticles();
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // To merge the leaves
    //////////////////////////////////////////////////////////////////////////

    static void MergeLeaves(const FMpi::FComm& communicator, IndexedParticle*& workingArray, FSize& workingSize,
                            char** leavesArray, FSize* const leavesSize){
        FTRACE( FTrace::FFunction functionTrace(__FUNCTION__, "Loader to Tree" , __FILE__ , __LINE__) );
        const int rank = communicator.processId();
        const int nbProcs = communicator.processCount();

        // be sure there is no splited leaves
        // to do that we exchange the first index with the left proc
        {
            FTRACE( FTrace::FRegion regionTrace("Remove Splited leaves", __FUNCTION__ , __FILE__ , __LINE__) );

            MortonIndex otherFirstIndex = -1;
            if(workingSize != 0 && rank != 0 && rank != nbProcs - 1){
                MPI_Sendrecv(&workingArray[0].index, 1, MPI_LONG_LONG, rank - 1, FMpi::TagExchangeIndexs,
                             &otherFirstIndex, 1, MPI_LONG_LONG, rank + 1, FMpi::TagExchangeIndexs,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else if( rank == 0){
                MPI_Recv(&otherFirstIndex, 1, MPI_LONG_LONG, rank + 1, FMpi::TagExchangeIndexs, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else if( rank == nbProcs - 1){
                MPI_Send( &workingArray[0].index, 1, MPI_LONG_LONG, rank - 1, FMpi::TagExchangeIndexs, MPI_COMM_WORLD);
            }
            else {
                MPI_Recv(&otherFirstIndex, 1, MPI_LONG_LONG, rank + 1, FMpi::TagExchangeIndexs, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(&otherFirstIndex, 1, MPI_LONG_LONG, rank - 1, FMpi::TagExchangeIndexs, MPI_COMM_WORLD);
            }

            // at this point every one know the first index of his right neighbors
            const bool needToRecvBeforeSend = (rank != 0 && ((workingSize && otherFirstIndex == workingArray[0].index ) || !workingSize));
            MPI_Request requestSendLeaf;

            IndexedParticle* sendBuffer = 0;
            if(rank != nbProcs - 1 && needToRecvBeforeSend == false){
                FSize idxPart = workingSize - 1 ;
                while(idxPart >= 0 && workingArray[idxPart].index == otherFirstIndex){
                    --idxPart;
                }
                const int particlesToSend = int(workingSize - 1 - idxPart);
                if(particlesToSend){
                    workingSize -= particlesToSend;
                    sendBuffer = new IndexedParticle[particlesToSend];
                    memcpy(sendBuffer, &workingArray[idxPart + 1], particlesToSend * sizeof(IndexedParticle));

                    MPI_Isend( sendBuffer, particlesToSend * sizeof(IndexedParticle), MPI_BYTE, rank + 1, FMpi::TagSplittedLeaf, MPI_COMM_WORLD, &requestSendLeaf);
                }
                else{
                    MPI_Isend( 0, 0, MPI_BYTE, rank + 1, FMpi::TagSplittedLeaf, MPI_COMM_WORLD, &requestSendLeaf);
                }
            }

            if( rank != 0 ){
                int sendByOther = 0;

                MPI_Status probStatus;
                MPI_Probe(rank - 1, FMpi::TagSplittedLeaf, MPI_COMM_WORLD, &probStatus);
                MPI_Get_count( &probStatus,  MPI_BYTE, &sendByOther);

                if(sendByOther){
                    sendByOther /= sizeof(IndexedParticle);

                    const IndexedParticle* const reallocOutputArray = workingArray;
                    const FSize reallocOutputSize = workingSize;

                    workingSize += sendByOther;
                    workingArray = new IndexedParticle[workingSize];
                    FMemUtils::memcpy(&workingArray[sendByOther], reallocOutputArray, reallocOutputSize * sizeof(IndexedParticle));
                    delete[] reallocOutputArray;

                    MPI_Recv(workingArray, sizeof(IndexedParticle) * sendByOther, MPI_BYTE, rank - 1, FMpi::TagSplittedLeaf, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                else{
                    MPI_Recv( 0, 0, MPI_BYTE, rank - 1, FMpi::TagSplittedLeaf, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }

            if(rank != nbProcs - 1 && needToRecvBeforeSend == true){
                MPI_Send( workingArray, int(workingSize * sizeof(IndexedParticle)), MPI_BYTE, rank + 1, FMpi::TagSplittedLeaf, MPI_COMM_WORLD);
                delete[] workingArray;
                workingArray = 0;
                workingSize  = 0;
            }
            else if(rank != nbProcs - 1){
                MPI_Wait( &requestSendLeaf, MPI_STATUS_IGNORE);
                delete[] sendBuffer;
                sendBuffer = 0;
            }
        }

        {
            FTRACE( FTrace::FRegion regionTrace("Remove Splited leaves", __FUNCTION__ , __FILE__ , __LINE__) );
            // We now copy the data from a sorted type into real particles array + counter

            (*leavesSize)  = 0;
            (*leavesArray) = 0;

            if(workingSize){
                (*leavesArray) = new char[workingSize * (sizeof(ParticleClass) + sizeof(int))];

                MortonIndex previousIndex = -1;
                char* writeIndex = (*leavesArray);
                int* writeCounter = 0;

                for( FSize idxPart = 0; idxPart < workingSize ; ++idxPart){
                    if( workingArray[idxPart].index != previousIndex ){
                        previousIndex = workingArray[idxPart].index;
                        ++(*leavesSize);

                        writeCounter = reinterpret_cast<int*>( writeIndex );
                        writeIndex += sizeof(int);

                        (*writeCounter) = 0;
                    }

                    memcpy(writeIndex, &workingArray[idxPart].particle, sizeof(ParticleClass));

                    writeIndex += sizeof(ParticleClass);
                    ++(*writeCounter);
                }
            }
            delete [] workingArray;

            workingArray = 0;
            workingSize = 0;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // To equalize (same number of leaves among the procs)
    //////////////////////////////////////////////////////////////////////////

    /** Put the interval into a tree */
    template <class OctreeClass>
    static void EqualizeAndFillTree(const FMpi::FComm& communicator,  OctreeClass& realTree,
                             char*& leavesArray, FSize& nbLeavesInIntervals ){
        FTRACE( FTrace::FFunction functionTrace(__FUNCTION__, "Loader to Tree" , __FILE__ , __LINE__) );
        const int rank = communicator.processId();
        const int nbProcs = communicator.processCount();
        const FSize currentNbLeafs = nbLeavesInIntervals;

        // We have to know the number of leaves each procs holds
        FSize leavesPerProcs[nbProcs];
        memset(leavesPerProcs, 0, sizeof(int) * nbProcs);
        MPI_Allgather(&nbLeavesInIntervals, 1, MPI_LONG_LONG, leavesPerProcs, 1, MPI_LONG_LONG, MPI_COMM_WORLD);

        // Count the number of leaves on each side
        FSize currentLeafsOnMyLeft  = 0;
        FSize currentLeafsOnMyRight = 0;
        for(int idxProc = 0 ; idxProc < nbProcs ; ++idxProc ){
            if(idxProc < rank) currentLeafsOnMyLeft  += leavesPerProcs[idxProc];
            if(rank < idxProc) currentLeafsOnMyRight += leavesPerProcs[idxProc];
        }

        // So we can know the number of total leafs and
        // the number of leaves each procs must have at the end
        const FSize totalNbLeaves               = (currentLeafsOnMyLeft + currentNbLeafs + currentLeafsOnMyRight);
        const FSize correctLeftLeavesNumber     = communicator.getLeft(totalNbLeaves);
        const FSize correctRightLeavesIndex     = communicator.getRight(totalNbLeaves);

        // This will be used for the all to all
        int leavesToSend[nbProcs];
        memset(leavesToSend, 0, sizeof(int) * nbProcs);
        int bytesToSend[nbProcs];
        memset(bytesToSend, 0, sizeof(int) * nbProcs);
        int bytesOffset[nbProcs];
        memset(bytesToSend, 0, sizeof(int) * nbProcs);

        // Buffer Position
        FSize currentIntervalPosition = 0;

        // I need to send left if I hold leaves that belong to procs on my left
        const bool iNeedToSendToLeft  = currentLeafsOnMyLeft < correctLeftLeavesNumber;
        // I need to send right if I hold leaves that belong to procs on my right
        const bool iNeedToSendToRight = correctRightLeavesIndex < currentLeafsOnMyLeft + currentNbLeafs;

        int leftProcToStartSend = rank;
        if(iNeedToSendToLeft){
            FTRACE( FTrace::FRegion regionTrace("Calcul SendToLeft", __FUNCTION__ , __FILE__ , __LINE__) );
            // Find the first proc that need my data
            int idxProc = rank - 1;
            while( idxProc > 0 ){
                const FSize thisProcRight = communicator.getOtherRight(totalNbLeaves, idxProc - 1);
                // Go to left until proc-1 has a right index lower than my current left
                if( thisProcRight < currentLeafsOnMyLeft){
                    break;
                }
                --idxProc;
            }

            // Count data for this proc
            leftProcToStartSend = idxProc;
            int ICanGive = int(currentNbLeafs);
            leavesToSend[idxProc] = int(FMath::Min(communicator.getOtherRight(totalNbLeaves, idxProc), totalNbLeaves - currentLeafsOnMyRight)
                                        - FMath::Max( currentLeafsOnMyLeft , communicator.getOtherLeft(totalNbLeaves, idxProc)));
            {
                bytesOffset[idxProc] = 0;
                for(FSize idxLeaf = 0 ; idxLeaf < leavesToSend[idxProc] ; ++idxLeaf){
                    currentIntervalPosition += ((*(int*)&leavesArray[currentIntervalPosition]) * sizeof(ParticleClass)) + sizeof(int);
                }
                bytesToSend[idxProc] = int(currentIntervalPosition - bytesOffset[idxProc]);
            }
            ICanGive -= leavesToSend[idxProc];
            ++idxProc;

            // count data to other proc
            while(idxProc < rank && ICanGive){
                leavesToSend[idxProc] = int(FMath::Min( communicator.getOtherRight(totalNbLeaves, idxProc) - communicator.getOtherLeft(totalNbLeaves, idxProc), FSize(ICanGive)));

                bytesOffset[idxProc] = int(currentIntervalPosition);
                for(FSize idxLeaf = 0 ; idxLeaf < leavesToSend[idxProc] ; ++idxLeaf){
                    currentIntervalPosition += ((*(int*)&leavesArray[currentIntervalPosition]) * sizeof(ParticleClass)) + sizeof(int);
                }
                bytesToSend[idxProc] = int(currentIntervalPosition - bytesOffset[idxProc]);

                ICanGive -= leavesToSend[idxProc];
                ++idxProc;
            }
        }

        // Store the index of my data but we do not insert the now
        const FSize myParticlesPosition = currentIntervalPosition;
        {
            FTRACE( FTrace::FRegion regionTrace("Jump My particles", __FUNCTION__ , __FILE__ , __LINE__) );
            const FSize iNeedToSendLeftCount = correctLeftLeavesNumber - currentLeafsOnMyLeft;
            FSize endForMe = currentNbLeafs;
            if(iNeedToSendToRight){
                const FSize iNeedToSendRightCount = currentLeafsOnMyLeft + currentNbLeafs - correctRightLeavesIndex;
                endForMe -= iNeedToSendRightCount;
            }

            // We have to jump the correct number of leaves
            for(FSize idxLeaf = FMath::Max(iNeedToSendLeftCount,FSize(0)) ; idxLeaf < endForMe ; ++idxLeaf){
                const int nbPartInLeaf = (*(int*)&leavesArray[currentIntervalPosition]);
                currentIntervalPosition += (nbPartInLeaf * sizeof(ParticleClass)) + sizeof(int);
            }
        }

        // Proceed same on the right
        if(iNeedToSendToRight){
            FTRACE( FTrace::FRegion regionTrace("Calcul SendToRight", __FUNCTION__ , __FILE__ , __LINE__) );
            // Find the last proc on the right that need my data
            int idxProc = rank + 1;
            while( idxProc < nbProcs ){
                const FSize thisProcLeft = communicator.getOtherLeft(totalNbLeaves, idxProc);
                const FSize thisProcRight = communicator.getOtherRight(totalNbLeaves, idxProc);
                // Progress until the proc+1 has its left index upper to my current right
                if( thisProcLeft < currentLeafsOnMyLeft || (totalNbLeaves - currentLeafsOnMyRight) < thisProcRight){
                    break;
                }
                ++idxProc;
            }

            // Count the data
            int ICanGive = int(currentLeafsOnMyLeft + currentNbLeafs - correctRightLeavesIndex);
            leavesToSend[idxProc] = int(FMath::Min(communicator.getOtherRight(totalNbLeaves, idxProc) , (totalNbLeaves - currentLeafsOnMyRight))
                                        - FMath::Max(communicator.getOtherLeft(totalNbLeaves, idxProc), currentLeafsOnMyLeft) );

            {
                bytesOffset[idxProc] = int(currentIntervalPosition);
                for(FSize idxLeaf = 0 ; idxLeaf < leavesToSend[idxProc] ; ++idxLeaf){
                    currentIntervalPosition += ((*(int*)&leavesArray[currentIntervalPosition]) * sizeof(ParticleClass)) + sizeof(int);
                }
                bytesToSend[idxProc] = int(currentIntervalPosition - bytesOffset[idxProc]);
            }
            ICanGive -= leavesToSend[idxProc];
            ++idxProc;

            // Now Count the data to other
            while(idxProc < nbProcs && ICanGive){
                leavesToSend[idxProc] = int(FMath::Min( communicator.getOtherRight(totalNbLeaves, idxProc) - communicator.getOtherLeft(totalNbLeaves, idxProc), FSize(ICanGive)));

                bytesOffset[idxProc] = int(currentIntervalPosition);
                for(FSize idxLeaf = 0 ; idxLeaf < leavesToSend[idxProc] ; ++idxLeaf){
                    currentIntervalPosition += ((*(int*)&leavesArray[currentIntervalPosition]) * sizeof(ParticleClass)) + sizeof(int);
                }
                bytesToSend[idxProc] = int(currentIntervalPosition - bytesOffset[idxProc]);

                ICanGive -= leavesToSend[idxProc];
                ++idxProc;
            }
        }

        // Inform other about who will send/receive what
        int bytesToSendRecv[nbProcs * nbProcs];
        memset(bytesToSendRecv, 0, sizeof(int) * nbProcs * nbProcs);
        MPI_Allgather(bytesToSend, nbProcs, MPI_INT, bytesToSendRecv, nbProcs, MPI_INT, MPI_COMM_WORLD);

        int bytesToRecv[nbProcs];
        memset(bytesToRecv, 0, sizeof(int) * nbProcs);
        int bytesOffsetToRecv[nbProcs];
        memset(bytesOffsetToRecv, 0, sizeof(int) * nbProcs);

        // Prepare needed buffer
        FSize sumBytesToRecv = 0;
        for(int idxProc = 0 ; idxProc < nbProcs ; ++idxProc){
            if( bytesToSendRecv[idxProc * nbProcs + rank] ){
                bytesOffsetToRecv[idxProc] = int(sumBytesToRecv);
                sumBytesToRecv += FSize(bytesToSendRecv[idxProc * nbProcs + rank]);
                bytesToRecv[idxProc] = bytesToSendRecv[idxProc * nbProcs + rank];
            }
        }

        // Send alll to  all
        char* const recvbuf = new char[sumBytesToRecv];
        MPI_Alltoallv(leavesArray, bytesToSend, bytesOffset, MPI_BYTE,
                      recvbuf, bytesToRecv, bytesOffsetToRecv, MPI_BYTE,
                      MPI_COMM_WORLD);

        { // Insert received data
            FTRACE( FTrace::FRegion regionTrace("Insert Received data", __FUNCTION__ , __FILE__ , __LINE__) );
            FSize recvBufferPosition = 0;
            while( recvBufferPosition < sumBytesToRecv){
                const int nbPartInLeaf = (*reinterpret_cast<int*>(&recvbuf[recvBufferPosition]));
                ParticleClass* const particles = reinterpret_cast<ParticleClass*>(&recvbuf[recvBufferPosition] + sizeof(int));

                for(int idxPart = 0 ; idxPart < nbPartInLeaf ; ++idxPart){
                    realTree.insert(particles[idxPart]);
                }
                recvBufferPosition += (nbPartInLeaf * sizeof(ParticleClass)) + sizeof(int);

            }
        }
        delete[] recvbuf;


        { // Insert my data
            FTRACE( FTrace::FRegion regionTrace("Insert My particles", __FUNCTION__ , __FILE__ , __LINE__) );
            currentIntervalPosition = myParticlesPosition;
            const FSize iNeedToSendLeftCount = correctLeftLeavesNumber - currentLeafsOnMyLeft;
            FSize endForMe = currentNbLeafs;
            if(iNeedToSendToRight){
                const FSize iNeedToSendRightCount = currentLeafsOnMyLeft + currentNbLeafs - correctRightLeavesIndex;
                endForMe -= iNeedToSendRightCount;
            }

            for(FSize idxLeaf = FMath::Max(iNeedToSendLeftCount,FSize(0)) ; idxLeaf < endForMe ; ++idxLeaf){
                const int nbPartInLeaf = (*(int*)&leavesArray[currentIntervalPosition]);
                ParticleClass* const particles = reinterpret_cast<ParticleClass*>(&leavesArray[currentIntervalPosition] + sizeof(int));

                for(int idxPart = 0 ; idxPart < nbPartInLeaf ; ++idxPart){
                    realTree.insert(particles[idxPart]);
                }
                currentIntervalPosition += (nbPartInLeaf * sizeof(ParticleClass)) + sizeof(int);

            }
        }

        delete[] leavesArray;
        leavesArray = 0;
        nbLeavesInIntervals = 0;
    }

public:

    //////////////////////////////////////////////////////////////////////////
    // The builder function
    //////////////////////////////////////////////////////////////////////////

    template <class LoaderClass, class OctreeClass>
    static void LoaderToTree(const FMpi::FComm& communicator, LoaderClass& loader,
                             OctreeClass& realTree, const SortingType type = QuickSort){

        IndexedParticle* particlesArray = 0;
        FSize particlesSize = 0;
        SortParticles(communicator, loader, type, realTree.getHeight(), &particlesArray, &particlesSize);

        char* leavesArray = 0;
        FSize leavesSize = 0;
        MergeLeaves(communicator, particlesArray, particlesSize, &leavesArray, &leavesSize);

        EqualizeAndFillTree(communicator, realTree, leavesArray, leavesSize);
    }


};

#endif // FMPITREEBUILDER_H
