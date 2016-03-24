
// Keep in private GIT
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include "../../Src/Utils/FGlobal.hpp"

#include "../../Src/GroupTree/Core/FGroupTree.hpp"

#include "../../Src/Components/FSimpleLeaf.hpp"
#include "../../Src/Containers/FVector.hpp"


#include "../../Src/Utils/FMath.hpp"
#include "../../Src/Utils/FMemUtils.hpp"
#include "../../Src/Utils/FParameters.hpp"

#include "../../Src/Files/FRandomLoader.hpp"

#include "../../Src/GroupTree/Core/FGroupSeqAlgorithm.hpp"
#include "../../Src/GroupTree/Core/FGroupTaskStarpuImplicitAlgorithm.hpp"
#include "../../Src/GroupTree/StarPUUtils/FStarPUKernelCapacities.hpp"

#include "../../Src/GroupTree/StarPUUtils/FStarPUCpuWrapper.hpp"
#include "../../Src/GroupTree/Core/FP2PGroupParticleContainer.hpp"
#include "../../Src/GroupTree/Core/FGroupTaskAlgorithm.hpp"

#include "../../Src/BalanceTree/FLeafBalance.hpp"

#include "../../Src/Utils/FParameterNames.hpp"

#include "../../Src/Components/FTestParticleContainer.hpp"
#include "../../Src/Components/FTestCell.hpp"
#include "../../Src/Components/FTestKernels.hpp"
#include "../../Src/GroupTree/TestKernel/FGroupTestParticleContainer.hpp"
#include "../../Src/GroupTree/TestKernel/FTestCellPOD.hpp"

#include "../../Src/Files/FFmaGenericLoader.hpp"
#include "../../Src/Core/FFmmAlgorithm.hpp"

    typedef double FReal;

    // Initialize the types
    typedef FTestCellPODCore  GroupCellSymbClass;
    typedef FTestCellPODData  GroupCellUpClass;
    typedef FTestCellPODData  GroupCellDownClass;
    typedef FTestCellPOD      GroupCellClass;


    typedef FGroupTestParticleContainer<FReal>                                GroupContainerClass;
    typedef FGroupTree< FReal, GroupCellClass, GroupCellSymbClass, GroupCellUpClass, GroupCellDownClass,
            GroupContainerClass, 0, 1, long long int>  GroupOctreeClass;
    typedef FStarPUAllCpuCapacities<FTestKernels< GroupCellClass, GroupContainerClass >>  GroupKernelClass;
    typedef FStarPUCpuWrapper<typename GroupOctreeClass::CellGroupClass, GroupCellClass, GroupKernelClass, typename GroupOctreeClass::ParticleGroupClass, GroupContainerClass> GroupCpuWrapper;
    typedef FGroupTaskStarPUImplicitAlgorithm<GroupOctreeClass, typename GroupOctreeClass::CellGroupClass, GroupKernelClass, typename GroupOctreeClass::ParticleGroupClass, GroupCpuWrapper > GroupAlgorithm;

    typedef FTestCell                   CellClass;
    typedef FTestParticleContainer<FReal>      ContainerClass;
    typedef FSimpleLeaf<FReal, ContainerClass >                     LeafClass;
    typedef FOctree<FReal, CellClass, ContainerClass , LeafClass >  OctreeClass;
    typedef FTestKernels< CellClass, ContainerClass >         KernelClass;

    // FFmmAlgorithmTask FFmmAlgorithmThread
    typedef FFmmAlgorithm<OctreeClass, CellClass, ContainerClass, KernelClass, LeafClass >     FmmClass;
#define LOAD_FILE
#ifndef LOAD_FILE
	typedef FRandomLoader<FReal> LoaderClass;
#else
	typedef FFmaGenericLoader<FReal> LoaderClass;
#endif
std::vector<MortonIndex> getMortonIndex(const char* const mapping_filename);
void timeAverage(int mpi_rank, int nproc, double elapsedTime);
void sortParticle(FPoint<FReal> * allParticlesToSort, int treeHeight, int groupSize, vector<vector<int>> & sizeForEachGroup, LoaderClass& loader, int nproc);
void createNodeRepartition(std::vector<MortonIndex> distributedMortonIndex, std::vector<std::vector<std::vector<MortonIndex>>>& nodeRepartition, int nproc, int treeHeight);

int main(int argc, char* argv[]){
    setenv("STARPU_NCPU","1",1);
    const FParameterNames LocalOptionBlocSize {
        {"-bs"},
        "The size of the block of the blocked tree"
    };
	const FParameterNames Mapping {
		{"-map"} ,
		"mapping  \\o/."
	};
    FHelpDescribeAndExit(argc, argv, "Test the blocked tree by counting the particles.",
                         FParameterDefinitions::OctreeHeight, FParameterDefinitions::NbParticles,
                         FParameterDefinitions::OctreeSubHeight, FParameterDefinitions::InputFile, LocalOptionBlocSize, Mapping);
	//int provided = 0;
	//MPI_Init_thread(&argc,&argv, MPI_THREAD_SERIALIZED, &provided);


    // Get params
    const int NbLevels      = FParameters::getValue(argc,argv,FParameterDefinitions::OctreeHeight.options, 5);
    const int groupSize      = FParameters::getValue(argc,argv,LocalOptionBlocSize.options, 8);
    const char* const mapping_filename      = FParameters::getStr(argc,argv,Mapping.options, "mapping");
	std::vector<MortonIndex> distributedMortonIndex = getMortonIndex(mapping_filename);

#ifndef STARPU_USE_MPI
		cout << "Pas de mpi -_-\" " << endl;
#endif
#ifndef LOAD_FILE
    const FSize NbParticles   = FParameters::getValue(argc,argv,FParameterDefinitions::NbParticles.options, FSize(20));
    LoaderClass loader(NbParticles, 1.0, FPoint<FReal>(0,0,0), 0);
#else
    // Load the particles
    const char* const filename = FParameters::getStr(argc,argv,FParameterDefinitions::InputFile.options, "../Data/test20k.fma");
    LoaderClass loader(filename);
#endif
    FAssertLF(loader.isOpen());

    // Usual octree
    OctreeClass tree(NbLevels, FParameters::getValue(argc,argv,FParameterDefinitions::OctreeSubHeight.options, 2),
                     loader.getBoxWidth(), loader.getCenterOfBox());
    FTestParticleContainer<FReal> allParticles;
	FPoint<FReal> allParticlesToSort[loader.getNumberOfParticles()];
    for(FSize idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
        loader.fillParticle(&allParticlesToSort[idxPart]);//Same with file or not
    }

	vector<vector<int>> sizeForEachGroup;
	sortParticle(allParticlesToSort, NbLevels, groupSize, sizeForEachGroup, loader, 8);

    for(FSize idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
        allParticles.push(allParticlesToSort[idxPart]);
        tree.insert(allParticlesToSort[idxPart]);
	}
	
    // Put the data into the tree
    //GroupOctreeClass groupedTree(NbLevels, groupSize, &tree);
	GroupOctreeClass groupedTree(NbLevels, loader.getBoxWidth(), loader.getCenterOfBox(), groupSize, &allParticles, false);
	
    // Check tree structure at leaf level
    groupedTree.forEachCellLeaf<GroupContainerClass>([&](GroupCellClass gcell, GroupContainerClass* gleaf){
        const ContainerClass* src = tree.getLeafSrc(gcell.getMortonIndex());
        if(src == nullptr){
            std::cout << "[PartEmpty] Error cell should not exist " << gcell.getMortonIndex() << "\n";
        }
        else {
            if(src->getNbParticles() != gleaf->getNbParticles()){
                std::cout << "[Part] Nb particles is different at index " << gcell.getMortonIndex() << " is " << gleaf->getNbParticles() << " should be " << src->getNbParticles() << "\n";
            }
        }
    });

    // Run the algorithm
    GroupKernelClass groupkernel;
    GroupAlgorithm groupalgo(&groupedTree,&groupkernel, distributedMortonIndex);
	FTic timerExecute;
    groupalgo.execute();
	double elapsedTime = timerExecute.tacAndElapsed();
	cout << "Executing time (implicit node " << groupalgo.getRank() << ") " << elapsedTime << "s\n";
	timeAverage(groupalgo.getRank(), groupalgo.getNProc(), elapsedTime);

    // Usual algorithm
    KernelClass kernels;            // FTestKernels FBasicKernels
    FmmClass algo(&tree,&kernels);  //FFmmAlgorithm FFmmAlgorithmThread
    algo.execute();
	int rank = groupalgo.getRank();
	for(int i = 2; i < groupedTree.getHeight(); ++i)//No task at level 0 and 1
	{
		if(groupedTree.getNbCellGroupAtLevel(i) < groupalgo.getNProc() && rank == 0)
			std::cout << "Error at level " << i << std::endl;
	}
    // Validate the result
	for(int idxLevel = 2 ; idxLevel < groupedTree.getHeight() ; ++idxLevel){
		for(int idxGroup = 0 ; idxGroup < groupedTree.getNbCellGroupAtLevel(idxLevel) ; ++idxGroup){
			//if(groupalgo.isDataOwned(idxGroup, groupedTree.getNbCellGroupAtLevel(idxLevel))){
			if(groupalgo.isDataOwnedBerenger(groupedTree.getCellGroup(idxLevel, idxGroup)->getStartingIndex(), idxLevel)){
				GroupOctreeClass::CellGroupClass* currentCells = groupedTree.getCellGroup(idxLevel, idxGroup);
				currentCells->forEachCell([&](GroupCellClass gcell){
						const CellClass* cell = tree.getCell(gcell.getMortonIndex(), idxLevel);
						if(cell == nullptr){
							std::cout << "[Empty node(" << rank << ")] Error cell should not exist " << gcell.getMortonIndex() << "\n";
						}
						else {
							if(gcell.getDataUp() != cell->getDataUp()){
								std::cout << "[Up node(" << rank << ")] Up is different at index " << gcell.getMortonIndex() << " level " << idxLevel << " is " << gcell.getDataUp() << " should be " << cell->getDataUp() << "\n";
							}
							if(gcell.getDataDown() != cell->getDataDown()){
								std::cout << "[Down node(" << rank << ")] Down is different at index " << gcell.getMortonIndex() << " level " << idxLevel << " is " << gcell.getDataDown() << " should be " << cell->getDataDown() << "\n";
							}
						}
				});
			}
		}
	}
	{
		int idxLevel = groupedTree.getHeight()-1;
		for(int idxGroup = 0 ; idxGroup < groupedTree.getNbCellGroupAtLevel(idxLevel) ; ++idxGroup){
			//if(groupalgo.isDataOwned(idxGroup, groupedTree.getNbCellGroupAtLevel(idxLevel))){
			if(groupalgo.isDataOwnedBerenger(groupedTree.getCellGroup(groupedTree.getHeight()-1, idxGroup)->getStartingIndex(), groupedTree.getHeight()-1)){
				GroupOctreeClass::ParticleGroupClass* particleGroup = groupedTree.getParticleGroup(idxGroup); 
				GroupOctreeClass::CellGroupClass* cellGroup = groupedTree.getCellGroup(idxLevel, idxGroup);
				cellGroup->forEachCell([&](GroupCellClass cell){
					MortonIndex midx = cell.getMortonIndex();
					const int leafIdx = particleGroup->getLeafIndex(midx);
					GroupContainerClass leaf = particleGroup->template getLeaf<GroupContainerClass>(leafIdx);
					const FSize nbPartsInLeaf = leaf.getNbParticles();
					if(cell.getDataUp() != nbPartsInLeaf){
						std::cout << "[P2M node(" << rank << ")] Error a Cell has " << cell.getDataUp() << " (it should be " << nbPartsInLeaf << ")\n";
					}
					const long long int* dataDown = leaf.getDataDown();
					for(FSize idxPart = 0 ; idxPart < nbPartsInLeaf ; ++idxPart){
						if(dataDown[idxPart] != loader.getNumberOfParticles()-1){
							std::cout << "[Full node(" << rank << ")] Error a particle has " << dataDown[idxPart] << " (it should be " << (loader.getNumberOfParticles()-1) << ") at index " << cell.getMortonIndex() << "\n";
						}
					}
				});
			}
		}
	}
    return 0;
}
std::vector<MortonIndex> getMortonIndex(const char* const mapping_filename)
{
	std::vector<MortonIndex> ret;

	std::ifstream fichier(mapping_filename, ios::in);  // on ouvre le fichier en lecture

	if(fichier)  // si l'ouverture a réussi
	{       
		int nbProcess;
		fichier >> nbProcess;
		for(int i = 0; i < nbProcess; ++i)
		{
			MortonIndex start, end;
			fichier >> start >> end;
			ret.push_back(start);
			ret.push_back(end);
		}
		// instructions
		fichier.close();  // on ferme le fichier
	}
	else  // sinon
		cerr << "Impossible d'ouvrir le fichier !" << endl;
	return ret;
}
void timeAverage(int mpi_rank, int nproc, double elapsedTime)
{
	if(mpi_rank == 0)
	{
		double sumElapsedTime = elapsedTime;
		for(int i = 1; i < nproc; ++i)
		{
			double tmp;
			MPI_Recv(&tmp, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, 0);
			sumElapsedTime += tmp;
		}
		sumElapsedTime = sumElapsedTime / (double)nproc;
		std::cout << "Average time per node : " << sumElapsedTime << "s" << std::endl;
	}
	else
	{
		MPI_Send(&elapsedTime, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
	}
}
void sortParticle(FPoint<FReal> * allParticles, int treeHeight, int groupSize, vector<vector<int>> & sizeForEachGroup, LoaderClass& loader, int nproc)
{
	//Structure pour trier
	struct ParticleSortingStruct{
		FPoint<FReal> position;
		MortonIndex mindex;
	};
	// Création d'un tableau de la structure pour trier puis remplissage du tableau
	const int nbParticles = loader.getNumberOfParticles();
	ParticleSortingStruct* particlesToSort = new ParticleSortingStruct[nbParticles];
	for(FSize idxPart = 0 ; idxPart < nbParticles ; ++idxPart){
		const FTreeCoordinate host = FCoordinateComputer::GetCoordinateFromPosition<FReal>(loader.getCenterOfBox(), loader.getBoxWidth(),
				treeHeight,
				allParticles[idxPart]);
		const MortonIndex particleIndex = host.getMortonIndex(treeHeight-1);
		particlesToSort[idxPart].mindex = particleIndex;
		particlesToSort[idxPart].position = allParticles[idxPart];
	}

	//Trie du nouveau tableau
	FQuickSort<ParticleSortingStruct, FSize>::QsOmp(particlesToSort, nbParticles, [](const ParticleSortingStruct& v1, const ParticleSortingStruct& v2){
			return v1.mindex <= v2.mindex;
			});
	//Replace tout dans l'ordre dans le tableau d'origine
	for(FSize idxPart = 0 ; idxPart < nbParticles ; ++idxPart){
		allParticles[idxPart] = particlesToSort[idxPart].position;
	}
	
	//Compte le nombre de feuilles
	sizeForEachGroup.resize(treeHeight+1);//Le +1 est pour les particules
	MortonIndex previousLeaf = -1;
	int numberOfLeaf = 0;
	for(FSize idxPart = 0 ; idxPart < nbParticles ; ++idxPart)
	{
		if(particlesToSort[idxPart].mindex != previousLeaf)
		{
			previousLeaf = particlesToSort[idxPart].mindex;
			++numberOfLeaf;
		}
	}

	//Calcul de la taille des groupes au niveau des feuilles
    FLeafBalance balancer;
	for(int processId = 0; processId < nproc; ++processId)
	{
		int size_last;
		int countGroup;
		int leafOnProcess = balancer.getRight(numberOfLeaf, nproc, processId) - balancer.getLeft(numberOfLeaf, nproc, processId);
		size_last = leafOnProcess%groupSize;
		countGroup = (leafOnProcess - size_last)/groupSize;
		for(int i = 0; i < countGroup; ++i)
			sizeForEachGroup[treeHeight-1].push_back(groupSize);
		sizeForEachGroup[treeHeight-1].push_back(size_last);
	}
	std::vector<MortonIndex> distributedMortonIndex;
	
	//Calcul du working interval au niveau des feuilles
	previousLeaf = -1;
	int countLeaf = 0;
	int processId = 0;
	int leafOnProcess = balancer.getRight(numberOfLeaf, nproc, 0) - balancer.getLeft(numberOfLeaf, nproc, 0);
	distributedMortonIndex.push_back(previousLeaf);
	for(FSize idxPart = 0 ; idxPart < nbParticles ; ++idxPart)
	{
		if(particlesToSort[idxPart].mindex != previousLeaf)
		{
			previousLeaf = particlesToSort[idxPart].mindex;
			++countLeaf;
			if(countLeaf == leafOnProcess)
			{
				distributedMortonIndex.push_back(previousLeaf);
				distributedMortonIndex.push_back(previousLeaf);
				countLeaf = 0;
				++processId;
				leafOnProcess = balancer.getRight(numberOfLeaf, nproc, processId) - balancer.getLeft(numberOfLeaf, nproc, processId);
			}
		}
	}
	distributedMortonIndex.push_back(particlesToSort[nbParticles - 1].mindex);

	//Ajout des groupes de particules
	int countParticle = 0;
	processId = 0;
	for(FSize idxPart = 0 ; idxPart < nbParticles ; ++idxPart)
	{
		if(particlesToSort[idxPart].mindex <= distributedMortonIndex[processId*2+1])
		{
			++countParticle;
			if(countParticle == groupSize)
			{
				sizeForEachGroup[treeHeight].push_back(countParticle);
				countParticle = 0;
			}
		}
		else
		{
			sizeForEachGroup[treeHeight].push_back(countParticle);
			countParticle = 0;
			++processId;
		}
	}

	//Calcul des working interval à chaque niveau
	std::vector<std::vector<std::vector<MortonIndex>>> nodeRepartition;
	createNodeRepartition(distributedMortonIndex, nodeRepartition, nproc, treeHeight);

	//Pour chaque niveau calcul de la taille des groupe
	for(int idxLevel = treeHeight - 2; idxLevel > 0; --idxLevel)
	{
		processId = 0;
		int countParticleInTheGroup = 0;
		MortonIndex previousMortonCell = -1;

		for(int idxPart = 0; idxPart < nbParticles; ++idxPart)
		{
			MortonIndex mortonCell = (particlesToSort[idxPart].mindex) >> (3*(treeHeight - idxLevel));
			if(nodeRepartition[idxLevel][processId][1] <= mortonCell) //Si l'indice est dans le working interval
			{
				if(mortonCell != previousMortonCell) //Si c'est un nouvelle indice
				{
					++countParticleInTheGroup; //On le compte dans le groupe
					previousMortonCell = mortonCell;
					if(countParticleInTheGroup == groupSize) //Si le groupe est plein on ajoute le compte
					{
						sizeForEachGroup[idxLevel].push_back(groupSize);
						countParticleInTheGroup = 0;
					}
				}
			}
			else //Si l'on change d'interval de process on ajoute ce que l'on a compté
			{
				sizeForEachGroup[idxLevel].push_back(countParticleInTheGroup);
				countParticleInTheGroup = 1;
				++processId;
			}
		}
	}
}
void createNodeRepartition(std::vector<MortonIndex> distributedMortonIndex, std::vector<std::vector<std::vector<MortonIndex>>>& nodeRepartition, int nproc, int treeHeight) {
	nodeRepartition.resize(treeHeight, std::vector<std::vector<MortonIndex>>(nproc, std::vector<MortonIndex>(2)));
	for(int node_id = 0; node_id < nproc; ++node_id){
		nodeRepartition[treeHeight-1][node_id][0] = distributedMortonIndex[node_id*2];
		nodeRepartition[treeHeight-1][node_id][1] = distributedMortonIndex[node_id*2+1];
	}
	for(int idxLevel = treeHeight - 2; idxLevel >= 0  ; --idxLevel){
		nodeRepartition[idxLevel][0][0] = nodeRepartition[idxLevel+1][0][0] >> 3;
		nodeRepartition[idxLevel][0][1] = nodeRepartition[idxLevel+1][0][1] >> 3;
		for(int node_id = 1; node_id < nproc; ++node_id){
			nodeRepartition[idxLevel][node_id][0] = FMath::Max(nodeRepartition[idxLevel+1][node_id][0] >> 3, nodeRepartition[idxLevel][node_id-1][0]+1); //Berenger phd :)
			nodeRepartition[idxLevel][node_id][1] = nodeRepartition[idxLevel+1][node_id][1] >> 3;
		}
	}
}
