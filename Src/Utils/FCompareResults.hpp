// ===================================================================================
// Copyright ScalFmm 2011 INRIA, Olivier Coulaud, Brenger Bramas
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
#ifndef FCOMPARERESULTS_HPP
#define FCOMPARERESULTS_HPP

#include <cstdlib>
#include <string>
//
#include "Utils/FMath.hpp"
#include "Files/FFmaGenericLoader.hpp"

template <class classArrayType>
int compareTwoArrays(const std::string &tag,  const int &nbParticles, const classArrayType &array1,
		const classArrayType &array2){
	//
	FMath::FAccurater potentialDiff;
	FMath::FAccurater fx, fy, fz;
	double energy1= 0.0, energy2= 0.0;
	int error = 0 ;
	for(int idxPart = 0 ; idxPart < nbParticles ;++idxPart){
		if(array1[idxPart].getPosition() != array2[idxPart].getPosition() ){
			std::cerr <<"Wrong positions  " <<std::endl
					<< "   P1: " <<array1[idxPart].getPosition()<<std::endl
					<< "   P2: " <<array2[idxPart].getPosition()<<std::endl
			<< "   error  " <<array1[idxPart].getPosition()-array2[idxPart].getPosition()<<std::endl;
		}
		potentialDiff.add(array1[idxPart].getPotential(),array2[idxPart].getPotential());
		fx.add(array1[idxPart].getForces()[0],array2[idxPart].getForces()[0]);
		fy.add(array1[idxPart].getForces()[1],array2[idxPart].getForces()[1]);
		fz.add(array1[idxPart].getForces()[2],array2[idxPart].getForces()[2]);
		energy1 += array1[idxPart].getPhysicalValue() *array1[idxPart].getPotential() ;
		energy2 += array2[idxPart].getPhysicalValue() *array2[idxPart].getPotential() ;
	}
	// Print for information
	std::cout << tag<< " Energy "  << FMath::Abs(energy1-energy2) << "  "
			<<  FMath::Abs(energy1-energy2) /energy1 <<std::endl;
	std::cout << tag<< " Potential " << potentialDiff << std::endl;
	std::cout << tag<< " Fx " << fx << std::endl;
	std::cout << tag<< " Fy " << fy << std::endl;
	std::cout << tag<< " Fz " << fz << std::endl;

	return error ;
}
//
template <class classArrayType>
void computeFirstMoment( const int &nbParticles, const classArrayType &particles,  FPoint &FirstMoment){

	double mx,my,mz ;
	//
#pragma omp parallel  for shared(nbParticles,particles) reduction(+:mx,my,mz)
	for(int idxPart = 0 ; idxPart < nbParticles ; ++idxPart){
		//
		mx += particles[idxPart].getPhysicalValue()*particles[idxPart].getPosition().getX() ;
		my += particles[idxPart].getPhysicalValue()*particles[idxPart].getPosition().getY() ;
		mz += particles[idxPart].getPhysicalValue()*particles[idxPart].getPosition().getZ() ;
	}
	FirstMoment.setPosition(mx,my,mz);
} ;

template <class classArrayType>
void removeFirstMoment( const std::string& TYPE, const int &nbParticles, const classArrayType &particles,  FReal &volume) {
	FPoint FirstMoment ;
	computeFirstMoment( nbParticles, particles,  FirstMoment);
	std::cout << std::endl;
	std::cout << "Electric Moment          = "<< FirstMoment <<std::endl;
	std::cout << "Electric Moment norm = "<< FirstMoment.norm2()  <<std::endl;
	std::cout << "----------------------------------------------------"<<std::endl;
	std::cout << std::endl;
	//
	// Remove
	FReal coeffCorrection  = 4.0*FMath::FPi/volume/3.0 ;
	FReal scaleEnergy=1.0, scaleForce=1.0  ;
	//
	if (TYPE == "DLPOLY") {
		const FReal r4pie0 = FReal(138935.4835);
		scaleEnergy =  r4pie0 / 418.4 ;   // kcal mol^{-1}
		scaleForce  = -r4pie0 ;           // 10 J mol^{-1} A^{-1}
	}
	else if (TYPE == "NOSCALE") {
		scaleEnergy=1.0, scaleForce=1.0  ;
	}
	else {
		std::cerr << "In removeFirstMoment TYPE " << TYPE << " was not know. Available TYPE DLPOLY "<< std::endl;
		std::exit( EXIT_FAILURE);
	}
      //
	double tmp;
	for(int idx = 0 ; idx < nbParticles ; ++idx){
		tmp = particles[idx].getPosition().getX()*FirstMoment.getX()  + particles[idx].getPosition().getY()*FirstMoment.getY()
								+ particles[idx].getPosition().getZ()*FirstMoment.getZ()  ;
		FReal Q =  particles[idx].getPhysicalValue(), P = particles[idx].getPotential();
		//

		particles[idx].setPotential( P - tmp*coeffCorrection);
		//
		std::cout << "idx: "<< idx <<" old: " << particles[idx].getForces()[0] ;
		particles[idx].getForces()[0] -= Q*coeffCorrection*FirstMoment.getX() ;
		particles[idx].getForces()[1] -= Q*coeffCorrection*FirstMoment.getY() ;
		particles[idx].getForces()[2] -= Q*coeffCorrection*FirstMoment.getZ() ;
		std::cout << " new: " << particles[idx].getForces()[0] <<std::endl;

		//
		particles[idx].getForces()[0] *= scaleForce;
		particles[idx].getForces()[1] *= scaleForce;
		particles[idx].getForces()[2] *= scaleForce;
		//
		//		newEnergy += Q*particles[idx].getPotential()  ;
	}

} ;
/*
template <class classArrayType, class classTree >
int compareTwoArrays(const int &nbParticles,const std::string &tag, const classArrayType &array1,
			const classTree &tree){
	//
	FMath::FAccurater potentialDiff;
	FMath::FAccurater fx, fy, fz;
	double energy1, energy2;
	int error = 0 ;

	for(int idxPart = 0 ; idxPart < nbParticles ;++idxPart){
		energy1 += array1[idxPart].getPhysicalValue() *array1[idxPart].getPotential() ;
	}
	//
	tree.forEachLeaf([&](LeafClass* leaf){
			const FReal*const physicalValues = leaf->getTargets()->getPhysicalValues();
			const FReal*const potentials        = leaf->getTargets()->getPotentials();
			const FReal*const forcesX            = leaf->getTargets()->getForcesX();
			const FReal*const forcesY            = leaf->getTargets()->getForcesY();
			const FReal*const forcesZ            = leaf->getTargets()->getForcesZ();
			const int nbParticlesInLeaf           = leaf->getTargets()->getNbParticles();
			const FVector<int>& indexes       = leaf->getTargets()->getIndexes();

			for(int idxPart = 0 ; idxPart < nbParticlesInLeaf ; ++idxPart){
				const int indexPartOrig = indexes[idxPart];
			//	if(array1[indexPartOrig].getPosition() != array2[idxPart].getPosition() )
			 {
				potentialDiff.add(particles[indexPartOrig].getPotential(),potentials[idxPart]);
				fx.add(particles[indexPartOrig].getForces()[0],forcesX[idxPart]);
				fy.add(particles[indexPartOrig].getForces()[1],forcesY[idxPart]);
				fz.add(particles[indexPartOrig].getForces()[2],forcesZ[idxPart]);
				energy2   += potentials[idxPart]*physicalValues[idxPart] ;
			}
		}
	});


	// Print for information
	std::cout << tag<< " Energy "  << FMath::Abs(energy1-energy2) << "  "
			<<  FMath::Abs(energy1-energy2) /energy1 <<std::endl;
	std::cout << tag<< " Potential " << potentialDiff << std::endl;
	std::cout << tag<< " Fx " << fx << std::endl;
	std::cout << tag<< " Fy " << fy << std::endl;
	std::cout << tag<< " Fz " << fz << std::endl;
	return error ;

}
 */
#endif
