#ifndef FGROUPTREE_HPP
#define FGROUPTREE_HPP

#include "../Utils/FAssert.hpp"
#include "../Utils/FPoint.hpp"
#include "../Utils/FQuickSort.hpp"
#include "../Containers/FTreeCoordinate.hpp"

#include "FGroupOfCells.hpp"
#include "FGroupOfParticles.hpp"
#include "FGroupAttachedLeaf.hpp"

#include <list>
#include <functional>

template <class CellClass, class GroupAttachedLeafClass, unsigned NbAttributesPerParticle, class AttributeClass = FReal>
class FGroupTree {
public:
    typedef GroupAttachedLeafClass BasicAttachedClass;
    typedef FGroupOfParticles<NbAttributesPerParticle,AttributeClass> ParticleGroupClass;
    typedef FGroupOfCells<CellClass> CellGroupClass;

protected:
    //< This value is for not used cells
    static const int CellIsEmptyFlag = -1;

    //< height of the tree (1 => only the root)
    const int treeHeight;
    //< max number of cells in a block
    const int nbElementsPerBlock;
    //< all the blocks of the tree
    std::list<CellGroupClass*>* cellBlocksPerLevel;
    //< all the blocks of leaves
    std::list<ParticleGroupClass*> particleBlocks;

    //< the space system center
    const FPoint boxCenter;
    //< the space system corner (used to compute morton index)
    const FPoint boxCorner;
    //< the space system width
    const FReal boxWidth;
    //< the width of a box at width level
    const FReal boxWidthAtLeafLevel;



    int getTreeCoordinate(const FReal inRelativePosition) const {
        FAssertLF( (inRelativePosition >= 0 && inRelativePosition <= this->boxWidth), "inRelativePosition : ",inRelativePosition );
        if(inRelativePosition == this->boxWidth){
            return FMath::pow2(treeHeight-1)-1;
        }
        const FReal indexFReal = inRelativePosition / boxWidthAtLeafLevel;
        return static_cast<int>(indexFReal);
    }

    FTreeCoordinate getCoordinateFromPosition(const FReal xpos,const FReal ypos,const FReal zpos) const {
        // box coordinate to host the particle
        FTreeCoordinate host;
        // position has to be relative to corner not center
        host.setX( getTreeCoordinate( xpos - this->boxCorner.getX() ));
        host.setY( getTreeCoordinate( ypos - this->boxCorner.getY() ));
        host.setZ( getTreeCoordinate( zpos - this->boxCorner.getZ() ));
        return host;
    }

public:
    /** This constructor create a blocked octree from a usual octree
   * The cell are allocated as in the usual octree (no copy constructor are called!)
   * Once allocated each cell receive its morton index and tree coordinate.
   * No blocks are allocated at level 0.
   */
    template<class OctreeClass>
    FGroupTree(const int inTreeHeight, const int inNbElementsPerBlock, OctreeClass*const inOctreeSrc)
        : treeHeight(inTreeHeight), nbElementsPerBlock(inNbElementsPerBlock), cellBlocksPerLevel(0),
          boxCenter(inOctreeSrc->getBoxCenter()), boxCorner(inOctreeSrc->getBoxCenter(),-(inOctreeSrc->getBoxWidth()/2)),
          boxWidth(inOctreeSrc->getBoxWidth()), boxWidthAtLeafLevel(inOctreeSrc->getBoxWidth()/FReal(1<<inTreeHeight)){
        cellBlocksPerLevel = new std::list<CellGroupClass*>[treeHeight];

        // Iterate on the tree and build
        typename OctreeClass::Iterator octreeIterator(inOctreeSrc);
        octreeIterator.gotoBottomLeft();

        { // First leaf level, we create leaves and cells groups
            const int idxLevel = treeHeight-1;
            typename OctreeClass::Iterator avoidGotoLeft = octreeIterator;
            // For each cell at this level
            do {
                typename OctreeClass::Iterator blockIteratorInOctree = octreeIterator;
                // Move the iterator per nbElementsPerBlock (or until it cannot move right)
                int sizeOfBlock = 1;
                int nbParticlesInGroup = octreeIterator.getCurrentLeaf()->getSrc()->getNbParticles();
                while(sizeOfBlock < nbElementsPerBlock && octreeIterator.moveRight()){
                    sizeOfBlock += 1;
                    nbParticlesInGroup += octreeIterator.getCurrentLeaf()->getSrc()->getNbParticles();
                }

                // Create a block with the apropriate parameters
                CellGroupClass*const newBlock = new CellGroupClass(blockIteratorInOctree.getCurrentGlobalIndex(),
                                                                 octreeIterator.getCurrentGlobalIndex()+1,
                                                                 sizeOfBlock);
                FGroupOfParticles<NbAttributesPerParticle, AttributeClass>*const newParticleBlock = new FGroupOfParticles<NbAttributesPerParticle, AttributeClass>(blockIteratorInOctree.getCurrentGlobalIndex(),
                                                                 octreeIterator.getCurrentGlobalIndex()+1,
                                                                 sizeOfBlock, nbParticlesInGroup);

                // Initialize each cell of the block
                int cellIdInBlock = 0;
                int nbParticlesBeforeLeaf = 0;
                while(cellIdInBlock != sizeOfBlock){
                    // Add cell
                    const CellClass*const oldNode = blockIteratorInOctree.getCurrentCell();
                    newBlock->newCell(oldNode->getMortonIndex(), cellIdInBlock);

                    CellClass* newNode = newBlock->getCell(oldNode->getMortonIndex());
                    newNode->setMortonIndex(oldNode->getMortonIndex());
                    newNode->setCoordinate(oldNode->getCoordinate());

                    // Add leaf
                    newParticleBlock->newLeaf(oldNode->getMortonIndex(), cellIdInBlock,
                                              blockIteratorInOctree.getCurrentLeaf()->getSrc()->getNbParticles(),
                                              nbParticlesBeforeLeaf);

                    BasicAttachedClass attachedLeaf = newParticleBlock->template getLeaf<BasicAttachedClass>(oldNode->getMortonIndex());
                    attachedLeaf.copyFromContainer(blockIteratorInOctree.getCurrentLeaf()->getSrc(), 0);

                    nbParticlesBeforeLeaf += blockIteratorInOctree.getCurrentLeaf()->getSrc()->getNbParticles();
                    cellIdInBlock += 1;
                    blockIteratorInOctree.moveRight();
                }

                // Keep the block
                cellBlocksPerLevel[idxLevel].push_back(newBlock);
                particleBlocks.push_back(newParticleBlock);

                // If we can move right then add another block
            } while(octreeIterator.moveRight());

            avoidGotoLeft.moveUp();
            octreeIterator = avoidGotoLeft;
        }

        // For each level from heigth - 2 to 1
        for(int idxLevel = treeHeight-2; idxLevel > 0 ; --idxLevel){
            typename OctreeClass::Iterator avoidGotoLeft = octreeIterator;
            // For each cell at this level
            do {
                typename OctreeClass::Iterator blockIteratorInOctree = octreeIterator;
                // Move the iterator per nbElementsPerBlock (or until it cannot move right)
                int sizeOfBlock = 1;
                while(sizeOfBlock < nbElementsPerBlock && octreeIterator.moveRight()){
                    sizeOfBlock += 1;
                }

                // Create a block with the apropriate parameters
                CellGroupClass*const newBlock = new CellGroupClass(blockIteratorInOctree.getCurrentGlobalIndex(),
                                                                 octreeIterator.getCurrentGlobalIndex()+1,
                                                                 sizeOfBlock);
                // Initialize each cell of the block
                int cellIdInBlock = 0;
                while(cellIdInBlock != sizeOfBlock){
                    const CellClass*const oldNode = blockIteratorInOctree.getCurrentCell();
                    newBlock->newCell(oldNode->getMortonIndex(), cellIdInBlock);

                    CellClass* newNode = newBlock->getCell(oldNode->getMortonIndex());
                    newNode->setMortonIndex(oldNode->getMortonIndex());
                    newNode->setCoordinate(oldNode->getCoordinate());

                    cellIdInBlock += 1;
                    blockIteratorInOctree.moveRight();
                }

                // Keep the block
                cellBlocksPerLevel[idxLevel].push_back(newBlock);

                // If we can move right then add another block
            } while(octreeIterator.moveRight());

            avoidGotoLeft.moveUp();
            octreeIterator = avoidGotoLeft;
        }
    }


    /**
     * This constructor create a group tree from a particle container index.
     * The morton index are computed and the particles are sorted in a first stage.
     * Then the leaf level is done.
     * Finally the other leve are proceed one after the other.
     * It should be easy to make it parallel using for and tasks.
     */
    template<class ParticleContainer>
    FGroupTree(const int inTreeHeight, const FReal inBoxWidth, const FPoint& inBoxCenter,
               const int inNbElementsPerBlock, ParticleContainer* inParticlesContainer, const bool particlesAreSorted = false):
            treeHeight(inTreeHeight),nbElementsPerBlock(inNbElementsPerBlock),cellBlocksPerLevel(0),
            boxCenter(inBoxCenter), boxCorner(inBoxCenter,-(inBoxWidth/2)), boxWidth(inBoxWidth),
            boxWidthAtLeafLevel(inBoxWidth/FReal(1<<inTreeHeight)){

        cellBlocksPerLevel = new std::list<CellGroupClass*>[treeHeight];

        MortonIndex* currentBlockIndexes = new MortonIndex[nbElementsPerBlock];
        // First we work at leaf level
        {
            // Build morton index for particles
            struct ParticleSortingStruct{
                int originalIndex;
                MortonIndex mindex;

                operator MortonIndex(){
                    return mindex;
                }
            };
            // Convert position to morton index
            const int nbParticles = inParticlesContainer->getNbParticles();
            ParticleSortingStruct* particlesToSort = new ParticleSortingStruct[nbParticles];
            {
                const FReal* xpos = inParticlesContainer->getPositions()[0];
                const FReal* ypos = inParticlesContainer->getPositions()[1];
                const FReal* zpos = inParticlesContainer->getPositions()[2];

                for(int idxPart = 0 ; idxPart < nbParticles ; ++idxPart){
                    const FTreeCoordinate host = getCoordinateFromPosition( xpos[idxPart], ypos[idxPart], zpos[idxPart] );
                    const MortonIndex particleIndex = host.getMortonIndex(treeHeight-1);
                    particlesToSort[idxPart].mindex = particleIndex;
                    particlesToSort[idxPart].originalIndex = idxPart;
                }
            }

            // Sort if needed
            if(particlesAreSorted == false){
                FQuickSort<ParticleSortingStruct, MortonIndex, int>::QsOmp(particlesToSort, nbParticles);
            }

            // Convert to block
            const int idxLevel = (treeHeight - 1);
            int* nbParticlesPerLeaf = new int[nbElementsPerBlock];
            int firstParticle = 0;
            // We need to proceed each group in sub level
            while(firstParticle != nbParticles){
                int sizeOfBlock = 0;
                int lastParticle = firstParticle + 1;
                // Count until end of sub group is reached or we have enough cells
                while(sizeOfBlock < nbElementsPerBlock && lastParticle < nbParticles){
                    if(sizeOfBlock == 0 || currentBlockIndexes[sizeOfBlock-1] != particlesToSort[lastParticle].mindex){
                        currentBlockIndexes[sizeOfBlock] = particlesToSort[lastParticle].mindex;
                        nbParticlesPerLeaf[sizeOfBlock]  = 1;
                        sizeOfBlock += 1;
                    }
                    else{
                        nbParticlesPerLeaf[sizeOfBlock-1] += 1;
                    }
                    lastParticle += 1;
                }

                // Create a group
                CellGroupClass*const newBlock = new CellGroupClass(currentBlockIndexes[0],
                                                                 currentBlockIndexes[sizeOfBlock-1]+1,
                                                                 sizeOfBlock);
                FGroupOfParticles<NbAttributesPerParticle, AttributeClass>*const newParticleBlock = new FGroupOfParticles<NbAttributesPerParticle, AttributeClass>(currentBlockIndexes[0],
                        currentBlockIndexes[sizeOfBlock-1]+1,
                        sizeOfBlock, lastParticle-firstParticle);

                // Init cells
                int nbParticlesBeforeLeaf = 0;
                for(int cellIdInBlock = 0; cellIdInBlock != sizeOfBlock ; ++cellIdInBlock){
                    newBlock->newCell(currentBlockIndexes[cellIdInBlock], cellIdInBlock);

                    CellClass* newNode = newBlock->getCell(currentBlockIndexes[cellIdInBlock]);
                    newNode->setMortonIndex(currentBlockIndexes[cellIdInBlock]);
                    FTreeCoordinate coord;
                    coord.setPositionFromMorton(currentBlockIndexes[cellIdInBlock], idxLevel);
                    newNode->setCoordinate(coord);

                    // Add leaf
                    newParticleBlock->newLeaf(currentBlockIndexes[cellIdInBlock], cellIdInBlock,
                                              nbParticlesPerLeaf[cellIdInBlock], nbParticlesBeforeLeaf);

                    BasicAttachedClass attachedLeaf = newParticleBlock->template getLeaf<BasicAttachedClass>(currentBlockIndexes[cellIdInBlock]);
                    // Copy each particle from the original position
                    for(int idxPart = 0 ; idxPart < nbParticlesPerLeaf[cellIdInBlock] ; ++idxPart){
                        attachedLeaf.setParticle(idxPart, particlesToSort[idxPart + firstParticle].originalIndex, inParticlesContainer);
                    }

                    nbParticlesBeforeLeaf += nbParticlesPerLeaf[cellIdInBlock];
                }

                // Keep the block
                cellBlocksPerLevel[idxLevel].push_back(newBlock);
                particleBlocks.push_back(newParticleBlock);

                sizeOfBlock = 0;
                firstParticle = lastParticle;
            }
            delete[] nbParticlesPerLeaf;
            delete[] particlesToSort;
        }

        // For each level from heigth - 2 to 1
        for(int idxLevel = treeHeight-2; idxLevel > 0 ; --idxLevel){
            typename std::list<CellGroupClass*>::const_iterator iterChildCells = cellBlocksPerLevel[idxLevel+1].begin();
            const typename std::list<CellGroupClass*>::const_iterator iterChildEndCells = cellBlocksPerLevel[idxLevel+1].end();

            MortonIndex currentCellIndex = (*iterChildCells)->getStartingIndex();
            int sizeOfBlock = 0;

            // We need to proceed each group in sub level
            while(iterChildCells != iterChildEndCells){
                // Count until end of sub group is reached or we have enough cells
                while(sizeOfBlock < nbElementsPerBlock && currentCellIndex != (*iterChildCells)->getEndingIndex()){
                    if((sizeOfBlock == 0 || currentBlockIndexes[sizeOfBlock-1] != (currentCellIndex>>3))
                            && (*iterChildCells)->exists(currentCellIndex)){
                        currentBlockIndexes[sizeOfBlock] = (currentCellIndex>>3);
                        sizeOfBlock += 1;
                    }
                    currentCellIndex += 1;
                }

                // If we are at the end of the sub group, move to next
                if(currentCellIndex == (*iterChildCells)->getEndingIndex()){
                    ++iterChildCells;
                    // Update morton index
                    if(iterChildCells != iterChildEndCells){
                        currentCellIndex = (*iterChildCells)->getStartingIndex();
                    }
                }

                // If group is full
                if(sizeOfBlock == nbElementsPerBlock || (sizeOfBlock && iterChildCells == iterChildEndCells)){
                    // Create a group
                    CellGroupClass*const newBlock = new CellGroupClass(currentBlockIndexes[0],
                                                                     currentBlockIndexes[sizeOfBlock-1]+1,
                                                                     sizeOfBlock);
                    // Init cells
                    for(int cellIdInBlock = 0; cellIdInBlock != sizeOfBlock ; ++cellIdInBlock){
                        newBlock->newCell(currentBlockIndexes[cellIdInBlock], cellIdInBlock);

                        CellClass* newNode = newBlock->getCell(currentBlockIndexes[cellIdInBlock]);
                        newNode->setMortonIndex(currentBlockIndexes[cellIdInBlock]);
                        FTreeCoordinate coord;
                        coord.setPositionFromMorton(currentBlockIndexes[cellIdInBlock], idxLevel);
                        newNode->setCoordinate(coord);
                    }

                    // Keep the block
                    cellBlocksPerLevel[idxLevel].push_back(newBlock);

                    sizeOfBlock = 0;
                }
            }
        }
        delete[] currentBlockIndexes;
    }


    ////////////////////////////////////////////////////////////////////
    // Can be deleted
    ////////////////////////////////////////////////////////////////////

    /** @brief This private method take an array of Morton index to
   * create the block and the cells, using the constructor of
   * FGroupOfCells !!
   */
    /*CellGroupClass * createBlockFromArray(MortonIndex head[]){
        //Store the start and end
        MortonIndex start = head[0];
        MortonIndex end = start;
        int count = 0;
        // Find the number of cell to allocate in the blocks
        for(int idxHead = 0 ; idxHead<nbElementsPerBlock ; idxHead++){
            if (head[idxHead] != CellIsEmptyFlag){
                count++;
                end = head[idxHead];
            }
            // else{
            // 	break;
            // }
        }
        //allocation of memory
        CellGroupClass * newBlock = new CellGroupClass(start,end+1,count);
        //allocation of cells
        for(int idx=0 ; idx<count ; idx++){
            newBlock->newCell(head[idx], idx);
            (newBlock->getCell(head[idx]))->setMortonIndex(head[idx]);
            //(this->getCell(head[idx]))->setCoordinate();
        }
        return newBlock;
    }*/

    /** This constructor build the BlockOctree from an Octree, but only
   * the cells at leaf level are read. The cells ares constructed with
   * several walk through the leafs.
   */
    /*template<class OctreeClass>
    FGroupTree(const int inTreeHeight, const int inNbElementsPerBlock, OctreeClass*const inOctreeSrc, int FLAG):
        treeHeight(inTreeHeight),nbElementsPerBlock(inNbElementsPerBlock),cellBlocksPerLevel(0)
    {
        cellBlocksPerLevel = new std::list<CellGroupClass*>[treeHeight];
        int *nbCellPerLevel = new int[treeHeight];
        inOctreeSrc->getNbCellsPerLevel(nbCellPerLevel);
        int nbLeaf = nbCellPerLevel[treeHeight-1];
        delete[] nbCellPerLevel;

        //We get the Morton index of all the cells at leaf level and store
        //them in order to build our blocks
        MortonIndex *leafsIdx = new MortonIndex[nbLeaf];
        int idxLeafs = 0;
        //Iterator on source octree
        typename OctreeClass::Iterator octreeIterator(inOctreeSrc);
        octreeIterator.gotoBottomLeft();
        do{
            leafsIdx[idxLeafs] = octreeIterator.getCurrentGlobalIndex();
            idxLeafs++;
        }
        while(octreeIterator.moveRight());

        //Now the work start !!1!
        idxLeafs = 0;

        //Temporary Header, will be filled
        MortonIndex * head = new MortonIndex[inNbElementsPerBlock];
        memset(head,CellIsEmptyFlag,sizeof(MortonIndex)*inNbElementsPerBlock);
        //Creation and addition of all the cells at leaf level
        while(idxLeafs < nbLeaf){

            //Initialisation of header
            memset(head,CellIsEmptyFlag,sizeof(MortonIndex)*inNbElementsPerBlock);
            memcpy(head,&leafsIdx[idxLeafs],FMath::Min(sizeof(MortonIndex)*inNbElementsPerBlock,sizeof(MortonIndex)*(nbLeaf-idxLeafs)));
            idxLeafs += inNbElementsPerBlock;

            //creation of the block and addition to the list
            CellGroupClass * tempBlock = createBlockFromArray(head);
            cellBlocksPerLevel[treeHeight-1].push_back(tempBlock);
        }
        delete[] leafsIdx;

        //Creation of the cells at others level
        //Loop over levels
        for(int idxLevel = treeHeight-2 ; idxLevel >= 0 ; --idxLevel){

            //Set the storing system (a simple array) to CellIsEmptyFlag
            memset(head,CellIsEmptyFlag,sizeof(MortonIndex)*inNbElementsPerBlock);
            int idxHead = -1;
            MortonIndex previous = -1;

            //Iterator over the list at a deeper level (READ)
            typename std::list<CellGroupClass*>::const_iterator curBlockRead;
            for(curBlockRead = cellBlocksPerLevel[idxLevel+1].begin() ; curBlockRead != cellBlocksPerLevel[idxLevel+1].end() ; ++curBlockRead){
                //Loop over cells in READ list
                for(MortonIndex idxCell = (*curBlockRead)->getStartingIndex() ; idxCell < (*curBlockRead)->getEndingIndex() ; ++idxCell){

                    MortonIndex newIdx(idxCell >> 3);
                    //printf("Lvl : %d Current Idx %lld, parent's one : %lld \n",idxLevel,idxCell,newIdx);
                    //For Head Increment, we need to know if this parent has
                    //already been created
                    if(newIdx != previous){
                        //test if (Read cell exists)
                        if((*curBlockRead)->exists(idxCell)){
                            idxHead++; //cell at deeper level exist, so we increment the count to store it

                            //Test if a new Block is necessary
                            if(idxHead < inNbElementsPerBlock){
                                //No need for a Block, just the cell is created
                                head[idxHead] = newIdx;
                                previous = newIdx;
                            }
                            else{
                                //Creation of the block from head, then reset head, and
                                //storage of new idx in new head
                                CellGroupClass * tempBlock = createBlockFromArray(head);
                                cellBlocksPerLevel[idxLevel].push_back(tempBlock);

                                //Need a new block
                                memset(head,CellIsEmptyFlag,sizeof(MortonIndex)*nbElementsPerBlock);
                                head[0] = newIdx;
                                idxHead = 0;
                                previous = newIdx;
                            }
                        }
                    }
                }
            }
            //Before changing Level, need to close current Block
            CellGroupClass * tempBlock = createBlockFromArray(head);
            cellBlocksPerLevel[idxLevel].push_back(tempBlock);
        }
        printf("toto \n");
        delete[] head;
    }*/

    ////////////////////////////////////////////////////////////////////
    // End of can be deleted
    ////////////////////////////////////////////////////////////////////


    /** This function dealloc the tree by deleting each block */
    ~FGroupTree(){
        for(int idxLevel = 0 ; idxLevel < treeHeight ; ++idxLevel){
            std::list<CellGroupClass*>& levelBlocks = cellBlocksPerLevel[idxLevel];
            for (CellGroupClass* block: levelBlocks){
                delete block;
            }
        }
        delete[] cellBlocksPerLevel;

        for (ParticleGroupClass* block: particleBlocks){
            delete block;
        }
    }


    /////////////////////////////////////////////////////////
    // Lambda function to apply to all member
    /////////////////////////////////////////////////////////

    /**
   * @brief forEachLeaf iterate on the leaf and apply the function
   * @param function
   */
    template<class ParticlesAttachedClass>
    void forEachLeaf(std::function<void(ParticlesAttachedClass*)> function){
        for (ParticleGroupClass* block: particleBlocks){
            block->forEachLeaf(function);
        }
    }

    /**
   * @brief forEachLeaf iterate on the cell and apply the function
   * @param function
   */
    void forEachCell(std::function<void(CellClass*)> function){
        for(int idxLevel = 0 ; idxLevel < treeHeight ; ++idxLevel){
            std::list<CellGroupClass*>& levelBlocks = cellBlocksPerLevel[idxLevel];
            for (CellGroupClass* block: levelBlocks){
                block->forEachCell(function);
            }
        }
    }

    /**
   * @brief forEachLeaf iterate on the cell and apply the function
   * @param function
   */
    void forEachCellWithLevel(std::function<void(CellClass*,const int)> function){
        for(int idxLevel = 0 ; idxLevel < treeHeight ; ++idxLevel){
            std::list<CellGroupClass*>& levelBlocks = cellBlocksPerLevel[idxLevel];
            for (CellGroupClass* block: levelBlocks){
                block->forEachCell(function, idxLevel);
            }
        }
    }

    /**
   * @brief forEachLeaf iterate on the cell and apply the function
   * @param function
   */
    template<class ParticlesAttachedClass>
    void forEachCellLeaf(std::function<void(CellClass*,ParticlesAttachedClass*)> function){
        typename std::list<CellGroupClass*>::iterator iterCells = cellBlocksPerLevel[treeHeight-1].begin();
        const typename std::list<CellGroupClass*>::iterator iterEndCells = cellBlocksPerLevel[treeHeight-1].end();

        typename std::list<ParticleGroupClass*>::iterator iterLeaves = particleBlocks.begin();
        const typename std::list<ParticleGroupClass*>::iterator iterEndLeaves = particleBlocks.end();

        while(iterCells != iterEndCells && iterLeaves != iterEndLeaves){
            (*iterCells)->forEachCell([&](CellClass* aCell){
                ParticlesAttachedClass aLeaf = (*iterLeaves)->getLeaf(aCell->getMortonIndex());
                FAssertLF(aLeaf.isAttachedToSomething());
                function(aCell, aLeaf);
                delete aLeaf;
            });

            ++iterCells;
            ++iterLeaves;
        }

        FAssertLF(iterCells == iterEndCells && iterLeaves == iterEndLeaves);
    }



    /** @brief, for statistic purpose, display each block with number of
   * cell, size of header, starting index, and ending index
   */
    void printInfoBlocks(){
        std::cout << "Group Tree information:\n";
        std::cout << "\t Group Size = " << nbElementsPerBlock << "\n";
        std::cout << "\t Tree height = " << treeHeight << "\n";
        for(int idxLevel = 1 ; idxLevel < treeHeight ; ++idxLevel){
            std::list<CellGroupClass*>& levelBlocks = cellBlocksPerLevel[idxLevel];
            std::cout << "Level " << idxLevel << ", there are " << levelBlocks.size() << " groups.\n";
            int idxGroup = 0;
            for (const CellGroupClass* block: levelBlocks){
                std::cout << "\t Group " << (idxGroup++);
                std::cout << "\t Size = " << block->getNumberOfCellsInBlock();
                std::cout << "\t Starting Index = " << block->getStartingIndex();
                std::cout << "\t Ending Index = " << block->getEndingIndex();
                std::cout << "\t Ratio of usage = " << float(block->getNumberOfCellsInBlock())/float(block->getEndingIndex()-block->getStartingIndex()) << "\n";
            }
        }

        std::cout << "There are " << particleBlocks.size() << " leaf-groups.\n";
        int idxGroup = 0;
        int totalNbParticles = 0;
        for (const ParticleGroupClass* block: particleBlocks){
            std::cout << "\t Group " << (idxGroup++);
            std::cout << "\t Size = " << block->getNumberOfLeavesInBlock();
            std::cout << "\t Starting Index = " << block->getStartingIndex();
            std::cout << "\t Ending Index = " << block->getEndingIndex();
            std::cout << "\t Nb Particles = " << block->getNbParticlesInGroup();
            std::cout << "\t Ratio of usage = " << float(block->getNumberOfLeavesInBlock())/float(block->getEndingIndex()-block->getStartingIndex()) << "\n";
            totalNbParticles += block->getNbParticlesInGroup();
        }
        std::cout << "There are " << totalNbParticles << " particles.\n";
    }

    /////////////////////////////////////////////////////////
    // Algorithm function
    /////////////////////////////////////////////////////////

    int getHeight() const {
        return treeHeight;
    }

    typename std::list<CellGroupClass*>::iterator cellsBegin(const int atHeight){
        FAssertLF(atHeight < treeHeight);
        return cellBlocksPerLevel[atHeight].begin();
    }

    typename std::list<CellGroupClass*>::const_iterator cellsBegin(const int atHeight) const {
        FAssertLF(atHeight < treeHeight);
        return cellBlocksPerLevel[atHeight].begin();
    }

    typename std::list<CellGroupClass*>::iterator cellsEnd(const int atHeight){
        FAssertLF(atHeight < treeHeight);
        return cellBlocksPerLevel[atHeight].end();
    }

    typename std::list<CellGroupClass*>::const_iterator cellsEnd(const int atHeight) const {
        FAssertLF(atHeight < treeHeight);
        return cellBlocksPerLevel[atHeight].end();
    }


    typename std::list<ParticleGroupClass*>::iterator leavesBegin(){
        return particleBlocks.begin();
    }

    typename std::list<ParticleGroupClass*>::const_iterator leavesBegin() const {
        return particleBlocks.begin();
    }

    typename std::list<ParticleGroupClass*>::iterator leavesEnd(){
        return particleBlocks.end();
    }

    typename std::list<ParticleGroupClass*>::const_iterator leavesEnd() const {
        return particleBlocks.end();
    }
};

#endif // FGROUPTREE_HPP
