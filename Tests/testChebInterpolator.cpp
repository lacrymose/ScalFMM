

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../Src/Utils/FTic.hpp"
#include "../Src/Utils/FMath.hpp"

#include "../Src/Containers/FVector.hpp"

#include "../Src/Utils/FAssertable.hpp"
#include "../Src/Utils/F3DPosition.hpp"

#include "../Src/Chebyshev/FChebParticle.hpp"
#include "../Src/Chebyshev/FChebLeaf.hpp"
#include "../Src/Chebyshev/FChebInterpolator.hpp"
#include "../Src/Chebyshev/FChebMatrixKernel.hpp"




const FReal computeL2norm(unsigned int N, FReal *const u, FReal *const v)
{
	FReal      dot = FReal(0.);
	FReal diff_dot = FReal(0.);
	for (unsigned int i=0; i<N; ++i) {
		FReal w = v[i] - u[i];
		diff_dot += w    * w;
		dot      += u[i] * u[i];
	}
	return FMath::Sqrt(diff_dot / dot);
}



const FReal computeINFnorm(unsigned int N, FReal *const u, FReal *const v)
{
	FReal      max = FReal(0.);
	FReal diff_max = FReal(0.);
	for (unsigned int n=0; n<N; ++n) {
		if (     max<std::abs(u[n]))           max = std::abs(u[n]);
		if (diff_max<std::abs(u[n]-v[n])) diff_max = std::abs(u[n]-v[n]);
	}
	return diff_max / max;
}



/**
* In this file we show how to use octree
*/

int main(int, char **){    

	typedef FChebParticle ParticleClass;
	typedef FVector<FChebParticle> ContainerClass;
	typedef FChebLeaf<ParticleClass,ContainerClass> LeafClass;
	typedef FChebMatrixKernelR MatrixKernelClass;


	///////////////////////What we do/////////////////////////////
	std::cout << "\nTask: Compute interactions between source particles in leaf Y and target\n";
	std::cout << " particles in leaf X. Compare the fast summation K ~ Sx K Sy' with the\n";
	std::cout << " direct computation.\n" << std::endl;
	//////////////////////////////////////////////////////////////

	MatrixKernelClass MatrixKernel;
	const FReal FRandMax = FReal(RAND_MAX);
	FTic time;

	
	// Leaf size
	FReal width = 1.;

	////////////////////////////////////////////////////////////////////
	LeafClass X;
	F3DPosition cx(0., 0., 0.);
	const long M = 10000;
	std::cout << "Fill the leaf X of width " << width
						<< " centered at cx=[" << cx.getX() << "," << cx.getY() << "," << cx.getZ()
						<< "] with M=" << M << " target particles" << std::endl;
	{
		FChebParticle particle;
		for(long i=0; i<M; ++i){
			FReal x = (FReal(rand())/FRandMax - FReal(.5)) * width + cx.getX();
			FReal y = (FReal(rand())/FRandMax - FReal(.5)) * width + cx.getY();
			FReal z = (FReal(rand())/FRandMax - FReal(.5)) * width + cx.getZ();
			particle.setPosition(x, y, z);
			X.push(particle);
		}
	}


	////////////////////////////////////////////////////////////////////
	LeafClass Y;
	F3DPosition cy(2., 0., 0.);
	const long N = 10000;
	std::cout << "Fill the leaf Y of width" << width
						<< " centered at cy=[" << cy.getX() << "," << cy.getY() << "," << cy.getZ()
						<< "] with N=" << N << " target particles" << std::endl;
	{
		FChebParticle particle;
		for(long i=0; i<N; ++i){
			FReal x = (FReal(rand())/FRandMax - FReal(.5)) * width + cy.getX();
			FReal y = (FReal(rand())/FRandMax - FReal(.5)) * width + cy.getY();
			FReal z = (FReal(rand())/FRandMax - FReal(.5)) * width + cy.getZ();
			particle.setPosition(x, y, z);
			particle.setPhysicalValue(FReal(rand())/FRandMax);
			Y.push(particle);
		}
	}



	////////////////////////////////////////////////////////////////////
	// approximative computation
	const unsigned int ORDER = 5;
	const unsigned int nnodes = TensorTraits<ORDER>::nnodes;
	typedef FChebInterpolator<ORDER> InterpolatorClass;
	InterpolatorClass S;

	std::cout << "\nCompute interactions approximatively, interpolation order = " << ORDER << " ..." << std::endl;
	time.tic();

	// Anterpolate: W_n = \sum_j^N S(y_j,\bar y_n) * w_j
	FReal W[nnodes]; // multipole expansion
	S.applyP2M(cy, width, W, Y.getSrc()); // the multipole expansions are set to 0 in S.applyP2M

	// Multipole to local: F_m = \sum_n^L K(\bar x_m, \bar y_n) * W_n
	F3DPosition rootsX[nnodes], rootsY[nnodes];
	FChebTensor<ORDER>::setRoots(cx, width, rootsX);
	FChebTensor<ORDER>::setRoots(cy, width, rootsY);

	FReal F[nnodes]; // local expansion
	for (unsigned int i=0; i<nnodes; ++i) {
		F[i] = FReal(0.);
		for (unsigned int j=0; j<nnodes; ++j)
			F[i] += MatrixKernel.evaluate(rootsX[i], rootsY[j]) * W[j];
	}

	// Interpolate f_i = \sum_m^L S(x_i,\bar x_m) * F_m
	S.applyL2P(cx, width, F, X.getTargets());

	time.tac();
	std::cout << "Done in " << time.elapsed() << "sec." << std::endl;
	// -----------------------------------------------------

	////////////////////////////////////////////////////////////////////
	// direct computation
	std::cout << "Compute interactions directly ..." << std::endl;
	time.tic();

	FReal* approx_f = new FReal[M];
	FReal*        f = new FReal[M];
	for (unsigned int i=0; i<M; ++i) f[i] = FReal(0.);
	ContainerClass::ConstBasicIterator iterY(*(Y.getSrc()));
	while(iterY.hasNotFinished()){
		const F3DPosition& y = iterY.data().getPosition();
		const FReal        w = iterY.data().getPhysicalValue();
		unsigned int counter = 0;
		ContainerClass::ConstBasicIterator iterX(*(X.getSrc()));
		while(iterX.hasNotFinished()){
			const F3DPosition& x = iterX.data().getPosition();
			f[counter++] += MatrixKernel.evaluate(x,y) * w;
			iterX.gotoNext();
		}
		iterY.gotoNext();
	}
	time.tac();
	std::cout << "Done in " << time.elapsed() << "sec." << std::endl;


	////////////////////////////////////////////////////////////////////
	ContainerClass::ConstBasicIterator iterX(*(X.getSrc()));
	unsigned int counter = 0;
	while(iterX.hasNotFinished()) {
		approx_f[counter++] = iterX.data().getPotential();
		iterX.gotoNext();
	}

	std::cout << "\nRelative L2 error  = " << computeL2norm( M, f, approx_f) << std::endl;
	std::cout << "Relative Lmax error = "  << computeINFnorm(M, f, approx_f) << "\n" << std::endl;

	// free memory
	delete [] approx_f;
	delete [] f;


	return 0;
}



