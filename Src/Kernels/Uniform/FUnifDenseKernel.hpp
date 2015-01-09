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
// Keep in private GIT
// @SCALFMM_PRIVATE

#ifndef FUNIFDENSEKERNEL_HPP
#define FUNIFDENSEKERNEL_HPP

#include "../../Utils/FGlobal.hpp"

#include "../../Utils/FSmartPointer.hpp"

#include "./FAbstractUnifKernel.hpp"
#include "./FUnifM2LHandler.hpp"

class FTreeCoordinate;

/**
 * @author Pierre Blanchard (pierre.blanchard@inria.fr)
 * @class FUnifDenseKernel
 * @brief
 * Please read the license
 *
 * This kernels implement the Lagrange interpolation based FMM operators. It
 * implements all interfaces (P2P,P2M,M2M,M2L,L2L,L2P) which are required by
 * the FFmmAlgorithm and FFmmAlgorithmThread. 
 * This is the dense version of the kernel. The transfer are done in real space
 * and not in Fourier space. 
 *
 * @tparam CellClass Type of cell
 * @tparam ContainerClass Type of container to store particles
 * @tparam MatrixKernelClass Type of matrix kernel function
 * @tparam ORDER Lagrange interpolation order
 */
template < class CellClass,	class ContainerClass,	class MatrixKernelClass, int ORDER, int NVALS = 1>
class FUnifDenseKernel
  : public FAbstractUnifKernel< CellClass, ContainerClass, MatrixKernelClass, ORDER, NVALS>
{
    // private types
    typedef FUnifM2LHandler<ORDER,MatrixKernelClass::Type> M2LHandlerClass;

    // using from
    typedef FAbstractUnifKernel< CellClass, ContainerClass, MatrixKernelClass, ORDER, NVALS>
    AbstractBaseClass;

    /// Needed for P2P and M2L operators
    const MatrixKernelClass *const MatrixKernel;

    /// Needed for M2L operator
    const M2LHandlerClass M2LHandler;


public:
    /**
    * The constructor initializes all constant attributes and it reads the
    * precomputed and compressed M2L operators from a binary file (an
    * runtime_error is thrown if the required file is not valid).
    */
    FUnifDenseKernel(const int inTreeHeight,
                     const FReal inBoxWidth,
                     const FPoint& inBoxCenter,
                     const MatrixKernelClass *const inMatrixKernel)
    : FAbstractUnifKernel< CellClass, ContainerClass, MatrixKernelClass, ORDER, NVALS>(inTreeHeight,inBoxWidth,inBoxCenter),
      MatrixKernel(inMatrixKernel),
      M2LHandler(MatrixKernel,
                 inTreeHeight,
                 inBoxWidth) 
    { }


    void P2M(CellClass* const LeafCell,
             const ContainerClass* const SourceParticles)
    {
        const FPoint LeafCellCenter(AbstractBaseClass::getLeafCellCenter(LeafCell->getCoordinate()));
        for(int idxRhs = 0 ; idxRhs < NVALS ; ++idxRhs){
            // 1) apply Sy
            AbstractBaseClass::Interpolator->applyP2M(LeafCellCenter, AbstractBaseClass::BoxWidthLeaf,
                                                      LeafCell->getMultipole(idxRhs), SourceParticles);
        }
    }


    void M2M(CellClass* const FRestrict ParentCell,
             const CellClass*const FRestrict *const FRestrict ChildCells,
             const int /*TreeLevel*/)
    {
        for(int idxRhs = 0 ; idxRhs < NVALS ; ++idxRhs){
            for (unsigned int ChildIndex=0; ChildIndex < 8; ++ChildIndex){
                if (ChildCells[ChildIndex]){
                    AbstractBaseClass::Interpolator->applyM2M(ChildIndex, ChildCells[ChildIndex]->getMultipole(idxRhs),
                                                              ParentCell->getMultipole(idxRhs));
                }
            }
        }
    }

    void M2L(CellClass* const FRestrict TargetCell,
             const CellClass* SourceCells[343],
             const int /*NumSourceCells*/,
             const int TreeLevel)
    {
        const FReal CellWidth(AbstractBaseClass::BoxWidth / FReal(FMath::pow(2, TreeLevel)));

        // interpolation points of source (Y) and target (X) cell
        FPoint X[AbstractBaseClass::nnodes], Y[AbstractBaseClass::nnodes];
        FUnifTensor<ORDER>::setRoots(AbstractBaseClass::getCellCenter(TargetCell->getCoordinate(),TreeLevel), CellWidth, X);

        for(int idxRhs = 0 ; idxRhs < NVALS ; ++idxRhs){

            for (int idx=0; idx<343; ++idx){
                if (SourceCells[idx]){

                    FUnifTensor<ORDER>::setRoots(AbstractBaseClass::getCellCenter(SourceCells[idx]->getCoordinate(),TreeLevel), CellWidth, Y);

                    for (unsigned int m=0; m<AbstractBaseClass::nnodes; ++m)
                        for (unsigned int n=0; n<AbstractBaseClass::nnodes; ++n){
                            TargetCell->getLocal(idxRhs)[m]+=MatrixKernel->evaluate(X[m], Y[n]) * SourceCells[idx]->getMultipole(idxRhs)[n];
                        }

                }
            }
        }
    }


    void L2L(const CellClass* const FRestrict ParentCell,
             CellClass* FRestrict *const FRestrict ChildCells,
             const int /*TreeLevel*/)
    {
        for(int idxRhs = 0 ; idxRhs < NVALS ; ++idxRhs){
            // 2) apply Sx
            for (unsigned int ChildIndex=0; ChildIndex < 8; ++ChildIndex){
                if (ChildCells[ChildIndex]){
                    AbstractBaseClass::Interpolator->applyL2L(ChildIndex, ParentCell->getLocal(idxRhs), ChildCells[ChildIndex]->getLocal(idxRhs));
                }
            }
        }
    }

    void L2P(const CellClass* const LeafCell,
             ContainerClass* const TargetParticles)
    {
        const FPoint LeafCellCenter(AbstractBaseClass::getLeafCellCenter(LeafCell->getCoordinate()));

        for(int idxRhs = 0 ; idxRhs < NVALS ; ++idxRhs){

            // 2.a) apply Sx
            AbstractBaseClass::Interpolator->applyL2P(LeafCellCenter, AbstractBaseClass::BoxWidthLeaf,
                                                      LeafCell->getLocal(idxRhs), TargetParticles);

            // 2.b) apply Px (grad Sx)
            AbstractBaseClass::Interpolator->applyL2PGradient(LeafCellCenter, AbstractBaseClass::BoxWidthLeaf,
                                                              LeafCell->getLocal(idxRhs), TargetParticles);

        }
    }

    void P2P(const FTreeCoordinate& /* LeafCellCoordinate */, // needed for periodic boundary conditions
             ContainerClass* const FRestrict TargetParticles,
             const ContainerClass* const FRestrict /*SourceParticles*/,
             ContainerClass* const NeighborSourceParticles[27],
             const int /* size */)
    {
        DirectInteractionComputer<MatrixKernelClass::NCMP, NVALS>::P2P(TargetParticles,NeighborSourceParticles,MatrixKernel);
    }


    void P2PRemote(const FTreeCoordinate& /*inPosition*/,
                   ContainerClass* const FRestrict inTargets, const ContainerClass* const FRestrict /*inSources*/,
                   ContainerClass* const inNeighbors[27], const int /*inSize*/)
    {
        DirectInteractionComputer<MatrixKernelClass::NCMP, NVALS>::P2PRemote(inTargets,inNeighbors,27,MatrixKernel);
    }

};


#endif //FUNIFDENSEKERNEL_HPP

// [--END--]
