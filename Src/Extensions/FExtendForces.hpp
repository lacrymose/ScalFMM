// ===================================================================================
// Logiciel initial: ScalFmm Version 0.5
// Co-auteurs : Olivier Coulaud, Bérenger Bramas.
// Propriétaires : INRIA.
// Copyright © 2011-2012, diffusé sous les termes et conditions d’une licence propriétaire.
// Initial software: ScalFmm Version 0.5
// Co-authors: Olivier Coulaud, Bérenger Bramas.
// Owners: INRIA.
// Copyright © 2011-2012, spread under the terms and conditions of a proprietary license.
// ===================================================================================
#ifndef FEXTENDFORCES_HPP
#define FEXTENDFORCES_HPP


#include "../Utils/FGlobal.hpp"
#include "../Utils/F3DPosition.hpp"

/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
* @class FExtendForces
* Please read the license
*
* This class is an extenssion.
* It proposes a 3d array as a forces vector.
*/
class FExtendForces {
protected:
    F3DPosition forces; //< 3D vector stored in a position object

public:
    /** Default constructor */
    FExtendForces() {
    }

    /** Copy constructor */
    FExtendForces(const FExtendForces& other) : forces(other.forces) {
    }

    /** Copy operator */
    FExtendForces& operator=(const FExtendForces& other) {
        this->forces = other.forces;
        return *this;
    }

    /** Return the forces */
    const F3DPosition& getForces() const {
        return this->forces;
    }

    /** Set Forces */
    void incForces(const F3DPosition& inForces) {
        this->forces += inForces;
    }

    /** Set Forces with 3 FReals */
    void incForces(const FReal inFx, const FReal inFy, const FReal inFz) {
        this->forces.incX(inFx);
        this->forces.incY(inFy);
        this->forces.incZ(inFz);
    }

    /** set the forces from 3 variables */
    void setForces(const FReal inFx, const FReal inFy, const FReal inFz) {
        this->forces.setPosition(inFx , inFy, inFz);
    }
};


#endif //FEXTENDFORCES_HPP


