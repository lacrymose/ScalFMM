// ===================================================================================
// Copyright ScalFmm 2011 INRIA, Olivier Coulaud, Berenger Bramas, Matthias Messner
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

// ==== CMAKE =====
// @FUSE_MPI
// ================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "../../Src/Utils/FTic.hpp"
#include "../../Src/Utils/FParameters.hpp"
#include "../../Src/Utils/FMpi.hpp"

#include "../../Src/Containers/FOctree.hpp"
#include "../../Src/Containers/FVector.hpp"

#include "../../Src/Files/FTreeMpiCsvSaver.hpp"
#include "../../Src/Files/FFmaGenericLoader.hpp"

#include "../../Src/Components/FSimpleLeaf.hpp"
#include "../../Src/Components/FBasicCell.hpp"
#include "../../Src/Kernels/P2P/FP2PParticleContainer.hpp"

#include "../../Src/Utils/FParameterNames.hpp"


class VelocityContainer : public FP2PParticleContainer<> {
    typedef FP2PParticleContainer<> Parent;

    FVector<FPoint> velocities;

public:
    template<typename... Args>
    void push(const FPoint& inParticlePosition, const FPoint& velocity, Args... args){
        Parent::push(inParticlePosition, args... );
        velocities.push(velocity);
    }

    const FVector<FPoint>& getVelocities() const{
        return velocities;
    }

    FVector<FPoint>& getVelocities() {
        return velocities;
    }

    void fillToCsv(const int partIdx, FReal values[4]) const {
        values[0] = Parent::getPositions()[0][partIdx];
        values[1] = Parent::getPositions()[1][partIdx];
        values[2] = Parent::getPositions()[2][partIdx];
        values[3] = Parent::getPotentials()[partIdx];
    }
};


class GalaxyLoader : public FFmaGenericLoader {
public:
    GalaxyLoader(const std::string & filename) : FFmaGenericLoader(filename) {
    }

    void fillParticle(FPoint* position, FReal* physivalValue, FPoint* velocity){
        FReal x,y,z,data, vx, vy, vz;
        (*this->file)  >> x >> y >> z >> data >> vx >> vy >> vz;
        position->setPosition(x,y,z);
        *physivalValue = (data);
        velocity->setPosition(vx,vy,vz);
    }
};

struct TestParticle{
    FPoint position;
    FReal physicalValue;
    FReal potential;
    FReal forces[3];
    FPoint velocity;
    const FPoint& getPosition(){
        return position;
    }
};

template <class ParticleClass>
class Converter {
public:
    template <class ContainerClass>
    static ParticleClass GetParticle(ContainerClass* containers, const int idxExtract){
        const FReal*const positionsX = containers->getPositions()[0];
        const FReal*const positionsY = containers->getPositions()[1];
        const FReal*const positionsZ = containers->getPositions()[2];
        const FReal*const forcesX = containers->getForcesX();
        const FReal*const forcesY = containers->getForcesY();
        const FReal*const forcesZ = containers->getForcesZ();
        const FReal*const physicalValues = containers->getPhysicalValues();
        const FReal*const potentials = containers->getPotentials();
        FVector<FPoint> velocites = containers->getVelocities();

        TestParticle part;
        part.position.setPosition( positionsX[idxExtract],positionsY[idxExtract],positionsZ[idxExtract]);
        part.physicalValue = physicalValues[idxExtract];
        part.forces[0] = forcesX[idxExtract];
        part.forces[1] = forcesY[idxExtract];
        part.forces[2] = forcesZ[idxExtract];
        part.potential = potentials[idxExtract];
        part.velocity  = velocites[idxExtract];

        return part;
    }

    template <class OctreeClass>
    static void Insert(OctreeClass* tree, const ParticleClass& part){
        tree->insert(part.position , part.velocity, part.physicalValue, part.forces[0],
                part.forces[1],part.forces[2],part.potential);
    }
};

// Simply create particles and try the kernels
int main(int argc, char ** argv){
    FHelpDescribeAndExit(argc, argv,
                         "Convert the data from a file into a csv file to load into Paraview for example.\n"
                         "It puts the file into the /tmp dir and the code is an example of using FTreeMpiCsvSaver.",
                         FParameterDefinitions::OctreeHeight, FParameterDefinitions::OctreeSubHeight,
                         FParameterDefinitions::InputFile);

    typedef FBasicCell              CellClass;
    typedef VelocityContainer  ContainerClass;

    typedef FSimpleLeaf< ContainerClass >                     LeafClass;
    typedef FOctree< CellClass, ContainerClass , LeafClass >  OctreeClass;
    ///////////////////////What we do/////////////////////////////
    std::cout << ">> This executable has to be used to test Spherical algorithm.\n";
    //////////////////////////////////////////////////////////////
    FMpi app( argc, argv);

    const int NbLevels = FParameters::getValue(argc,argv,FParameterDefinitions::OctreeHeight.options, 6);
    const int SizeSubLevels = FParameters::getValue(argc,argv,FParameterDefinitions::OctreeSubHeight.options, 3);

    GalaxyLoader loader(FParameters::getStr(argc,argv,FParameterDefinitions::InputFile.options, "../Data/galaxy.fma"));

    // -----------------------------------------------------

    OctreeClass tree(NbLevels, SizeSubLevels, loader.getBoxWidth(), loader.getCenterOfBox());

    // -----------------------------------------------------

    std::cout << "Creating & Inserting " << loader.getNumberOfParticles() << " particles ..." << std::endl;
    std::cout << "\tHeight : " << NbLevels << " \t sub-height : " << SizeSubLevels << std::endl;

    {
        FPoint position, velocity;
        FReal physicalValue;

        for(int idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
            loader.fillParticle(&position, &physicalValue, &velocity);
            if( (idxPart+1) % (app.global().processId()+1) == 0) tree.insert(position,velocity,physicalValue);
        }
    }

    // -----------------------------------------------------

    {
        FTreeMpiCsvSaver<OctreeClass, ContainerClass> saver("/tmp/test%d.csv", app.global() , false);
        saver.exportTree(&tree);
    }

    // -----------------------------------------------------

    {
        FTreeMpiCsvSaver<OctreeClass, ContainerClass> saver("/tmp/htest%d.csv", app.global() , true);
        saver.exportTree(&tree);
    }

    return 0;
}
