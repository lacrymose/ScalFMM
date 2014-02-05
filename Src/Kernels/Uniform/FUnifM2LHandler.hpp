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
#ifndef FUNIFM2LHANDLER_HPP
#define FUNIFM2LHANDLER_HPP

#include <numeric>
#include <stdexcept>
#include <string>
#include <sstream>
#include <fstream>
#include <typeinfo>

#include "../../Utils/FBlas.hpp"
#include "../../Utils/FTic.hpp"
#include "../../Utils/FDft.hpp"

#include "../../Utils/FComplexe.hpp"


#include "./FUnifTensor.hpp"

/*!  Precomputation of the 316 interactions by evaluation of the matrix kernel on the uniform grid and transformation into Fourier space. 
PB: Compute() does not belong to the M2LHandler like it does in the Chebyshev kernel. This allows much nicer specialization of the M2LHandler class with respect to the homogeneity of the kernel of interaction like in the ChebyshevSym kernel.*/
template <int ORDER, typename MatrixKernelClass>
static void Compute(const MatrixKernelClass *const MatrixKernel, const FReal CellWidth, FComplexe* &FC)
{
	// allocate memory and store compressed M2L operators
  if (FC) throw std::runtime_error("M2L operators are already set");

  // PB: need to redefine some constant since not function from m2lhandler class
  const unsigned int order = ORDER;
  const unsigned int nnodes = TensorTraits<ORDER>::nnodes;
  const unsigned int ninteractions = 316;

  typedef FUnifTensor<ORDER> TensorType;

	// interpolation points of source (Y) and target (X) cell
	FPoint X[nnodes], Y[nnodes];
	// set roots of target cell (X)
	TensorType::setRoots(FPoint(0.,0.,0.), CellWidth, X);

	// allocate memory and compute 316 m2l operators
	FReal *_C;
	FComplexe *_FC;

  // reduce storage from nnodes^2=order^6 to (2order-1)^3
  const unsigned int rc = (2*order-1)*(2*order-1)*(2*order-1);
	_C = new FReal [rc];
	_FC = new FComplexe [rc * ninteractions];

  // initialize root node ids pairs
  unsigned int node_ids_pairs[rc][2];
  TensorType::setNodeIdsPairs(node_ids_pairs);

  // init Discrete Fourier Transformator
//	FDft Dft(rc);
	FFft<1> Dft(rc);

  // get first column of K via permutation
  unsigned int perm[rc];
  TensorType::setStoragePermutation(perm);

	unsigned int counter = 0;
	for (int i=-3; i<=3; ++i) {
		for (int j=-3; j<=3; ++j) {
			for (int k=-3; k<=3; ++k) {
				if (abs(i)>1 || abs(j)>1 || abs(k)>1) {
					// set roots of source cell (Y)
					const FPoint cy(CellWidth*FReal(i), CellWidth*FReal(j), CellWidth*FReal(k));
					FUnifTensor<order>::setRoots(cy, CellWidth, Y);
					// evaluate m2l operator
          unsigned int ido=0;
          for(unsigned int l=0; l<2*order-1; ++l)
            for(unsigned int m=0; m<2*order-1; ++m)
              for(unsigned int n=0; n<2*order-1; ++n){   
          
                // store value at current position in C
                // use permutation if DFT is used because 
                // the storage of the first column is required
                // i.e. C[0] C[rc-1] C[rc-2] .. C[1] < WRONG!
                // i.e. C[rc-1] C[0] C[1] .. C[rc-2] < RIGHT!
                //                _C[counter*rc + ido]
                _C[perm[ido]]
                  = MatrixKernel->evaluate(X[node_ids_pairs[ido][0]], 
                                          Y[node_ids_pairs[ido][1]]);
                ido++;
              }

          // Apply Discrete Fourier Transformation
          Dft.applyDFT(_C,_FC+counter*rc);

					// increment interaction counter
					counter++;
				}
			}
		}
	}
	if (counter != ninteractions)
		throw std::runtime_error("Number of interactions must correspond to 316");

  // Free _C
	delete [] _C;

	// store FC
	counter = 0;
  // reduce storage if real valued kernel
  const unsigned int opt_rc = rc/2+1;
  // allocate M2L
	FC = new FComplexe[343 * opt_rc];

	for (int i=-3; i<=3; ++i)
		for (int j=-3; j<=3; ++j)
			for (int k=-3; k<=3; ++k) {
				const unsigned int idx = (i+3)*7*7 + (j+3)*7 + (k+3);
				if (abs(i)>1 || abs(j)>1 || abs(k)>1) {
					FBlas::c_copy(opt_rc, reinterpret_cast<FReal*>(_FC + counter*rc), 
                        reinterpret_cast<FReal*>(FC + idx*opt_rc));
//          for (unsigned int n=0; n<rc; ++n){
//            FC[idx*rc+n]=_FC[counter*rc+n];
//          }
					counter++;
				} else{ 
          FBlas::c_setzero(opt_rc, reinterpret_cast<FReal*>(FC + idx*opt_rc));
//          for (unsigned int n=0; n<rc; ++n){
//            FC[idx*rc+n]=FComplexe(0.0,0.0);
//          }
        }
      }

	if (counter != ninteractions)
		throw std::runtime_error("Number of interactions must correspond to 316");
  delete [] _FC;  	
}




/**
 * @author Pierre Blanchard (pierre.blanchard@inria.fr)
 * @class FUnifM2LHandler
 * Please read the license
 *
 * This class precomputes and efficiently stores the M2L operators
 * \f$[K_1,\dots,K_{316}]\f$ for all (\f$7^3-3^3 = 316\f$ possible interacting
 * cells in the far-field) interactions for the Lagrange interpolation
 * approach. The resulting Lagrange operators have a Circulant Toeplitz 
 * structure and can be applied efficiently in Fourier Space. Hence, the
 * originally \f$K_t\f$ of size \f$\ell^3\times\ell^3\f$ times \f$316\f$ for
 * all interactions is reduced to \f$316\f$ \f$C_t\f$, each of size \f$2\ell-1\f$.
 *
 * @tparam ORDER interpolation order \f$\ell\f$
 */
template <int ORDER, KERNEL_FUNCTION_TYPE TYPE> class FUnifM2LHandler;

/*! Specialization for homogeneous kernel functions */
template <int ORDER>
class FUnifM2LHandler<ORDER,HOMOGENEOUS> : FNoCopyable
{
	enum {order = ORDER,
				nnodes = TensorTraits<ORDER>::nnodes,
				ninteractions = 316, // 7^3 - 3^3 (max num cells in far-field)
        rc = (2*ORDER-1)*(2*ORDER-1)*(2*ORDER-1)};


	FComplexe *FC;

  // for real valued kernel only n/2+1 complex values are stored 
  // after performing the DFT (the rest is deduced by conjugation)
  unsigned int opt_rc; 

  typedef FUnifTensor<ORDER> TensorType;
  unsigned int node_diff[nnodes*nnodes];

  //  FDft Dft; // Direct Discrete Fourier Transformator
  FFft<1> Dft; // Fast Discrete Fourier Transformator

	static const std::string getFileName()
	{
		const char precision_type = (typeid(FReal)==typeid(double) ? 'd' : 'f');
		std::stringstream stream;
		stream << "m2l_k"/*<< MatrixKernelClass::Identifier*/ << "_" << precision_type
					 << "_o" << order << ".bin";
		return stream.str();
	}

	
public:
	template <typename MatrixKernelClass>
	FUnifM2LHandler(const MatrixKernelClass *const MatrixKernel, const unsigned int, const FReal)
		: FC(NULL),
      opt_rc(rc/2+1),
      Dft(rc) // initialize Discrete Fourier Transformator
	{    
    // initialize root node ids
    TensorType::setNodeIdsDiff(node_diff);

    // Compute and Set M2L Operators
    ComputeAndSet(MatrixKernel);

  }

	~FUnifM2LHandler()
	{
		if (FC != NULL) delete [] FC;
	}

	/**
	 * Computes and sets the matrix \f$C_t\f$
	 */
	template <typename MatrixKernelClass>
	void ComputeAndSet(const MatrixKernelClass *const MatrixKernel)
	{
		// measure time
		FTic time; time.tic();
		// check if aready set
    if (FC) throw std::runtime_error("M2L operator already set");
    // Compute matrix of interactions
		const FReal ReferenceCellWidth = FReal(2.);
		Compute<order>(MatrixKernel,ReferenceCellWidth,FC);

    // Compute memory usage
    unsigned long sizeM2L = 343*opt_rc*sizeof(FComplexe);


		// write info
		std::cout << "Compute and Set M2L operators ("<< long(sizeM2L/**1e-6*/) <<" Bytes) in "
							<< time.tacAndElapsed() << "sec."	<< std::endl;
	}
		

  /**
	 * Expands potentials \f$x+=IDFT(X)\f$ of a target cell. This operation can be
	 * seen as part of the L2L operation.
	 *
	 * @param[in] X transformed local expansion of size \f$r\f$
	 * @param[out] x local expansion of size \f$\ell^3\f$
	 */
  void unapplyZeroPaddingAndDFT(const FComplexe *const FX, FReal *const x) const
  { 
    FReal Px[rc];
    FBlas::setzero(rc,Px);
    // Apply forward Discrete Fourier Transform
    Dft.applyIDFT(FX,Px);

    // Unapply Zero Padding
    for (unsigned int j=0; j<nnodes; ++j)
      x[j]+=Px[node_diff[nnodes-j-1]];
  }

  /**
	 * The M2L operation \f$X+=C_tY\f$ is performed in Fourier space by 
   * application of the convolution theorem (i.e. \f$FX+=FC_t:FY\f$), 
   * where \f$FY\f$ is the transformed multipole expansion and 
   * \f$FX\f$ is the transformed local expansion, both padded with zeros and
	 * of size \f$r_c=(2\times ORDER-1)^3\f$ or \f$r_c^{opt}=\frac{r_c}{2}+1\f$ 
   * for real valued kernels. The index \f$t\f$ denotes the transfer vector 
	 * of the target cell to the source cell.
	 *
	 * @param[in] transfer transfer vector
	 * @param[in] FY transformed multipole expansion
	 * @param[out] FX transformed local expansion
	 * @param[in] CellWidth needed for the scaling of the compressed M2L operators which are based on a homogeneous matrix kernel computed for the reference cell width \f$w=2\f$, ie in \f$[-1,1]^3\f$.
	 */
  void applyFC(const unsigned int idx, const unsigned int, const FReal scale,
               const FComplexe *const FY, FComplexe *const FX) const
  {

    FComplexe tmpFX; 

    // Perform entrywise product manually
    for (unsigned int j=0; j<opt_rc; ++j){
      tmpFX=FC[idx*opt_rc + j];
      tmpFX*=FY[j];
      tmpFX*=scale;
      FX[j]+=tmpFX;
    }

//    // Perform entrywise product using BLAS and MKL routines
//    // PB: not necessary faster than the naive version
//    FComplexe tmpFX[rc]; 
//    FBlas::c_setzero(rc,reinterpret_cast<FReal*>(tmpFX));
//    FMkl::c_had(rc,reinterpret_cast<const FReal* const>(FC + idx*rc),
//                reinterpret_cast<const FReal* const>(FY),
//                reinterpret_cast<FReal* const>(tmpFX));
//    // Scale
//    FBlas::c_axpy(rc,&scale,reinterpret_cast<FReal* const>(tmpFX),
//                  reinterpret_cast<FReal* const>(FX));

  }


  /**
	 * Transform densities \f$Y= DFT(y)\f$ of a source cell. This operation
	 * can be seen as part of the M2M operation.
	 *
	 * @param[in] y multipole expansion of size \f$\ell^3\f$
	 * @param[out] Y transformed multipole expansion of size \f$r\f$
	 */
  void applyZeroPaddingAndDFT(FReal *const y, FComplexe *const FY) const
  {
    FReal Py[rc];
    FBlas::setzero(rc,Py);

    // Apply Zero Padding
    for (unsigned int i=0; i<nnodes; ++i)
      Py[node_diff[i*nnodes]]=y[i];

    // Apply forward Discrete Fourier Transform
    Dft.applyDFT(Py,FY);

  }


};


/*! Specialization for non-homogeneous kernel functions */
template <int ORDER>
class FUnifM2LHandler<ORDER,NON_HOMOGENEOUS> : FNoCopyable
{
	enum {order = ORDER,
				nnodes = TensorTraits<ORDER>::nnodes,
				ninteractions = 316, // 7^3 - 3^3 (max num cells in far-field)
        rc = (2*ORDER-1)*(2*ORDER-1)*(2*ORDER-1)};

  // Homogeneity specific
	FComplexe** FC;

  const unsigned int TreeHeight;
  const FReal RootCellWidth;

  // for real valued kernel only n/2+1 complex values are stored 
  // after performing the DFT (the rest is deduced by conjugation)
  unsigned int opt_rc; 

  typedef FUnifTensor<ORDER> TensorType;
  unsigned int node_diff[nnodes*nnodes];

  //  FDft Dft; // Direct Discrete Fourier Transformator
  FFft<1> Dft; // Fast Discrete Fourier Transformator

	static const std::string getFileName()
	{
		const char precision_type = (typeid(FReal)==typeid(double) ? 'd' : 'f');
		std::stringstream stream;
		stream << "m2l_k"/*<< MatrixKernelClass::Identifier*/ << "_" << precision_type
					 << "_o" << order << ".bin";
		return stream.str();
	}

	
public:
	template <typename MatrixKernelClass>
	FUnifM2LHandler(const MatrixKernelClass *const MatrixKernel, const unsigned int inTreeHeight, const FReal inRootCellWidth)
		: TreeHeight(inTreeHeight),
      RootCellWidth(inRootCellWidth),
      opt_rc(rc/2+1),
      Dft(rc) // initialize Discrete Fourier Transformator
	{    
    // initialize root node ids
    TensorType::setNodeIdsDiff(node_diff);
    
    // init M2L operators
    FC = new FComplexe*[TreeHeight];
    FC[0]       = NULL; FC[1]       = NULL;
		for (unsigned int l=2; l<TreeHeight; ++l) FC[l] = NULL;

    // Compute and Set M2L Operators
    ComputeAndSet(MatrixKernel);

  }

	~FUnifM2LHandler()
	{
    for (unsigned int l=0; l<TreeHeight; ++l) 
      if (FC[l] != NULL) delete [] FC[l];
	}

	/**
	 * Computes and sets the matrix \f$C_t\f$
	 */
	template <typename MatrixKernelClass>
	void ComputeAndSet(const MatrixKernelClass *const MatrixKernel)
	{
		// measure time
		FTic time; time.tic();

    // Compute matrix of interactions at each level !! (since non homog)
		FReal CellWidth = RootCellWidth / FReal(2.); // at level 1
		CellWidth /= FReal(2.);                      // at level 2
		for (unsigned int l=2; l<TreeHeight; ++l) {
//			precompute<ORDER>(MatrixKernel, CellWidth, Epsilon, K[l], LowRank[l]);
      // check if aready set
      if (FC[l]) throw std::runtime_error("M2L operator already set");
      Compute<order>(MatrixKernel,CellWidth,FC[l]);
			CellWidth /= FReal(2.);                    // at level l+1 
		}

    // Compute memory usage
    unsigned long sizeM2L = (TreeHeight-1)*343*opt_rc*sizeof(FComplexe);


		// write info
		std::cout << "Compute and Set M2L operators of " << TreeHeight-2 << " levels ("<< long(sizeM2L/**1e-6*/) <<" Bytes) in "
							<< time.tacAndElapsed() << "sec."	<< std::endl;
	}


  /**
	 * Expands potentials \f$x+=IDFT(X)\f$ of a target cell. This operation can be
	 * seen as part of the L2L operation.
	 *
	 * @param[in] X transformed local expansion of size \f$r\f$
	 * @param[out] x local expansion of size \f$\ell^3\f$
	 */
  void unapplyZeroPaddingAndDFT(const FComplexe *const FX, FReal *const x) const
  { 
    FReal Px[rc];
    FBlas::setzero(rc,Px);
    // Apply forward Discrete Fourier Transform
    Dft.applyIDFT(FX,Px);

    // Unapply Zero Padding
    for (unsigned int j=0; j<nnodes; ++j)
      x[j]+=Px[node_diff[nnodes-j-1]];
  }

  /**
	 * The M2L operation \f$X+=C_tY\f$ is performed in Fourier space by 
   * application of the convolution theorem (i.e. \f$FX+=FC_t:FY\f$), 
   * where \f$FY\f$ is the transformed multipole expansion and 
   * \f$FX\f$ is the transformed local expansion, both padded with zeros and
	 * of size \f$r_c=(2\times ORDER-1)^3\f$ or \f$r_c^{opt}=\frac{r_c}{2}+1\f$ 
   * for real valued kernels. The index \f$t\f$ denotes the transfer vector 
	 * of the target cell to the source cell.
	 *
	 * @param[in] transfer transfer vector
	 * @param[in] FY transformed multipole expansion
	 * @param[out] FX transformed local expansion
	 * @param[in] CellWidth needed for the scaling of the compressed M2L operators which are based on a homogeneous matrix kernel computed for the reference cell width \f$w=2\f$, ie in \f$[-1,1]^3\f$.
	 */
  void applyFC(const unsigned int idx, const unsigned int TreeLevel, const FReal,
               const FComplexe *const FY, FComplexe *const FX) const
  {

    FComplexe tmpFX; 

    // Perform entrywise product manually
    for (unsigned int j=0; j<opt_rc; ++j){
      tmpFX=FC[TreeLevel][idx*opt_rc + j];
      tmpFX*=FY[j];
      FX[j]+=tmpFX;
    }

//    // Perform entrywise product using BLAS and MKL routines
//    // PB: not necessary faster than the naive version
//    FComplexe tmpFX[rc]; 
//    FBlas::c_setzero(rc,reinterpret_cast<FReal*>(tmpFX));
//    FMkl::c_had(rc,reinterpret_cast<const FReal* const>(FC + idx*rc),
//                reinterpret_cast<const FReal* const>(FY),
//                reinterpret_cast<FReal* const>(tmpFX));
//    // Scale
//    FBlas::c_axpy(rc,&scale,reinterpret_cast<FReal* const>(tmpFX),
//                  reinterpret_cast<FReal* const>(FX));

  }


  /**
	 * Transform densities \f$Y= DFT(y)\f$ of a source cell. This operation
	 * can be seen as part of the M2M operation.
	 *
	 * @param[in] y multipole expansion of size \f$\ell^3\f$
	 * @param[out] Y transformed multipole expansion of size \f$r\f$
	 */
  void applyZeroPaddingAndDFT(FReal *const y, FComplexe *const FY) const
  {
    FReal Py[rc];
    FBlas::setzero(rc,Py);

    // Apply Zero Padding
    for (unsigned int i=0; i<nnodes; ++i)
      Py[node_diff[i*nnodes]]=y[i];

    // Apply forward Discrete Fourier Transform
    Dft.applyDFT(Py,FY);

  }


};


#endif // FUNIFM2LHANDLER_HPP

// [--END--]
