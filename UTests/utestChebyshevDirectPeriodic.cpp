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

// ==== CMAKE =====
// @FUSE_BLAS
// ================

#include "../Src/Utils/FGlobal.hpp"

#include "../Src/Containers/FOctree.hpp"
#include "../Src/Containers/FVector.hpp"

#include "../Src/Files/FRandomLoader.hpp"
#include "../Src/Files/FTreeIO.hpp"

#include "../Src/Core/FFmmAlgorithmPeriodic.hpp"

#include "FUTester.hpp"

#include "../Src/Kernels/Chebyshev/FChebLeaf.hpp"
#include "../Src/Kernels/Chebyshev/FChebCell.hpp"
#include "../Src/Kernels/Chebyshev/FChebMatrixKernel.hpp"
#include "../Src/Kernels/Chebyshev/FChebKernel.hpp"
#include "../Src/Kernels/Chebyshev/FChebSymKernel.hpp"

#include "../Src/Components/FSimpleLeaf.hpp"
#include "../Src/Kernels/P2P/FP2PParticleContainerIndexed.hpp"

/*
  In this test we compare the spherical fmm results and the direct results.
*/


/** the test class
 *
 */
class TestChebyshevDirect : public FUTester<TestChebyshevDirect> {
	
	///////////////////////////////////////////////////////////
	// The tests!
	///////////////////////////////////////////////////////////
	
    template <class CellClass, class ContainerClass, class KernelClass, class MatrixKernelClass,
						class LeafClass, class OctreeClass, class FmmClass>
	void RunTest(const FReal epsilon)	{
		// Warning in make test the exec dir it Build/UTests
		// Load particles

        const int NbLevels      = 3;
        const int SizeSubLevels = 2;
        const int PeriodicDeep  = 2;
        const int NbParticles   = 1;

        FRandomLoader loader(NbParticles);

        Print("Number of particles:");
        Print(loader.getNumberOfParticles());

        // Create octree
        OctreeClass tree(NbLevels, SizeSubLevels, loader.getBoxWidth(), loader.getCenterOfBox());

        struct TestParticle{
            FPoint position;
            FReal forces[3];
            FReal physicalValue;
            FReal potential;
        };
        TestParticle* const particles = new TestParticle[loader.getNumberOfParticles()];
        for(int idxPart = 0 ; idxPart < loader.getNumberOfParticles() ; ++idxPart){
            FPoint position;
            loader.fillParticle(&position);
            // put in tree
            tree.insert(position, idxPart, 0.10);
            // get copy
            particles[idxPart].position = position;
            particles[idxPart].physicalValue = 0.10;
            particles[idxPart].potential = 0.0;
            particles[idxPart].forces[0] = 0.0;
            particles[idxPart].forces[1] = 0.0;
            particles[idxPart].forces[2] = 0.0;
        }

        // Run FMM
        Print("Fmm...");
        FmmClass algo(&tree,PeriodicDeep );
        KernelClass kernels(algo.extendedTreeHeight(), algo.extendedBoxCenter(),
                            algo.extendedBoxWidth(), epsilon);
        algo.setKernel(&kernels);
        algo.execute();

        // Run direct computation
        const MatrixKernelClass MatrixKernel;
        Print("Direct...");

        FTreeCoordinate min, max;
        algo.repetitionsIntervals(&min, &max);

        for(int idxTarget = 0 ; idxTarget < loader.getNumberOfParticles() ; ++idxTarget){
            for(int idxOther = idxTarget + 1 ; idxOther < loader.getNumberOfParticles() ; ++idxOther){
                FP2P::MutualParticles(particles[idxTarget].position.getX(), particles[idxTarget].position.getY(),
                                      particles[idxTarget].position.getZ(),particles[idxTarget].physicalValue,
                                      &particles[idxTarget].forces[0],&particles[idxTarget].forces[1],
                                      &particles[idxTarget].forces[2],&particles[idxTarget].potential,
                                particles[idxOther].position.getX(), particles[idxOther].position.getY(),
                                particles[idxOther].position.getZ(),particles[idxOther].physicalValue,
                                &particles[idxOther].forces[0],&particles[idxOther].forces[1],
                                &particles[idxOther].forces[2],&particles[idxOther].potential);
            }
            for(int idxX = min.getX() ; idxX <= max.getX() ; ++idxX){
                for(int idxY = min.getY() ; idxY <= max.getY() ; ++idxY){
                    for(int idxZ = min.getZ() ; idxZ <= max.getZ() ; ++idxZ){
                        if(idxX ==0 && idxY == 0 && idxZ == 0) continue;
                        // next lines for test

                        const FPoint offset(loader.getBoxWidth() * FReal(idxX),
                                            loader.getBoxWidth() * FReal(idxY),
                                            loader.getBoxWidth() * FReal(idxZ));

                        for(int idxSource = 0 ; idxSource < NbParticles ; ++idxSource){
                            TestParticle source = particles[idxSource];
                            source.position += offset;

                            FP2P::NonMutualParticles(
                                        source.position.getX(), source.position.getY(),
                                        source.position.getZ(),source.physicalValue,
                                        particles[idxTarget].position.getX(), particles[idxTarget].position.getY(),
                                          particles[idxTarget].position.getZ(),particles[idxTarget].physicalValue,
                                          &particles[idxTarget].forces[0],&particles[idxTarget].forces[1],
                                          &particles[idxTarget].forces[2],&particles[idxTarget].potential);
                        }
                    }
                }
            }
        }

        // Compare
        Print("Compute Diff...");
        FMath::FAccurater potentialDiff;
        FMath::FAccurater fx, fy, fz;
        { // Check that each particle has been summed with all other

            tree.forEachLeaf([&](LeafClass* leaf){
                const FReal*const potentials = leaf->getTargets()->getPotentials();
                const FReal*const forcesX = leaf->getTargets()->getForcesX();
                const FReal*const forcesY = leaf->getTargets()->getForcesY();
                const FReal*const forcesZ = leaf->getTargets()->getForcesZ();
                const int nbParticlesInLeaf = leaf->getTargets()->getNbParticles();
                const FVector<int>& indexes = leaf->getTargets()->getIndexes();

                for(int idxPart = 0 ; idxPart < nbParticlesInLeaf ; ++idxPart){
                    const int indexPartOrig = indexes[idxPart];
                    potentialDiff.add(particles[indexPartOrig].potential,potentials[idxPart]);
                    fx.add(particles[indexPartOrig].forces[0],forcesX[idxPart]);
                    fy.add(particles[indexPartOrig].forces[1],forcesY[idxPart]);
                    fz.add(particles[indexPartOrig].forces[2],forcesZ[idxPart]);
                }
            });
        }

        delete[] particles;

        // Print for information
        Print("Potential diff is = ");
        Print(potentialDiff.getL2Norm());
        Print(potentialDiff.getInfNorm());
        Print("Fx diff is = ");
        Print(fx.getL2Norm());
        Print(fx.getInfNorm());
        Print("Fy diff is = ");
        Print(fy.getL2Norm());
        Print(fy.getInfNorm());
        Print("Fz diff is = ");
        Print(fz.getL2Norm());
        Print(fz.getInfNorm());

        // Assert
        const FReal MaximumDiffPotential = FReal(9e-5);
        const FReal MaximumDiffForces = FReal(9e-3);

        uassert(potentialDiff.getL2Norm() < MaximumDiffPotential);
        uassert(potentialDiff.getInfNorm() < MaximumDiffPotential);
        uassert(fx.getL2Norm()  < MaximumDiffForces);
        uassert(fx.getInfNorm() < MaximumDiffForces);
        uassert(fy.getL2Norm()  < MaximumDiffForces);
        uassert(fy.getInfNorm() < MaximumDiffForces);
        uassert(fz.getL2Norm()  < MaximumDiffForces);
        uassert(fz.getInfNorm() < MaximumDiffForces);

    }

	/** If memstas is running print the memory used */
	void PostTest() {
		if( FMemStats::controler.isUsed() ){
			std::cout << "Memory used at the end " << FMemStats::controler.getCurrentAllocated()
								<< " Bytes (" << FMemStats::controler.getCurrentAllocatedMB() << "MB)\n";
			std::cout << "Max memory used " << FMemStats::controler.getMaxAllocated()
								<< " Bytes (" << FMemStats::controler.getMaxAllocatedMB() << "MB)\n";
			std::cout << "Total memory used " << FMemStats::controler.getTotalAllocated()
								<< " Bytes (" << FMemStats::controler.getTotalAllocatedMB() << "MB)\n";
		}
	}


	///////////////////////////////////////////////////////////
	// Set the tests!
	///////////////////////////////////////////////////////////


	/** TestChebKernel */
	void TestChebKernel(){
		const unsigned int ORDER = 5;
        const FReal epsilon = FReal(1e-5);
        typedef FP2PParticleContainerIndexed ContainerClass;
        typedef FSimpleLeaf<ContainerClass> LeafClass;
		typedef FChebMatrixKernelR MatrixKernelClass;
		typedef FChebCell<ORDER> CellClass;
        typedef FOctree<CellClass,ContainerClass,LeafClass> OctreeClass;
        typedef FChebKernel<CellClass,ContainerClass,MatrixKernelClass,ORDER> KernelClass;
        typedef FFmmAlgorithmPeriodic<OctreeClass,CellClass,ContainerClass,KernelClass,LeafClass> FmmClass;
		// run test
        RunTest<CellClass,ContainerClass,KernelClass,MatrixKernelClass,LeafClass,OctreeClass,FmmClass>(epsilon);
	}

	/** TestChebSymKernel */
	void TestChebSymKernel(){
		const unsigned int ORDER = 5;
        const FReal epsilon = FReal(1e-5);
        typedef FP2PParticleContainerIndexed ContainerClass;
        typedef FSimpleLeaf<ContainerClass> LeafClass;
		typedef FChebMatrixKernelR MatrixKernelClass;
		typedef FChebCell<ORDER> CellClass;
        typedef FOctree<CellClass,ContainerClass,LeafClass> OctreeClass;
        typedef FChebSymKernel<CellClass,ContainerClass,MatrixKernelClass,ORDER> KernelClass;
        typedef FFmmAlgorithmPeriodic<OctreeClass,CellClass,ContainerClass,KernelClass,LeafClass> FmmClass;
		// run test
        RunTest<CellClass,ContainerClass,KernelClass,MatrixKernelClass,LeafClass,OctreeClass,FmmClass>(epsilon);
	}



	///////////////////////////////////////////////////////////
	// Set the tests!
	///////////////////////////////////////////////////////////

	/** set test */
	void SetTests(){
		AddTest(&TestChebyshevDirect::TestChebKernel,"Test Chebyshev Kernel with one big SVD");
        AddTest(&TestChebyshevDirect::TestChebSymKernel,"Test Chebyshev Kernel with 16 small SVDs and symmetries");
	}
};


// You must do this
TestClass(TestChebyshevDirect)




