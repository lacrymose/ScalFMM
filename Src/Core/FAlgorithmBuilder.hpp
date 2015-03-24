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
#ifndef FALGORITHMBUILDER_HPP
#define FALGORITHMBUILDER_HPP

#include "FCoreCommon.hpp"

#include "../Utils/FGlobal.hpp"
#include "../Utils/FGlobalPeriodic.hpp"

#include "FFmmAlgorithm.hpp"
#include "FFmmAlgorithmThread.hpp"
#include "FFmmAlgorithmPeriodic.hpp"

#ifdef ScalFMM_USE_MPI
#include "../Utils/FMpi.hpp"
#include "FFmmAlgorithmThreadProc.hpp"
#include "FFmmAlgorithmThreadProcPeriodic.hpp"
#else
typedef int MPI_Comm;
#endif




/**
 * @brief The FAlgorithmBuilder class
 * This class manage the creation of an algorithm.
 */
template <class FReal>
class FAlgorithmBuilder {
public:
    /** To give the informtion to the kernel */
    struct SimulationProperties{
        const int height;
        const FPoint<FReal> centerOfBox;
        const FReal dimOfBox;
    };

    /** Get the simulation information from the properties (periodic or not etc.) */
    static SimulationProperties BuildKernelSimulationProperties(const int realHeight, const FPoint<FReal> realCenterOfBox, const FReal realDimOfBox,
                                                   const bool isPeriodic = false, const int inUpperLevel = 0){
        if( isPeriodic == false ){
            // Simply return the original data
            SimulationProperties properties = {realHeight, realCenterOfBox, realDimOfBox};
            return properties;
        }
        else{
            // Get the new data
            const int offsetRealTree(inUpperLevel + 3);
            const FReal extendedBoxWidth = realDimOfBox * FReal(1<<offsetRealTree);

            const FReal originalBoxWidth = realDimOfBox;
            const FReal originalBoxWidthDiv2 = originalBoxWidth/2.0;
            const FPoint<FReal> originalBoxCenter = realCenterOfBox;

            const FReal offset = extendedBoxWidth/2;
            const FPoint<FReal> extendedBoxCenter( originalBoxCenter.getX() - originalBoxWidthDiv2 + offset,
                           originalBoxCenter.getY() - originalBoxWidthDiv2 + offset,
                           originalBoxCenter.getZ() - originalBoxWidthDiv2 + offset);

            const int extendedTreeHeight = realHeight + offsetRealTree;

            SimulationProperties properties = {extendedTreeHeight, extendedBoxCenter, extendedBoxWidth};
            return properties;
        }
    }

    /**
     * Build an algorithm using mpi/periodic or not.
     */
    template<class OctreeClass, class CellClass, class ContainerClass, class KernelClass, class LeafClass>
    static FAbstractAlgorithm* BuildAlgorithm(OctreeClass*const tree, KernelClass*const kernel,
                                       const MPI_Comm mpiComm = (MPI_Comm)0, const bool isPeriodic = false,
                                       const int periodicUpperlevel = 0){
    #ifdef ScalFMM_USE_MPI
        if(isPeriodic == false){
            return new FFmmAlgorithmThreadProc<OctreeClass, CellClass, ContainerClass, KernelClass, LeafClass>(FMpi::FComm(mpiComm), tree, kernel);
        }
        else{
            auto algo = new FFmmAlgorithmThreadProcPeriodic<OctreeClass, CellClass, ContainerClass, KernelClass, LeafClass>(FMpi::FComm(mpiComm), tree, periodicUpperlevel);
            algo->setKernel(kernel);
            return algo;
        }
    #else
        if(isPeriodic == false){
            return new FFmmAlgorithmThread<OctreeClass, CellClass, ContainerClass, KernelClass, LeafClass>(tree, kernel);
        }
        else{
            auto algo = new FFmmAlgorithmPeriodic<OctreeClass, CellClass, ContainerClass, KernelClass, LeafClass>(tree, periodicUpperlevel);
            algo->setKernel(kernel);
            return algo;
        }
    #endif
    }
};



#endif // FALGORITHMBUILDER_HPP
