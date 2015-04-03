// ===================================================================================
// Copyright ScalFmm 2014 I
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


/**
 * @file This file contain a class, gathering all the function that
 * can be called by the ScalFMM API. Documentation for each function
 * can be found in the C Header.
 *
 */

#ifndef FSCALFMMENGINE_HPP
#define FSCALFMMENGINE_HPP


#include "Utils/FAssert.hpp"

//For tree
#include "Components/FSimpleLeaf.hpp"
#include "Kernels/P2P/FP2PParticleContainerIndexed.hpp"
#include "Containers/FOctree.hpp"

//For interpolation
#include "Kernels/Interpolation/FInterpMatrixKernel.hpp"

//For lagrange interpolation
// #include "Kernels/Uniform/FUnifCell.hpp"
// #include "Kernels/Uniform/FUnifKernel.hpp"

//For chebyshev Interpolation
#include "Kernels/Chebyshev/FChebCell.hpp"
#include "Kernels/Chebyshev/FChebSymKernel.hpp"




/**
 * @class FScalFMMEngine
 */
class FScalFMMEngine{
protected:
    scalfmm_kernel_type kernelType;

    scalfmm_algorithm Algorithm;
    FVector<bool>* progress;
    int nbPart;

public:
    FScalFMMEngine() : Algorithm(multi_thread), progress(nullptr), nbPart(0){
        progress = new FVector<bool>();
    }

    virtual ~FScalFMMEngine() {
        delete progress;
    }

    //First function displayed there are common function for every
    //kernel
    scalfmm_kernel_type getKernelType(){
        return this->kernelType;
    }


    //To change default algorithm
    void algorithm_config(scalfmm_algorithm config){
        this->Algorithm = config;
    }


    //Functions displayed there are function that are to be redefined
    //by specific Engine

    //Function about the tree
    virtual void build_tree(int TreeHeight,double BoxWidth,double* BoxCenter,Scalfmm_Cell_Descriptor user_cell_descriptor){
        FAssertLF(0,"Nothing has been done yet, exiting");
    }

    virtual void tree_insert_particles( int NbPositions, double * arrayX, double * arrayY, double * arrayZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }

    virtual void tree_insert_particles_xyz( int NbPositions, double * XYZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }

    virtual void set_physical_values( int nbPhysicalValues, double * physicalValues){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }

    virtual void get_physical_values( int nbPhysicalValues, double * physicalValues){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }

    virtual void set_physical_values_npart( int nbPhysicalValues,
                                            int* idxOfParticles, double * physicalValues){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void get_physical_values_npart( int nbPhysicalValues,
                                            int* idxOfParticles, double * physicalValues){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }

    //To get the result
    virtual void get_forces_xyz( int nbParts, double * forcesToFill){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void get_forces_xyz_npart( int nbParts, int* idxOfParticles, double * forcesToFill){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void get_forces( int nbParts, double * fX, double* fY, double* fZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void get_forces_npart( int nbParts, int* idxOfParticles, double * fX, double* fY, double* fZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }

    //To set initial condition
    virtual void set_forces_xyz( int nbParts, double * forcesToFill){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void set_forces_xyz_npart( int nbParts, int* idxOfParticles, double * forcesToFill){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void set_forces( int nbParts, double * fX, double* fY, double* fZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void set_forces_npart( int nbParts, int* idxOfParticles, double * fX, double* fY, double* fZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    //To deal with potential
    virtual void get_potentials( int nbParts, double * potentialsToFill){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void set_potentials( int nbParts, double * potentialsToRead){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void get_potentials_npart( int nbParts, int* idxOfParticles, double * potentialsToFill){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void set_potentials_npart( int nbParts, int* idxOfParticles, double * potentialsToRead){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }

    //Function to move particles
    virtual void add_to_positions_xyz( int NbPositions, double * updatedXYZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void add_to_positions( int NbPositions, double * X, double * Y , double * Z){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void add_to_positions_xyz_npart( int NbPositions, int* idxOfParticles, double * updatedXYZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void add_to_positions_npart( int NbPositions, int* idxOfParticles,
                                         double * X, double * Y , double * Z){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void set_positions_xyz( int NbPositions, double * updatedXYZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void set_positions( int NbPositions, double * X, double * Y , double * Z){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void set_positions_xyz_npart( int NbPositions, int* idxOfParticles, double * updatedXYZ){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void set_positions_npart( int NbPositions, int* idxOfParticles,
                                      double * X, double * Y , double * Z){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }

    //Function to update the tree
    virtual void update_tree(){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }


    //Function to get the positions
    virtual void get_positions_xyz( int NbPositions, double * positionsToFill){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void get_positions( int NbPositions, double * X, double * Y , double * Z){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void get_positions_xyz_npart( int NbPositions, int* idxOfParticles, double * positionsToFill){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }
    virtual void get_positions_npart( int NbPositions, int* idxOfParticles,
                                      double * X, double * Y , double * Z){
        FAssertLF(0,"No tree instancied, exiting ...\n");
    }

    virtual void reset_tree(){
    }

    template<class FReal,class ContainerClass, class CellClass, class LeafClass>
    void generic_reset_tree(FOctree<FReal,CellClass,ContainerClass,LeafClass> * tree){
        //Reset forces and potentials
        tree->forEachLeaf([&](LeafClass * leaf){
                ContainerClass * targets = leaf->getTargets();
                FSize nbPartTarget = targets->getNbParticles();
                //Set potential to 0
                FReal * potentialsTarget = targets->getPotentialsArray();
                memset(potentialsTarget,0,sizeof(FReal)*nbPartTarget);
                //Set forces to 0
                FReal * forcesX = targets->getForcesXArray();
                FReal * forcesY = targets->getForcesYArray();
                FReal * forcesZ = targets->getForcesZArray();
                memset(forcesX,0,sizeof(FReal)*nbPartTarget);
                memset(forcesY,0,sizeof(FReal)*nbPartTarget);
                memset(forcesZ,0,sizeof(FReal)*nbPartTarget);
            });

        //Reset multipole and local development
        tree->forEachCell([&](CellClass * cell){
                cell->resetToInitialState();
            });
    }

    //User define Kernel Part
    virtual void user_kernel_config( Scalfmm_Kernel_Descriptor userKernel, void * userDatas){
        FAssertLF(0,"No user kernel defined, exiting ...\n");
    }

     virtual void execute_fmm(){
        FAssertLF(0,"No kernel set, cannot execute anything, exiting ...\n");
    }

    virtual void intern_dealloc_handle(Callback_free_cell userDeallocator){
        FAssertLF(0,"No kernel set, cannot execute anything, exiting ...\n");
    }



};


struct ScalFmmCoreHandle {
    struct ScalFmmCoreConfig {
        // Read/Write parameter
        int treeHeight;     //  Number of level in the octree
        double boxWidth;    // Simulation box size (root level)
        FPoint<double> boxCenter; // Center position of the box simulation(FReal[3])
    };

    ScalFmmCoreConfig config;
    FScalFMMEngine* engine;
};



extern "C" void scalfmm_build_tree(scalfmm_handle Handle,int TreeHeight,double BoxWidth,double* BoxCenter,Scalfmm_Cell_Descriptor user_cell_descriptor){
    ((ScalFmmCoreHandle *) Handle)->engine->build_tree(TreeHeight,BoxWidth, BoxCenter, user_cell_descriptor);
}

extern "C" void scalfmm_tree_insert_particles(scalfmm_handle Handle, int NbPositions, double * arrayX, double * arrayY, double * arrayZ){
    ((ScalFmmCoreHandle *) Handle)->engine->tree_insert_particles(NbPositions, arrayX, arrayY, arrayZ);
}

extern "C" void scalfmm_tree_insert_particles_xyz(scalfmm_handle Handle, int NbPositions, double * XYZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->tree_insert_particles_xyz(NbPositions, XYZ);
}

extern "C" void scalfmm_set_physical_values(scalfmm_handle Handle, int nbPhysicalValues, double * physicalValues){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_physical_values(nbPhysicalValues, physicalValues);
}

extern "C" void scalfmm_get_physical_values(scalfmm_handle Handle, int nbPhysicalValues, double * physicalValues){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_physical_values(nbPhysicalValues, physicalValues);
}

extern "C" void scalfmm_set_physical_values_npart(scalfmm_handle Handle, int nbPhysicalValues,
                                                  int* idxOfParticles, double * physicalValues){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_physical_values_npart(nbPhysicalValues,
                                                                       idxOfParticles, physicalValues);
}
extern "C" void scalfmm_get_physical_values_npart(scalfmm_handle Handle, int nbPhysicalValues,
                                                  int* idxOfParticles, double * physicalValues){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_physical_values_npart(nbPhysicalValues,
                                                                       idxOfParticles, physicalValues);
}

//To get the result
extern "C" void scalfmm_get_forces_xyz(scalfmm_handle Handle, int nbParts, double * forcesToFill){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_forces_xyz(nbParts, forcesToFill);
}

extern "C" void scalfmm_get_forces_xyz_npart(scalfmm_handle Handle, int nbParts, int* idxOfParticles, double * forcesToFill){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_forces_xyz_npart(nbParts, idxOfParticles, forcesToFill);
}
extern "C" void scalfmm_get_forces(scalfmm_handle Handle, int nbParts, double * fX, double* fY, double* fZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_forces(nbParts,fX, fY, fZ) ;
}

extern "C" void scalfmm_get_forces_npart(scalfmm_handle Handle, int nbParts, int* idxOfParticles, double * fX, double* fY, double* fZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_forces_npart(nbParts, idxOfParticles, fX, fY, fZ) ;
}

//To set iniital condition
extern "C" void scalfmm_set_forces_xyz(scalfmm_handle Handle, int nbParts, double * forcesToFill){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_forces_xyz(nbParts, forcesToFill) ;
}

extern "C" void scalfmm_set_forces_xyz_npart(scalfmm_handle Handle, int nbParts, int* idxOfParticles, double * forcesToFill){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_forces_xyz_npart(nbParts, idxOfParticles, forcesToFill) ;
}

extern "C" void scalfmm_set_forces(scalfmm_handle Handle, int nbParts, double * fX, double* fY, double* fZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_forces(nbParts, fX, fY, fZ) ;
}

extern "C" void scalfmm_set_forces_npart(scalfmm_handle Handle, int nbParts, int* idxOfParticles, double * fX, double* fY, double* fZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_forces_npart(nbParts, idxOfParticles, fX, fY, fZ) ;
}

//To deal with potential
extern "C" void scalfmm_get_potentials(scalfmm_handle Handle, int nbParts, double * potentialsToFill){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_potentials(nbParts, potentialsToFill) ;
}

extern "C" void scalfmm_set_potentials(scalfmm_handle Handle, int nbParts, double * potentialsToFill){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_potentials(nbParts, potentialsToFill) ;
}

extern "C" void scalfmm_get_potentials_npart(scalfmm_handle Handle, int nbParts, int* idxOfParticles, double * potentialsToFill){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_potentials_npart(nbParts, idxOfParticles, potentialsToFill) ;
}

extern "C" void scalfmm_set_potentials_npart(scalfmm_handle Handle, int nbParts, int* idxOfParticles, double * potentialsToFill){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_potentials_npart(nbParts, idxOfParticles, potentialsToFill) ;
}


// //To deal with positions
// //Out of the box behavior
// extern "C" void scalfmm_out_of_the_box_config(scalfmm_handle Handle,scalfmm_out_of_box_behavior config){
//     ((ScalFmmCoreHandle * ) Handle)->engine->out_of_the_box_config(config);
// }

//Update
extern "C" void scalfmm_add_to_positions_xyz(scalfmm_handle Handle, int NbPositions, double * updatedXYZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->add_to_positions_xyz(NbPositions, updatedXYZ);
}

extern "C" void scalfmm_add_to_positions(scalfmm_handle Handle, int NbPositions, double * X, double * Y , double * Z){
    ((ScalFmmCoreHandle * ) Handle)->engine->add_to_positions(NbPositions, X, Y, Z);
}

extern "C" void scalfmm_add_to_positions_xyz_npart(scalfmm_handle Handle, int NbPositions, int* idxOfParticles, double * updatedXYZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->add_to_positions_xyz_npart(NbPositions, idxOfParticles, updatedXYZ);
}

extern "C" void scalfmm_add_to_positions_npart(scalfmm_handle Handle, int NbPositions, int* idxOfParticles,
                                               double * X, double * Y , double * Z){
    ((ScalFmmCoreHandle * ) Handle)->engine->add_to_positions_npart(NbPositions, idxOfParticles, X, Y, Z);
}
//Set new positions
extern "C" void scalfmm_set_positions_xyz(scalfmm_handle Handle, int NbPositions, double * updatedXYZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_positions_xyz(NbPositions, updatedXYZ);
}

extern "C" void scalfmm_set_positions(scalfmm_handle Handle, int NbPositions, double * X, double * Y , double * Z){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_positions(NbPositions, X, Y , Z);
}

extern "C" void scalfmm_set_positions_xyz_npart(scalfmm_handle Handle, int NbPositions, int* idxOfParticles, double * updatedXYZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_positions_xyz_npart(NbPositions, idxOfParticles, updatedXYZ);
}

extern "C" void scalfmm_set_positions_npart(scalfmm_handle Handle, int NbPositions, int* idxOfParticles,
                                            double * X, double * Y , double * Z){
    ((ScalFmmCoreHandle * ) Handle)->engine->set_positions_npart(NbPositions, idxOfParticles, X, Y, Z);
}

extern "C" void scalfmm_update_tree(scalfmm_handle Handle){
    ((ScalFmmCoreHandle * ) Handle)->engine->update_tree();
}

//Get back positions
extern "C" void scalfmm_get_positions_xyz(scalfmm_handle Handle, int NbPositions, double * updatedXYZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_positions_xyz(NbPositions, updatedXYZ);
}

extern "C" void scalfmm_get_positions(scalfmm_handle Handle, int NbPositions, double * X, double * Y , double * Z){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_positions(NbPositions, X, Y , Z);
}

extern "C" void scalfmm_get_positions_xyz_npart(scalfmm_handle Handle, int NbPositions, int* idxOfParticles, double * updatedXYZ){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_positions_xyz_npart(NbPositions, idxOfParticles, updatedXYZ);
}

extern "C" void scalfmm_get_positions_npart(scalfmm_handle Handle, int NbPositions, int* idxOfParticles,
                                            double * X, double * Y , double * Z){
    ((ScalFmmCoreHandle * ) Handle)->engine->get_positions_npart(NbPositions, idxOfParticles, X, Y, Z);
}

//To choose algorithm
extern "C" void scalfmm_algorithm_config(scalfmm_handle Handle, scalfmm_algorithm config){
    ((ScalFmmCoreHandle * ) Handle)->engine->algorithm_config(config);
}

//Executing FMM
extern "C" void scalfmm_execute_fmm(scalfmm_handle Handle){
    ((ScalFmmCoreHandle * ) Handle)->engine->execute_fmm();
}

extern "C" void scalfmm_user_kernel_config(scalfmm_handle Handle, Scalfmm_Kernel_Descriptor userKernel, void * userDatas){
    ((ScalFmmCoreHandle * ) Handle)->engine->user_kernel_config(userKernel,userDatas);
}

/**
 * These functions are just translating functions.
 */

//< This function fill the childFullPosition[3] with [-1;1] to know the position of a child relatively to
//< its position from its parent
extern "C" void scalfmm_utils_parentChildPosition(int childPosition, int* childFullPosition){
    childFullPosition[2] = (childPosition%2 ? 1 : -1);
    childFullPosition[1] = ((childPosition/2)%2 ? 1 : -1);
    childFullPosition[0] = ((childPosition/4)%2 ? 1 : -1);
}

//< This function fill the childFullPosition[3] with [-3;3] to know the position of a interaction
//< cell relatively to its position from the target
extern "C" void scalfmm_utils_interactionPosition(int interactionPosition, int* srcPosition){
    srcPosition[2] = interactionPosition%7 - 3;
    srcPosition[1] = (interactionPosition/7)%7 - 3;
    srcPosition[0] = (interactionPosition/49)%7 - 3;
}


extern "C" void scalfmm_reset_tree(scalfmm_handle Handle){
    ((ScalFmmCoreHandle * ) Handle)->engine->reset_tree();
}

#endif
