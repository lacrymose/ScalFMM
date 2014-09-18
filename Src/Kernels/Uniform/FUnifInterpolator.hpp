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
#ifndef FUNIFINTERPOLATOR_HPP
#define FUNIFINTERPOLATOR_HPP


#include "./../Interpolation/FInterpMapping.hpp"

#include "./FUnifTensor.hpp"
#include "./FUnifRoots.hpp"

#include "../../Utils/FBlas.hpp"



/**
 * @author Pierre Blanchard (pierre.blanchard@inria.fr)
 * Please read the license
 */

/**
 * @class FUnifInterpolator
 *
 * The class @p FUnifInterpolator defines the anterpolation (M2M) and
 * interpolation (L2L) concerning operations.
 */
template <int ORDER, class MatrixKernelClass>
class FUnifInterpolator : FNoCopyable
{
  // compile time constants and types
  enum {nnodes = TensorTraits<ORDER>::nnodes,
        nRhs = MatrixKernelClass::NRHS,
        nLhs = MatrixKernelClass::NLHS,
        nPV = MatrixKernelClass::NPV};
  typedef FUnifRoots< ORDER>  BasisType;
  typedef FUnifTensor<ORDER> TensorType;

  unsigned int node_ids[nnodes][3];

  // 8 Non-leaf (i.e. M2M/L2L) interpolators 
  // x1 per level if box is extended
  // only 1 is required for all levels if extension is 0
  FReal*** ChildParentInterpolator;

  // Tree height (needed by M2M/L2L if cell width is extended)
  const int TreeHeight;
  // Root cell width (only used by M2M/L2L)
  const FReal RootCellWidth;
  // Cell width extension (only used by M2M/L2L, kernel handles extension for P2M/L2P)
  const FReal CellWidthExtension;


  // permutations (only needed in the tensor product interpolation case)
  unsigned int perm[3][nnodes];

  ////////////////////////////////////////////////////////////////////


  // PB: use improved version of M2M/L2L
  /**
   * Initialize the child - parent - interpolator, it is basically the matrix
   * S which is precomputed and reused for all M2M and L2L operations, ie for
   * all non leaf inter/anterpolations.
   */
/*
  void initM2MandL2L()
  {
    FPoint ParentRoots[nnodes], ChildRoots[nnodes];
    const FReal ParentWidth(2.);
    const FPoint ParentCenter(0., 0., 0.);
    FUnifTensor<ORDER>::setRoots(ParentCenter, ParentWidth, ParentRoots);

    FPoint ChildCenter;
    const FReal ChildWidth(1.);

    // loop: child cells
    for (unsigned int child=0; child<8; ++child) {

      // allocate memory
      ChildParentInterpolator[child] = new FReal [nnodes * nnodes];

      // set child info
      FUnifTensor<ORDER>::setRelativeChildCenter(child, ChildCenter);
      FUnifTensor<ORDER>::setRoots(ChildCenter, ChildWidth, ChildRoots);

      // assemble child - parent - interpolator
      assembleInterpolator(nnodes, ChildRoots, ChildParentInterpolator[child]);
    }
  }
*/

  /**
   * Initialize the child - parent - interpolator, it is basically the matrix
   * S which is precomputed and reused for all M2M and L2L operations, ie for
   * all non leaf inter/anterpolations.
   */
  void initTensorM2MandL2L(const int TreeLevel, const FReal ParentWidth)
  {
    FReal ChildCoords[3][ORDER];
    FPoint ChildCenter;

    // Ratio of extended cell widths (definition: child ext / parent ext)
    const FReal ExtendedCellRatio = 
      FReal(FReal(ParentWidth)/FReal(2.) + CellWidthExtension) / FReal(ParentWidth + CellWidthExtension);

    // Child cell width
    const FReal ChildWidth(2.*ExtendedCellRatio);

    // loop: child cells
    for (unsigned int child=0; child<8; ++child) {

      // set child info
      FUnifTensor<ORDER>::setRelativeChildCenter(child, ChildCenter, ExtendedCellRatio);
      FUnifTensor<ORDER>::setPolynomialsRoots(ChildCenter, ChildWidth, ChildCoords);

      // allocate memory
      ChildParentInterpolator[TreeLevel][child] = new FReal [3 * ORDER*ORDER];
      assembleInterpolator(ORDER, ChildCoords[0], ChildParentInterpolator[TreeLevel][child]);
      assembleInterpolator(ORDER, ChildCoords[1], ChildParentInterpolator[TreeLevel][child] + 1 * ORDER*ORDER);
      assembleInterpolator(ORDER, ChildCoords[2], ChildParentInterpolator[TreeLevel][child] + 2 * ORDER*ORDER);
    }


    // init permutations
    for (unsigned int i=0; i<ORDER; ++i) {
      for (unsigned int j=0; j<ORDER; ++j) {
        for (unsigned int k=0; k<ORDER; ++k) {
          const unsigned int index = k*ORDER*ORDER + j*ORDER + i;
          perm[0][index] = k*ORDER*ORDER + j*ORDER + i;
          perm[1][index] = i*ORDER*ORDER + k*ORDER + j;
          perm[2][index] = j*ORDER*ORDER + i*ORDER + k;
        }
      }
    }

  }



public:
  /**
   * Constructor: Initialize the Lagrange polynomials at the equispaced
   * roots/interpolation point
     *
     * PB: Input parameters ONLY affect the computation of the M2M/L2L ops.
     * These parameters are ONLY required in the context of extended bbox.
     * If no M2M/L2L is required then the interpolator can be built with 
     * the default ctor.
   */
  explicit FUnifInterpolator(const int inTreeHeight=3,
                             const FReal inRootCellWidth=FReal(1.), 
                             const FReal inCellWidthExtension=FReal(0.))
  : TreeHeight(inTreeHeight), 
    RootCellWidth(inRootCellWidth),
    CellWidthExtension(inCellWidthExtension)
  {
    // initialize root node ids
    TensorType::setNodeIds(node_ids);

    // initialize interpolation operator for M2M and L2L (non leaf operations)

    // allocate 8 arrays per level
    ChildParentInterpolator = new FReal**[TreeHeight];
    for ( int l=0; l<TreeHeight; ++l){
      ChildParentInterpolator[l] = new FReal*[8];
      for (unsigned int c=0; c<8; ++c)
        ChildParentInterpolator[l][c]=nullptr;        
    }

    // Set number of non-leaf ios that actually need to be computed
    unsigned int reducedTreeHeight; // = 2 + nb of computed nl ios
    if(CellWidthExtension==0.) // if no cell extension, then ...
      reducedTreeHeight = 3; // cmp only 1 non-leaf io
    else 
      reducedTreeHeight = TreeHeight; // cmp 1 non-leaf io per level

    // Init non-leaf interpolators
    FReal CellWidth = RootCellWidth / FReal(2.); // at level 1
    CellWidth /= FReal(2.);                      // at level 2
        
    for (unsigned int l=2; l<reducedTreeHeight; ++l) {
          
      //this -> initM2MandL2L(l,CellWidth);     // non tensor-product interpolation
      this -> initTensorM2MandL2L(l,CellWidth); // tensor-product interpolation

      // update cell width
      CellWidth /= FReal(2.);                    // at level l+1 
    }
  }


  /**
   * Destructor: Delete dynamically allocated memory for M2M and L2L operator
   */
  ~FUnifInterpolator()
  {
    for ( int l=0; l<TreeHeight; ++l)
      for (unsigned int child=0; child<8; ++child)
        if(ChildParentInterpolator[l][child] != nullptr)
          delete [] ChildParentInterpolator[l][child];
  }


  /**
   * Assembles the interpolator \f$S_\ell\f$ of size \f$N\times
   * \ell^3\f$. Here local points is meant as points whose global coordinates
   * have already been mapped to the reference interval [-1,1].
   *
   * @param[in] NumberOfLocalPoints
   * @param[in] LocalPoints
   * @param[out] Interpolator
   */
  void assembleInterpolator(const unsigned int NumberOfLocalPoints,
                            const FPoint *const LocalPoints,
                            FReal *const Interpolator) const
  {
    // values of Lagrange polynomials of source particle: L_o(x_i)
    FReal L_of_x[ORDER][3];
    // loop: local points (mapped in [-1,1])
    for (unsigned int m=0; m<NumberOfLocalPoints; ++m) {
      // evaluate Lagrange polynomials at local points
      for (unsigned int o=0; o<ORDER; ++o) {
        L_of_x[o][0] = BasisType::L(o, LocalPoints[m].getX());
        L_of_x[o][1] = BasisType::L(o, LocalPoints[m].getY());
        L_of_x[o][2] = BasisType::L(o, LocalPoints[m].getZ());
      }

      // assemble interpolator
      for (unsigned int n=0; n<nnodes; ++n) {
        Interpolator[n*NumberOfLocalPoints + m] = FReal(1.);
        for (unsigned int d=0; d<3; ++d) {
          const unsigned int j = node_ids[n][d];
          // The Lagrange case is much simpler than the Chebyshev case
          // as no summation is required
          Interpolator[n*NumberOfLocalPoints + m] *= L_of_x[j][d];
        }

      }

    }

  }


  void assembleInterpolator(const unsigned int M, const FReal *const x, FReal *const S) const
  {
    // loop: local points (mapped in [-1,1])
    for (unsigned int m=0; m<M; ++m) {
      // evaluate Lagrange polynomials at local points
      for (unsigned int o=0; o<ORDER; ++o)
        S[o*M + m] = BasisType::L(o, x[m]);

    }

  }



  const FReal *const * getChildParentInterpolator() const
  { return ChildParentInterpolator; }
  const unsigned int * getPermutationsM2ML2L(unsigned int i) const
  { return perm[i]; }






  /**
   * Particle to moment: application of \f$S_\ell(y,\bar y_n)\f$
   * (anterpolation, it is the transposed interpolation)
   */
  template <class ContainerClass>
  void applyP2M(const FPoint& center,
                const FReal width,
                FReal *const multipoleExpansion,
                const ContainerClass *const sourceParticles) const;



  /**
   * Local to particle operation: application of \f$S_\ell(x,\bar x_m)\f$ (interpolation)
   */
  template <class ContainerClass>
  void applyL2P(const FPoint& center,
                const FReal width,
                const FReal *const localExpansion,
                ContainerClass *const localParticles) const;


  /**
   * Local to particle operation: application of \f$\nabla_x S_\ell(x,\bar x_m)\f$ (interpolation)
   */
  template <class ContainerClass>
  void applyL2PGradient(const FPoint& center,
                        const FReal width,
                        const FReal *const localExpansion,
                        ContainerClass *const localParticles) const;

  /**
   * Local to particle operation: application of \f$S_\ell(x,\bar x_m)\f$ and
   * \f$\nabla_x S_\ell(x,\bar x_m)\f$ (interpolation)
   */
  template <class ContainerClass>
  void applyL2PTotal(const FPoint& center,
                     const FReal width,
                     const FReal *const localExpansion,
                     ContainerClass *const localParticles) const;

  // PB: ORDER^6 version of applyM2M/L2L
  /*
    void applyM2M(const unsigned int ChildIndex,
    const FReal *const ChildExpansion,
    FReal *const ParentExpansion) const
    {
    FBlas::gemtva(nnodes, nnodes, FReal(1.),
    ChildParentInterpolator[ChildIndex],
    const_cast<FReal*>(ChildExpansion), ParentExpansion);
    }

    void applyL2L(const unsigned int ChildIndex,
    const FReal *const ParentExpansion,
    FReal *const ChildExpansion) const
    {
    FBlas::gemva(nnodes, nnodes, FReal(1.),
    ChildParentInterpolator[ChildIndex],
    const_cast<FReal*>(ParentExpansion), ChildExpansion);
    }
  */

  // PB: improved version of applyM2M/L2L also applies to Lagrange interpolation
  // PB: Multidim version handled in kernel !
  void applyM2M(const unsigned int ChildIndex,
                const FReal *const ChildExpansion,
                FReal *const ParentExpansion,
                const unsigned int TreeLevel = 2) const
  {
    FReal Exp[nnodes], PermExp[nnodes];
    // ORDER*ORDER*ORDER * (2*ORDER-1)
    FBlas::gemtm(ORDER, ORDER, ORDER*ORDER, FReal(1.),
                 ChildParentInterpolator[TreeLevel][ChildIndex], ORDER,
                 const_cast<FReal*>(ChildExpansion), ORDER, PermExp, ORDER);

    for (unsigned int n=0; n<nnodes; ++n)	Exp[n] = PermExp[perm[1][n]];
    // ORDER*ORDER*ORDER * (2*ORDER-1)
    FBlas::gemtm(ORDER, ORDER, ORDER*ORDER, FReal(1.),
                 ChildParentInterpolator[TreeLevel][ChildIndex] + 2 * ORDER*ORDER, ORDER,
                 Exp, ORDER, PermExp, ORDER);

    for (unsigned int n=0; n<nnodes; ++n)	Exp[perm[1][n]] = PermExp[perm[2][n]];
    // ORDER*ORDER*ORDER * (2*ORDER-1)
    FBlas::gemtm(ORDER, ORDER, ORDER*ORDER, FReal(1.),
                 ChildParentInterpolator[TreeLevel][ChildIndex] + 1 * ORDER*ORDER, ORDER,
                 Exp, ORDER, PermExp, ORDER);

    for (unsigned int n=0; n<nnodes; ++n)	ParentExpansion[perm[2][n]] += PermExp[n];
  }


  void applyL2L(const unsigned int ChildIndex,
                const FReal *const ParentExpansion,
                FReal *const ChildExpansion,
                const unsigned int TreeLevel = 2) const
  {
    FReal Exp[nnodes], PermExp[nnodes];
    // ORDER*ORDER*ORDER * (2*ORDER-1)
    FBlas::gemm(ORDER, ORDER, ORDER*ORDER, FReal(1.),
                ChildParentInterpolator[TreeLevel][ChildIndex], ORDER,
                const_cast<FReal*>(ParentExpansion), ORDER, PermExp, ORDER);

    for (unsigned int n=0; n<nnodes; ++n)	Exp[n] = PermExp[perm[1][n]];
    // ORDER*ORDER*ORDER * (2*ORDER-1)
    FBlas::gemm(ORDER, ORDER, ORDER*ORDER, FReal(1.),
                ChildParentInterpolator[TreeLevel][ChildIndex] + 2 * ORDER*ORDER, ORDER,
                Exp, ORDER, PermExp, ORDER);

    for (unsigned int n=0; n<nnodes; ++n)	Exp[perm[1][n]] = PermExp[perm[2][n]];
    // ORDER*ORDER*ORDER * (2*ORDER-1)
    FBlas::gemm(ORDER, ORDER, ORDER*ORDER, FReal(1.),
                ChildParentInterpolator[TreeLevel][ChildIndex] + 1 * ORDER*ORDER, ORDER,
                Exp, ORDER, PermExp, ORDER);

    for (unsigned int n=0; n<nnodes; ++n)	ChildExpansion[perm[2][n]] += PermExp[n];
  }
  // total flops count: 3 * ORDER*ORDER*ORDER * (2*ORDER-1)
};







/**
 * Particle to moment: application of \f$S_\ell(y,\bar y_n)\f$
 * (anterpolation, it is the transposed interpolation)
 */
template <int ORDER, class MatrixKernelClass>
template <class ContainerClass>
inline void FUnifInterpolator<ORDER,MatrixKernelClass>::applyP2M(const FPoint& center,
                                                                 const FReal width,
                                                                 FReal *const multipoleExpansion,
                                                                 const ContainerClass *const inParticles) const
{
  // set all multipole expansions to zero
  // Should not be set to zero here (TODO remove this line) FBlas::setzero(nRhs*nnodes, multipoleExpansion);

  // allocate stuff
  const map_glob_loc map(center, width);
  FPoint localPosition;

  // loop over source particles
  const FReal*const positionsX = inParticles->getPositions()[0];
  const FReal*const positionsY = inParticles->getPositions()[1];
  const FReal*const positionsZ = inParticles->getPositions()[2];

  for(int idxPart = 0 ; idxPart < inParticles->getNbParticles() ; ++idxPart){
    // map global position to [-1,1]
    map(FPoint(positionsX[idxPart],positionsY[idxPart],positionsZ[idxPart]), localPosition); // 15 flops
    // evaluate Lagrange polynomial at local position
    FReal L_of_x[ORDER][3];
    for (unsigned int o=0; o<ORDER; ++o) {
      L_of_x[o][0] = BasisType::L(o, localPosition.getX()); // 3 * ORDER*(ORDER-1) flops PB: TODO confirm
      L_of_x[o][1] = BasisType::L(o, localPosition.getY()); // 3 * ORDER*(ORDER-1) flops
      L_of_x[o][2] = BasisType::L(o, localPosition.getZ()); // 3 * ORDER*(ORDER-1) flops
    }

    for(int idxRhs = 0 ; idxRhs < nRhs ; ++idxRhs){

      // read physicalValue
      const FReal*const physicalValues = inParticles->getPhysicalValues(idxRhs);
      // compute weight
      const FReal weight = physicalValues[idxPart];

      // assemble multipole expansions
      for (unsigned int i=0; i<ORDER; ++i) {
        for (unsigned int j=0; j<ORDER; ++j) {
          for (unsigned int k=0; k<ORDER; ++k) {
            const unsigned int idx = idxRhs*nnodes + k*ORDER*ORDER + j*ORDER + i;
            multipoleExpansion[idx] += L_of_x[i][0] * L_of_x[j][1] * L_of_x[k][2] * weight; // 3 * ORDER*ORDER*ORDER flops
          }
        }
      }

    } // idxRhs

  } // flops: N * (3 * ORDER*ORDER*ORDER + 3 * 3 * ORDER*(ORDER-1)) flops

}


/**
 * Local to particle operation: application of \f$S_\ell(x,\bar x_m)\f$ (interpolation)
 */
template <int ORDER, class MatrixKernelClass>
template <class ContainerClass>
inline void FUnifInterpolator<ORDER,MatrixKernelClass>::applyL2P(const FPoint& center,
                                                                 const FReal width,
                                                                 const FReal *const localExpansion,
                                                                 ContainerClass *const inParticles) const
{
  // loop over particles
  const map_glob_loc map(center, width);
  FPoint localPosition;

  const FReal*const positionsX = inParticles->getPositions()[0];
  const FReal*const positionsY = inParticles->getPositions()[1];
  const FReal*const positionsZ = inParticles->getPositions()[2];

  const  int nParticles = inParticles->getNbParticles();

  for(int idxPart = 0 ; idxPart < nParticles ; ++ idxPart){

    // map global position to [-1,1]
    map(FPoint(positionsX[idxPart],positionsY[idxPart],positionsZ[idxPart]), localPosition); // 15 flops

    // evaluate Lagrange polynomial at local position
    FReal L_of_x[ORDER][3];
    for (unsigned int o=0; o<ORDER; ++o) {
      L_of_x[o][0] = BasisType::L(o, localPosition.getX()); // 3 * ORDER*(ORDER-1) flops
      L_of_x[o][1] = BasisType::L(o, localPosition.getY()); // 3 * ORDER*(ORDER-1) flops
      L_of_x[o][2] = BasisType::L(o, localPosition.getZ()); // 3 * ORDER*(ORDER-1) flops
    }

    // loop over dim of local expansions
    for(int idxLhs = 0 ; idxLhs < nLhs ; ++idxLhs){
      // distribution over potential components:
      // We sum the multidim contribution of PhysValue
      // This was originally done at M2L step but moved here 
      // because their storage is required by the force computation.
      // In fact : f_{ik}(x)=w_j(x) \nabla_{x_i} K_{ij}(x,y)w_j(y))
      const unsigned int idxPot = idxLhs / nPV; 

      // interpolate and increment target value
      FReal targetValue=0.;
      {
        for (unsigned int l=0; l<ORDER; ++l) {
          for (unsigned int m=0; m<ORDER; ++m) {
            for (unsigned int n=0; n<ORDER; ++n) {
              const unsigned int idx = idxLhs*nnodes + n*ORDER*ORDER + m*ORDER + l;
              targetValue +=
                L_of_x[l][0] * L_of_x[m][1] * L_of_x[n][2] * localExpansion[idx];
            } // ORDER * 4 flops
          } // ORDER * ORDER * 4 flops
        } // ORDER * ORDER * ORDER * 4 flops
      }

      // get potential
      FReal*const potentials = inParticles->getPotentials(idxPot);
      // add contribution to potential
      potentials[idxPart] += (targetValue);

    } // idxLhs

  } // N * (4 * ORDER * ORDER * ORDER + 9 * ORDER*(ORDER-1) ) flops
}



/**
 * Local to particle operation: application of \f$\nabla_x S_\ell(x,\bar x_m)\f$ (interpolation)
 */
template <int ORDER, class MatrixKernelClass>
template <class ContainerClass>
inline void FUnifInterpolator<ORDER,MatrixKernelClass>::applyL2PGradient(const FPoint& center,
                                                                         const FReal width,
                                                                         const FReal *const localExpansion,
                                                                         ContainerClass *const inParticles) const
{
  ////////////////////////////////////////////////////////////////////
  // TENSOR-PRODUCT INTERPOLUTION NOT IMPLEMENTED YET HERE!!! ////////
  ////////////////////////////////////////////////////////////////////

  // setup local to global mapping
  const map_glob_loc map(center, width);
  FPoint Jacobian;
  map.computeJacobian(Jacobian);
  const FReal jacobian[3] = {Jacobian.getX(), Jacobian.getY(), Jacobian.getZ()};
  FPoint localPosition;
  FReal L_of_x[ORDER][3];
  FReal dL_of_x[ORDER][3];

  const FReal*const positionsX = inParticles->getPositions()[0];
  const FReal*const positionsY = inParticles->getPositions()[1];
  const FReal*const positionsZ = inParticles->getPositions()[2];

//  const unsigned int nParticles = inParticles->getNbParticles();

  for(int idxPart = 0 ; idxPart < inParticles->getNbParticles() ; ++ idxPart){

    // map global position to [-1,1]
    map(FPoint(positionsX[idxPart],positionsY[idxPart],positionsZ[idxPart]), localPosition);

    // evaluate Lagrange polynomials of source particle
    for (unsigned int o=0; o<ORDER; ++o) {
      L_of_x[o][0] = BasisType::L(o, localPosition.getX()); // 3 * ORDER*(ORDER-1) flops 
      L_of_x[o][1] = BasisType::L(o, localPosition.getY()); // 3 * ORDER*(ORDER-1) flops
      L_of_x[o][2] = BasisType::L(o, localPosition.getZ()); // 3 * ORDER*(ORDER-1) flops
      dL_of_x[o][0] = BasisType::dL(o, localPosition.getX()); // TODO verify 3 * ORDER*(ORDER-1) flops
      dL_of_x[o][1] = BasisType::dL(o, localPosition.getY()); // TODO verify 3 * ORDER*(ORDER-1) flops
      dL_of_x[o][2] = BasisType::dL(o, localPosition.getZ()); // TODO verify 3 * ORDER*(ORDER-1) flops
    }

    for(int idxLhs = 0 ; idxLhs < nLhs ; ++idxLhs){
      const unsigned int idxPot = idxLhs / nPV; 
      const unsigned int idxPV  = idxLhs % nPV; 

      // interpolate and increment forces value
      FReal forces[3] = {FReal(0.), FReal(0.), FReal(0.)};
      {
        for (unsigned int l=0; l<ORDER; ++l) {
          for (unsigned int m=0; m<ORDER; ++m) {
            for (unsigned int n=0; n<ORDER; ++n) {
              const unsigned int idx = idxLhs*nnodes + n*ORDER*ORDER + m*ORDER + l;
              forces[0] +=
                dL_of_x[l][0] * L_of_x[m][1] * L_of_x[n][2] * localExpansion[idx];
              forces[1] +=
                L_of_x[l][0] * dL_of_x[m][1] * L_of_x[n][2] * localExpansion[idx];
              forces[2] +=
                L_of_x[l][0] * L_of_x[m][1] * dL_of_x[n][2] * localExpansion[idx];

            } // ORDER * 4 flops
          } // ORDER * ORDER * 4 flops
        } // ORDER * ORDER * ORDER * 4 flops

        // scale forces
        forces[0] *= jacobian[0];
        forces[1] *= jacobian[1];
        forces[2] *= jacobian[2];
      }

      const FReal*const physicalValues = inParticles->getPhysicalValues(idxPV);
      FReal*const forcesX = inParticles->getForcesX(idxPot);
      FReal*const forcesY = inParticles->getForcesY(idxPot);
      FReal*const forcesZ = inParticles->getForcesZ(idxPot);

      // set computed forces
      forcesX[idxPart] += forces[0] * physicalValues[idxPart];
      forcesY[idxPart] += forces[1] * physicalValues[idxPart];
      forcesZ[idxPart] += forces[2] * physicalValues[idxPart];
    }// idxLhs

  }

}


#endif /* FUNIFINTERPOLATOR_HPP */
