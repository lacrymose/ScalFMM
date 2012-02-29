#ifndef FCHEBLEAF_HPP
#define FCHEBLEAF_HPP


#include "../Utils/FNoCopyable.hpp"
#include "../Containers/FVector.hpp"

class FChebParticle;

/**
 * @author Matthias Messner (matthias.messner@inria.fr)
 *
 * @class FChebLeaf
 *
 * @brief Please read the license
 *
 * This class is used as a leaf in the Chebyshev FMM approach.
 */
template<class ParticleClass, class ContainerClass>
class FChebLeaf : FNoCopyable
{
private:
	ContainerClass particles; //!< Stores all particles contained by this leaf
	
public:
	~FChebLeaf() {}
	
	/**
	 * @param particle The new particle to be added to the leaf
	 */
        void push(const ParticleClass& inParticle)
	{
		particles.push(inParticle);
	}
	
	/**
	 * @return Array containing source particles
	 */
	ContainerClass* getSrc()
	{
		return &particles;
	}

	/**
	 * @return Array containing target particles
	 */
	ContainerClass* getTargets()
	{
		return &particles;
	}

};


#endif //FSIMPLELEAF_HPP


