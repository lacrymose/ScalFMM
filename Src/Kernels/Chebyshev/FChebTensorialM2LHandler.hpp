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
#ifndef FCHEBTENSORIALM2LHANDLER_HPP
#define FCHEBTENSORIALM2LHANDLER_HPP

#include <numeric>
#include <stdexcept>
#include <string>
#include <sstream>
#include <fstream>
#include <typeinfo>

#include "../../Utils/FBlas.hpp"
#include "../../Utils/FTic.hpp"

#include "./FChebTensor.hpp"

/**
 * Computes and compresses all \f$K_t\f$.
 *
 * @param[in] epsilon accuracy
 * @param[out] U matrix of size \f$\ell^3\times r\f$
 * @param[out] C matrix of size \f$r\times 316 r\f$ storing \f$[C_1,\dots,C_{316}]\f$
 * @param[out] B matrix of size \f$\ell^3\times r\f$
 */
template <int ORDER, class MatrixKernelClass>
unsigned int ComputeAndCompress(const MatrixKernelClass *const MatrixKernel, 
                                const FReal CellWidth, 
                                const FReal epsilon,
                                FReal* &U,
                                FReal** &C,
                                FReal* &B);

//template <int ORDER>
//unsigned int Compress(const FReal epsilon, const unsigned int ninteractions,
//											FReal* &U,	FReal* &C, FReal* &B);


/**
 * @author Matthias Messner (matthias.messner@inria.fr)
 * @class FChebM2LHandler
 * Please read the license
 *
 * This class precomputes and compresses the M2L operators
 * \f$[K_1,\dots,K_{316}]\f$ for all (\f$7^3-3^3 = 316\f$ possible interacting
 * cells in the far-field) interactions for the Chebyshev interpolation
 * approach. The class uses the compression via a truncated SVD and represents
 * the compressed M2L operator as \f$K_t \sim U C_t B^\top\f$ with
 * \f$t=1,\dots,316\f$. The truncation rank is denoted by \f$r\f$ and is
 * determined by the prescribed accuracy \f$\varepsilon\f$. Hence, the
 * originally \f$K_t\f$ of size \f$\ell^3\times\ell^3\f$ times \f$316\f$ for
 * all interactions is reduced to only one \f$U\f$ and one \f$B\f$, each of
 * size \f$\ell^3\times r\f$, and \f$316\f$ \f$C_t\f$, each of size \f$r\times
 * r\f$.
 *
 * PB: FChebM2LHandler does not seem to support non_homogeneous kernels!
 * In fact nothing appears to handle this here (i.e. adapt scaling and storage 
 * to MatrixKernelClass::Type). Given the relatively important cost of the 
 * Chebyshev variant, it is probably a choice not to have implemented this 
 * feature here but instead in the ChebyshevSym variant. But what if the 
 * kernel is non homogeneous and non symmetric (e.g. Dislocations)... 
 * 
 * TODO Specialize class (see UnifM2LHandler) OR prevent from using this 
 * class with non homogeneous kernels ?!
 *
 * @tparam ORDER interpolation order \f$\ell\f$
 */
template <int ORDER, class MatrixKernelClass, KERNEL_FUNCTION_TYPE TYPE> class FChebTensorialM2LHandler;

template <int ORDER, class MatrixKernelClass>
class FChebTensorialM2LHandler<ORDER,MatrixKernelClass,HOMOGENEOUS> : FNoCopyable
{
	enum {order = ORDER,
				nnodes = TensorTraits<ORDER>::nnodes,
				ninteractions = 316,// 7^3 - 3^3 (max num cells in far-field)
        ncmp = MatrixKernelClass::NCMP};

//	const MatrixKernelClass MatrixKernel;

//	FReal *U, *C, *B;
	FReal *U, *B;
	FReal** C;

	const FReal epsilon; //<! accuracy which determines trucation of SVD
	unsigned int rank;   //<! truncation rank, satisfies @p epsilon


	static const std::string getFileName(FReal epsilon)
	{
		const char precision_type = (typeid(FReal)==typeid(double) ? 'd' : 'f');
		std::stringstream stream;
		stream << "m2l_k"<< MatrixKernelClass::Identifier << "_" << precision_type
					 << "_o" << order << "_e" << epsilon << ".bin";
		return stream.str();
	}

	
public:
	FChebTensorialM2LHandler(const MatrixKernelClass *const MatrixKernel, const unsigned int, const FReal, const FReal _epsilon)
		: U(NULL), B(NULL), epsilon(_epsilon), rank(0)
	{
		// measure time
		FTic time; time.tic();
		// check if aready set
		if (U||B) throw std::runtime_error("U or B operator already set");

    // allocate C
    C = new FReal*[ncmp];
    for (unsigned int d=0; d<ncmp; ++d)
      C[d]=NULL;

    for (unsigned int d=0; d<ncmp; ++d)
      if (C[d]) throw std::runtime_error("Compressed M2L operator already set");

    // Compute matrix of interactions
    const FReal ReferenceCellWidth = FReal(2.);
		rank = ComputeAndCompress<order>(MatrixKernel, ReferenceCellWidth, epsilon, U, C, B);

    unsigned long sizeM2L = 343*ncmp*rank*rank*sizeof(FReal);


		// write info
		std::cout << "Compressed and set M2L operators (" << long(sizeM2L) << " B) in "
							<< time.tacAndElapsed() << "sec."	<< std::endl;
}

	~FChebTensorialM2LHandler()
	{
		if (U != NULL) delete [] U;
		if (B != NULL) delete [] B;
    for (unsigned int d=0; d<ncmp; ++d)
      if (C[d] != NULL) delete [] C[d];
	}

	/**
	 * @return rank of the SVD compressed M2L operators
	 */
	unsigned int getRank() const {return rank;}

  /**
	 * Expands potentials \f$x+=UX\f$ of a target cell. This operation can be
	 * seen as part of the L2L operation.
	 *
	 * @param[in] X compressed local expansion of size \f$r\f$
	 * @param[out] x local expansion of size \f$\ell^3\f$
	 */
  void applyU(const FReal *const X, FReal *const x) const
  {
//    FBlas::gemva(nnodes, rank, 1., U, const_cast<FReal*>(X), x);
    FBlas::add(nnodes, const_cast<FReal*>(X), x);
  }

  /**
	 * Compressed M2L operation \f$X+=C_tY\f$, where \f$Y\f$ is the compressed
	 * multipole expansion and \f$X\f$ is the compressed local expansion, both
	 * of size \f$r\f$. The index \f$t\f$ denotes the transfer vector of the
	 * target cell to the source cell.
	 *
	 * @param[in] transfer transfer vector
	 * @param[in] Y compressed multipole expansion
	 * @param[out] X compressed local expansion
	 * @param[in] CellWidth needed for the scaling of the compressed M2L operators which are based on a homogeneous matrix kernel computed for the reference cell width \f$w=2\f$, ie in \f$[-1,1]^3\f$.
	 */
  void applyC(const unsigned int idx, const unsigned int , 
               const FReal scale, const unsigned int d,
							const FReal *const Y, FReal *const X) const
  {
    FBlas::gemva(rank, rank, scale, C[d] + idx*rank*rank, const_cast<FReal*>(Y), X);
  }

  /**
	 * Compresses densities \f$Y=B^\top y\f$ of a source cell. This operation
	 * can be seen as part of the M2M operation.
	 *
	 * @param[in] y multipole expansion of size \f$\ell^3\f$
	 * @param[out] Y compressed multipole expansion of size \f$r\f$
	 */
  void applyB(FReal *const y, FReal *const Y) const
  {
//    FBlas::gemtv(nnodes, rank, 1., B, y, Y);
    FBlas::copy(nnodes, y, Y);
  }


};


template <int ORDER, class MatrixKernelClass>
class FChebTensorialM2LHandler<ORDER,MatrixKernelClass,NON_HOMOGENEOUS> : FNoCopyable
{
	enum {order = ORDER,
				nnodes = TensorTraits<ORDER>::nnodes,
				ninteractions = 316,// 7^3 - 3^3 (max num cells in far-field)
        ncmp = MatrixKernelClass::NCMP};

  // Tensorial MatrixKernel and homogeneity specific
	FReal **U, **B;
	FReal*** C;

  const unsigned int TreeHeight;
  const FReal RootCellWidth;

	const FReal epsilon; //<! accuracy which determines trucation of SVD
	unsigned int *rank;   //<! truncation rank, satisfies @p epsilon


	static const std::string getFileName(FReal epsilon)
	{
		const char precision_type = (typeid(FReal)==typeid(double) ? 'd' : 'f');
		std::stringstream stream;
		stream << "m2l_k"<< MatrixKernelClass::Identifier << "_" << precision_type
					 << "_o" << order << "_e" << epsilon << ".bin";
		return stream.str();
	}

	
public:
	FChebTensorialM2LHandler(const MatrixKernelClass *const MatrixKernel, const unsigned int inTreeHeight, const FReal inRootCellWidth, const FReal _epsilon)
		: TreeHeight(inTreeHeight),
      RootCellWidth(inRootCellWidth),
      epsilon(_epsilon)
	{
		// measure time
		FTic time; time.tic();

    // allocate rank
    rank = new unsigned int[TreeHeight];

    // allocate U and B
    U = new FReal*[TreeHeight]; 
    B = new FReal*[TreeHeight]; 
		for (unsigned int l=0; l<TreeHeight; ++l){
       B[l]=NULL; U[l]=NULL;
    }

    // allocate C
    C = new FReal**[TreeHeight];
		for (unsigned int l=0; l<TreeHeight; ++l){ 
      C[l] = new FReal*[ncmp];
      for (unsigned int d=0; d<ncmp; ++d)
        C[l][d]=NULL;
    }

    for (unsigned int l=0; l<TreeHeight; ++l) {
      if (U[l] || B[l]) throw std::runtime_error("Operator U or B already set");
      for (unsigned int d=0; d<ncmp; ++d)
        if (C[l][d]) throw std::runtime_error("Compressed M2L operator already set");
    }

    // Compute matrix of interactions at each level !! (since non homog)
		FReal CellWidth = RootCellWidth / FReal(2.); // at level 1
		CellWidth /= FReal(2.);                      // at level 2
    rank[0]=rank[1]=0;
		for (unsigned int l=2; l<TreeHeight; ++l) {
      rank[l] = ComputeAndCompress<order>(MatrixKernel, CellWidth, epsilon, U[l], C[l], B[l]);
			CellWidth /= FReal(2.);                    // at level l+1 
    }
    unsigned long sizeM2L = (TreeHeight-2)*343*ncmp*rank[2]*rank[2]*sizeof(FReal);

		// write info
    std::cout << "Compute and Set M2L operators of " << TreeHeight-2 << " levels ("<< long(sizeM2L/**1e-6*/) <<" Bytes) in "
							<< time.tacAndElapsed() << "sec."	<< std::endl;
}

	~FChebTensorialM2LHandler()
	{
    if (rank != NULL) delete [] rank;
    for (unsigned int l=0; l<TreeHeight; ++l) {
      if (U[l] != NULL) delete [] U[l];
      if (B[l] != NULL) delete [] B[l];
      for (unsigned int d=0; d<ncmp; ++d)
        if (C[l][d] != NULL) delete [] C[l][d];
    }
	}

	/**
	 * @return rank of the SVD compressed M2L operators
	 */
	unsigned int getRank(unsigned int l = 2) const {return rank[l];}

  /**
	 * Expands potentials \f$x+=UX\f$ of a target cell. This operation can be
	 * seen as part of the L2L operation.
	 *
	 * @param[in] X compressed local expansion of size \f$r\f$
	 * @param[out] x local expansion of size \f$\ell^3\f$
	 */
  void applyU(const FReal *const X, FReal *const x) const
  {
//    FBlas::gemva(nnodes, rank, 1., U, const_cast<FReal*>(X), x);
    FBlas::add(nnodes, const_cast<FReal*>(X), x);
  }

  /**
	 * Compressed M2L operation \f$X+=C_tY\f$, where \f$Y\f$ is the compressed
	 * multipole expansion and \f$X\f$ is the compressed local expansion, both
	 * of size \f$r\f$. The index \f$t\f$ denotes the transfer vector of the
	 * target cell to the source cell.
	 *
	 * @param[in] transfer transfer vector
	 * @param[in] Y compressed multipole expansion
	 * @param[out] X compressed local expansion
	 * @param[in] CellWidth needed for the scaling of the compressed M2L operators which are based on a homogeneous matrix kernel computed for the reference cell width \f$w=2\f$, ie in \f$[-1,1]^3\f$.
	 */
  void applyC(const unsigned int idx, const unsigned int l, 
               const FReal, const unsigned int d,
							const FReal *const Y, FReal *const X) const
  {
    FBlas::gemva(rank[l], rank[l], 1., C[l][d] + idx*rank[l]*rank[l], const_cast<FReal*>(Y), X);
  }

  /**
	 * Compresses densities \f$Y=B^\top y\f$ of a source cell. This operation
	 * can be seen as part of the M2M operation.
	 *
	 * @param[in] y multipole expansion of size \f$\ell^3\f$
	 * @param[out] Y compressed multipole expansion of size \f$r\f$
	 */
  void applyB(FReal *const y, FReal *const Y) const
  {
//    FBlas::gemtv(nnodes, rank, 1., B, y, Y);
    FBlas::copy(nnodes, y, Y);
  }


};





//////////////////////////////////////////////////////////////////////
// definition ////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////






template <int ORDER, class MatrixKernelClass>
unsigned int ComputeAndCompress(const MatrixKernelClass *const MatrixKernel, 
                                const FReal CellWidth, 
                                const FReal epsilon,
                                FReal* &U,
                                FReal** &C,
                                FReal* &B)
{
  // PB: need to redefine some constant since not function from m2lhandler class
  const unsigned int order = ORDER;
  const unsigned int nnodes = TensorTraits<ORDER>::nnodes;
  const unsigned int ninteractions = 316;
  const unsigned int ncmp = MatrixKernelClass::NCMP;

	// allocate memory and store compressed M2L operators
  for (unsigned int d=0; d<ncmp; ++d) 
    if (C[d]) throw std::runtime_error("Compressed M2L operators are already set");

	// interpolation points of source (Y) and target (X) cell
	FPoint X[nnodes], Y[nnodes];
	// set roots of target cell (X)
	FChebTensor<order>::setRoots(FPoint(0.,0.,0.), CellWidth, X);

	// allocate memory and compute 316 m2l operators
  FReal** _C; 
  _C  = new FReal* [ncmp];
  for (unsigned int d=0; d<ncmp; ++d) 
    _C[d] = new FReal [nnodes*nnodes * ninteractions];

	unsigned int counter = 0;
	for (int i=-3; i<=3; ++i) {
		for (int j=-3; j<=3; ++j) {
			for (int k=-3; k<=3; ++k) {
				if (abs(i)>1 || abs(j)>1 || abs(k)>1) {
					// set roots of source cell (Y)
					const FPoint cy(CellWidth*FReal(i), CellWidth*FReal(j), CellWidth*FReal(k));
					FChebTensor<order>::setRoots(cy, CellWidth, Y);
					// evaluate m2l operator
					for (unsigned int n=0; n<nnodes; ++n)
						for (unsigned int m=0; m<nnodes; ++m){
//							_C[counter*nnodes*nnodes + n*nnodes + m]
//								= MatrixKernel.evaluate(X[m], Y[n]);
              // Compute current M2L interaction (block matrix)

              FReal* block; 
              block = new FReal[ncmp]; 
              MatrixKernel->evaluateBlock(X[m], Y[n], block);

              // Copy block in C
              for (unsigned int d=0; d<ncmp; ++d) 
                _C[d][counter*nnodes*nnodes + n*nnodes + m] = block[d];

              delete [] block;
              
            }
					// increment interaction counter
					counter++;
				}
			}
		}
	}
	if (counter != ninteractions)
		throw std::runtime_error("Number of interactions must correspond to 316");


//	//////////////////////////////////////////////////////////		
//	FReal weights[nnodes];
//	FChebTensor<order>::setRootOfWeights(weights);
//	for (unsigned int i=0; i<316; ++i)
//		for (unsigned int n=0; n<nnodes; ++n) {
//			FBlas::scal(nnodes, weights[n], _C+i*nnodes*nnodes + n,  nnodes); // scale rows
//			FBlas::scal(nnodes, weights[n], _C+i*nnodes*nnodes + n * nnodes); // scale cols
//		}
//	//////////////////////////////////////////////////////////		

	// svd compression of M2L
//	const unsigned int rank	= Compress<ORDER>(epsilon, ninteractions, _U, _C, _B);
  const unsigned int rank	= nnodes; //PB: dense Chebyshev
	if (!(rank>0)) throw std::runtime_error("Low rank must be larger then 0!");

	// store U
	U = new FReal [nnodes * rank];
//	FBlas::copy(rank*nnodes, _U, U);
	FBlas::setzero(rank*nnodes, U);
//	delete [] _U;
	// store B
	B = new FReal [nnodes * rank];
//	FBlas::copy(rank*nnodes, _B, B);
	FBlas::setzero(rank*nnodes, B);
//	delete [] _B;

	// store C
	counter = 0;
  for (unsigned int d=0; d<ncmp; ++d) 
    C[d] = new FReal [343 * rank*rank];
	for (int i=-3; i<=3; ++i)
		for (int j=-3; j<=3; ++j)
			for (int k=-3; k<=3; ++k) {
				const unsigned int idx = (i+3)*7*7 + (j+3)*7 + (k+3);
				if (abs(i)>1 || abs(j)>1 || abs(k)>1) {
          for (unsigned int d=0; d<ncmp; ++d) 
            FBlas::copy(rank*rank, _C[d] + counter*rank*rank, C[d] + idx*rank*rank);
					counter++;
				} else {
          for (unsigned int d=0; d<ncmp; ++d) 
            FBlas::setzero(rank*rank, C[d] + idx*rank*rank);
        }
			}
	if (counter != ninteractions)
		throw std::runtime_error("Number of interactions must correspond to 316");
  for (unsigned int d=0; d<ncmp; ++d) 
    delete [] _C[d];


//	//////////////////////////////////////////////////////////		
//	for (unsigned int n=0; n<nnodes; ++n) {
//		FBlas::scal(rank, FReal(1.) / weights[n], U+n, nnodes); // scale rows
//		FBlas::scal(rank, FReal(1.) / weights[n], B+n, nnodes); // scale rows
//	}
//	//////////////////////////////////////////////////////////		

	// return low rank
	return rank;
}






//template <int ORDER, class MatrixKernelClass>
//void
//FChebM2LHandler<ORDER, MatrixKernelClass>::ComputeAndCompressAndStoreInBinaryFile(const FReal epsilon)
//{
//	// measure time
//	FTic time; time.tic();
//	// start computing process
//	FReal *U, *C, *B;
//	U = C = B = NULL;
//	const unsigned int rank = ComputeAndCompress(epsilon, U, C, B);
//	// store into binary file
//	const std::string filename(getFileName(epsilon));
//	std::ofstream stream(filename.c_str(),
//											 std::ios::out | std::ios::binary | std::ios::trunc);
//	if (stream.good()) {
//		stream.seekp(0);
//		// 1) write number of interpolation points (int)
//		int _nnodes = nnodes;
//		stream.write(reinterpret_cast<char*>(&_nnodes), sizeof(int));
//		// 2) write low rank (int)
//		int _rank = rank;
//		stream.write(reinterpret_cast<char*>(&_rank), sizeof(int));
//		// 3) write U (rank*nnodes * FReal)
//		stream.write(reinterpret_cast<char*>(U), sizeof(FReal)*rank*nnodes);
//		// 4) write B (rank*nnodes * FReal)
//		stream.write(reinterpret_cast<char*>(B), sizeof(FReal)*rank*nnodes);
//		// 5) write 343 C (343 * rank*rank * FReal)
//		stream.write(reinterpret_cast<char*>(C), sizeof(FReal)*rank*rank*343);
//	} 	else throw std::runtime_error("File could not be opened to write");
//	stream.close();
//	// free memory
//	if (U != NULL) delete [] U;
//	if (B != NULL) delete [] B;
//	if (C != NULL) delete [] C;
//	// write info
//	std::cout << "Compressed M2L operators ("<< rank << ") stored in binary file "	<< filename
//						<< " in " << time.tacAndElapsed() << "sec."	<< std::endl;
//}
//
//
//template <int ORDER, class MatrixKernelClass>
//void
//FChebM2LHandler<ORDER, MatrixKernelClass>::ReadFromBinaryFileAndSet()
//{
//	// measure time
//	FTic time; time.tic();
//	// start reading process
//	if (U||C||B) throw std::runtime_error("Compressed M2L operator already set");
//	const std::string filename(getFileName(epsilon));
//	std::ifstream stream(filename.c_str(),
//											 std::ios::in | std::ios::binary | std::ios::ate);
//	const std::ifstream::pos_type size = stream.tellg();
//	if (size<=0) {
//		std::cout << "Info: The requested binary file " << filename
//							<< " does not yet exist. Compute it now ... " << std::endl;
//		this->ComputeAndCompressAndStoreInBinaryFileAndReadFromFileAndSet();
//		return;
//	} 
//	if (stream.good()) {
//		stream.seekg(0);
//		// 1) read number of interpolation points (int)
//		int npts;
//		stream.read(reinterpret_cast<char*>(&npts), sizeof(int));
//		if (npts!=nnodes) throw std::runtime_error("nnodes and npts do not correspond");
//		// 2) read low rank (int)
//		stream.read(reinterpret_cast<char*>(&rank), sizeof(int));
//		// 3) write U (rank*nnodes * FReal)
//		U = new FReal [rank*nnodes];
//		stream.read(reinterpret_cast<char*>(U), sizeof(FReal)*rank*nnodes);
//		// 4) write B (rank*nnodes * FReal)
//		B = new FReal [rank*nnodes];
//		stream.read(reinterpret_cast<char*>(B), sizeof(FReal)*rank*nnodes);
//		// 5) write 343 C (343 * rank*rank * FReal)
//		C = new FReal [343 * rank*rank];
//		stream.read(reinterpret_cast<char*>(C), sizeof(FReal)*rank*rank*343);
//	}	else throw std::runtime_error("File could not be opened to read");
//	stream.close();
//	// write info
//	std::cout << "Compressed M2L operators (" << rank << ") read from binary file "
//						<< filename << " in " << time.tacAndElapsed() << "sec."	<< std::endl;
//}
//
///*
//unsigned int ReadRankFromBinaryFile(const std::string& filename)
//{
//	// start reading process
//	std::ifstream stream(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
//	const std::ifstream::pos_type size = stream.tellg();
//	if (size<=0) throw std::runtime_error("The requested binary file does not exist.");
//	unsigned int rank = -1;
//	if (stream.good()) {
//		stream.seekg(0);
//		// 1) read number of interpolation points (int)
//		int npts;
//		stream.read(reinterpret_cast<char*>(&npts), sizeof(int));
//		// 2) read low rank (int)
//		stream.read(reinterpret_cast<char*>(&rank), sizeof(int));
//		return rank;
//	}	else throw std::runtime_error("File could not be opened to read");
//	stream.close();
//	return rank;
//}
//*/


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

///**
// * Computes the low-rank \f$k\f$ based on \f$\|K-U\Sigma_kV^\top\|_F \le
// * \epsilon \|K\|_F\f$, ie., the truncation rank of the singular value
// * decomposition. With the definition of the Frobenius norm \f$\|K\|_F =
// * \left(\sum_{i=1}^N \sigma_i^2\right)^{\frac{1}{2}}\f$ the determination of
// * the low-rank follows as \f$\|K-U\Sigma_kV^\top\|_F^2 = \sum_{i=k+1}^N
// * \sigma_i^2 \le \epsilon^2 \sum_{i=1}^N \sigma_i^2 = \epsilon^2
// * \|K\|_F^2\f$.
// *
// * @param[in] singular_values array of singular values ordered as \f$\sigma_1
// * \ge \sigma_2 \ge \dots \ge \sigma_N\f$
// * @param[in] eps accuracy \f$\epsilon\f$
// */ 
//template <int ORDER>
//unsigned int getRank(const FReal singular_values[], const double eps)
//{
//	enum {nnodes = TensorTraits<ORDER>::nnodes};
//
//	FReal nrm2(0.);
//	for (unsigned int k=0; k<nnodes; ++k)
//		nrm2 += singular_values[k] * singular_values[k];
//
//	FReal nrm2k(0.);
//	for (unsigned int k=nnodes; k>0; --k) {
//		nrm2k += singular_values[k-1] * singular_values[k-1];
//		if (nrm2k > eps*eps * nrm2)	return k;
//	}
//	throw std::runtime_error("rank cannot be larger than nnodes");
//	return 0;
//}
//
//unsigned int getRank(const FReal singular_values[], const unsigned int size, const double eps)
//{
//	const FReal nrm2 = FBlas::scpr(size, singular_values, singular_values);
//	FReal nrm2k(0.);
//	for (unsigned int k=size; k>0; --k) {
//		nrm2k += singular_values[k-1] * singular_values[k-1];
//		if (nrm2k > eps*eps * nrm2)	return k;
//	}
//	throw std::runtime_error("rank cannot be larger than size");
//	return 0;
//}
//
//
///**
// * Compresses \f$[K_1,\dots,K_{316}]\f$ in \f$C\f$. Attention: the matrices
// * \f$U,B\f$ are not initialized, no memory is allocated as input, as output
// * they store the respective matrices. The matrix \f$C\f$ stores
// * \f$[K_1,\dots,K_{316}]\f$ as input and \f$[C_1,\dots,C_{316}]\f$ as output.
// *
// * @param[in] epsilon accuracy
// * @param[out] U matrix of size \f$\ell^3\times r\f$
// * @param[in] C matrix of size \f$\ell^3\times 316 \ell^e\f$ storing \f$[K_1,\dots,K_{316}]\f$
// * @param[out] C matrix of size \f$r\times 316 r\f$ storing \f$[C_1,\dots,C_{316}]\f$
// * @param[out] B matrix of size \f$\ell^3\times r\f$
// */
//template <int ORDER>
//unsigned int Compress(const FReal epsilon, const unsigned int ninteractions,
//											FReal* &U,	FReal* &C, FReal* &B)
//{
//	// compile time constants
//	enum {order = ORDER,
//				nnodes = TensorTraits<ORDER>::nnodes};
//
//	// init SVD
//	const unsigned int LWORK = 2 * (3*nnodes + ninteractions*nnodes);
//	FReal *const WORK = new FReal [LWORK];
//	
//	// K_col ///////////////////////////////////////////////////////////
//	FReal *const K_col = new FReal [ninteractions * nnodes*nnodes]; 
//	for (unsigned int i=0; i<ninteractions; ++i)
//		for (unsigned int j=0; j<nnodes; ++j)
//			FBlas::copy(nnodes,
//									C     + i*nnodes*nnodes + j*nnodes,
//									K_col + j*ninteractions*nnodes + i*nnodes);
//	// singular value decomposition
//	FReal *const Q = new FReal [nnodes*nnodes];
//	FReal *const S = new FReal [nnodes];
//	const unsigned int info_col
//		= FBlas::gesvd(ninteractions*nnodes, nnodes, K_col, S, Q, nnodes,
//									 LWORK, WORK);
//	if (info_col!=0)
//		throw std::runtime_error("SVD did not converge with " + info_col);
//	delete [] K_col;
//	const unsigned int k_col = getRank<ORDER>(S, epsilon);
//
//	// Q' -> B 
//	B = new FReal [nnodes*k_col];
//	for (unsigned int i=0; i<k_col; ++i)
//		FBlas::copy(nnodes, Q+i, nnodes, B+i*nnodes, 1);
//
//	// K_row //////////////////////////////////////////////////////////////
//	FReal *const K_row = C;
//
//	const unsigned int info_row
//		= FBlas::gesvdSO(nnodes, ninteractions*nnodes, K_row, S, Q, nnodes,
//										 LWORK, WORK);
//	if (info_row!=0)
//		throw std::runtime_error("SVD did not converge with " + info_row);
//	const unsigned int k_row = getRank<ORDER>(S, epsilon);
//	delete [] WORK;
//
//	// Q -> U
//	U = Q;
//
//	// V' -> V
//	FReal *const V = new FReal [nnodes*ninteractions * k_row];
//	for (unsigned int i=0; i<k_row; ++i)
//		FBlas::copy(nnodes*ninteractions, K_row+i, nnodes,
//								V+i*nnodes*ninteractions, 1);
//
//	// rank k(epsilon) /////////////////////////////////////////////////////
//	const unsigned int k = k_row < k_col ? k_row : k_col;
//
//	// C_row ///////////////////////////////////////////////////////////
//	C = new FReal [ninteractions * k*k];
//	for (unsigned int i=0; i<k; ++i) {
//		FBlas::scal(nnodes*ninteractions, S[i], V + i*nnodes*ninteractions);
//		for (unsigned int m=0; m<ninteractions; ++m)
//			for (unsigned int j=0; j<k; ++j)
//				C[m*k*k + j*k + i]
//					= FBlas::scpr(nnodes,
//												V + i*nnodes*ninteractions + m*nnodes,
//												B + j*nnodes);
//	}
//
//	delete [] V;
//	delete [] S;
//	delete [] K_row;
//
//	return k;
//}




#endif // FCHEBTENSORIALM2LHANDLER_HPP

// [--END--]
