// ===================================================================================
// Copyright ScalFmm 2011 INRIA, Olivier Coulaud, Bérenger Bramas, Matthias Messner
// olivier.coulaud@inria.fr, berenger.bramas@inria.fr
// This software is a computer program whose purpose is to compute the FMM.
//
// This software is governed by the CeCILL-C and LGPL licenses and
// abiding by the rules of distribution of free software.  
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public and CeCILL-C Licenses for more details.
// "http://www.cecill.info". 
// "http://www.gnu.org/licenses".
// ===================================================================================

#include <iostream>

#include <cstdio>
#include <cstdlib>

#include "../../Src/Utils/FTic.hpp"

#include "../../Src/Containers/FOctree.hpp"
#include "../../Src/Containers/FVector.hpp"
#include "../../Src/Utils/FParameters.hpp"

#include "../../Src/Components/FTypedLeaf.hpp"

#include "../../Src/Utils/FPoint.hpp"

#include "../../Src/Components/FTestParticle.hpp"
#include "../../Src/Components/FTestCell.hpp"
#include "../../Src/Components/FTestKernels.hpp"

#include "../../Src/Extensions/FExtendParticleType.hpp"
#include "../../Src/Extensions/FExtendCellType.hpp"

#include "../../Src/Core/FFmmAlgorithmTsm.hpp"
#include "../../Src/Core/FFmmAlgorithmThreadTsm.hpp"

#include "../../Src/Components/FBasicKernels.hpp"

#include "../../Src/Files/FRandomLoader.hpp"

/** This program show an example of use of
  * the fmm basic algo
  * it also check that each particles is impacted each other particles
  */

class FTestParticleTsm : public FTestParticle, public FExtendParticleType {
};

class FTestCellTsm: public FTestCell , public FExtendCellType{
};


// Simply create particles and try the kernels
int main(int argc, char ** argv){
    typedef FTestParticleTsm             ParticleClassTyped;
    typedef FTestCellTsm                 CellClassTyped;
    typedef FVector<ParticleClassTyped>  ContainerClassTyped;

    typedef FTypedLeaf<ParticleClassTyped, ContainerClassTyped >                      LeafClassTyped;
    typedef FOctree<ParticleClassTyped, CellClassTyped, ContainerClassTyped , LeafClassTyped >  OctreeClassTyped;
    typedef FTestKernels<ParticleClassTyped, CellClassTyped, ContainerClassTyped >          KernelClassTyped;

    typedef FFmmAlgorithmThreadTsm<OctreeClassTyped, ParticleClassTyped, CellClassTyped, ContainerClassTyped, KernelClassTyped, LeafClassTyped > FmmClassTyped;
    ///////////////////////What we do/////////////////////////////
    std::cout << ">> This executable has to be used to test the FMM algorithm.\n";
    //////////////////////////////////////////////////////////////

    const int NbLevels = FParameters::getValue(argc,argv,"-h", 5);
    const int SizeSubLevels = FParameters::getValue(argc,argv,"-sh", 3);
    const int NbPart = FParameters::getValue(argc,argv,"-nb", 2000000);
    FTic counter;

    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    FRandomLoaderTsm<ParticleClassTyped> loader(NbPart, 1, FPoint(0.5,0.5,0.5), 1);
    OctreeClassTyped tree(NbLevels, SizeSubLevels,loader.getBoxWidth(),loader.getCenterOfBox());

    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    std::cout << "Creating " << NbPart << " particles ..." << std::endl;
    counter.tic();

    loader.fillTree(tree);

    counter.tac();
    std::cout << "Done  " << "(@Creating and Inserting Particles = " << counter.elapsed() << "s)." << std::endl;


    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    std::cout << "Working on particles ..." << std::endl;
    counter.tic();

    KernelClassTyped kernels;

    FmmClassTyped algo(&tree,&kernels);
    algo.execute();

    counter.tac();
    std::cout << "Done  " << "(@Algorithm = " << counter.elapsed() << "s)." << std::endl;

    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    ValidateFMMAlgo<OctreeClassTyped, ParticleClassTyped, CellClassTyped, ContainerClassTyped, LeafClassTyped>(&tree);


    return 0;
}



