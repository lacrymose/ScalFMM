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
#ifndef FEXTENDPOSITION_HPP
#define FEXTENDPOSITION_HPP


#include "../Utils/FGlobal.hpp"
#include "../Utils/FPoint.hpp"
#include "../Containers/FBufferReader.hpp"
#include "../Containers/FBufferWriter.hpp"

/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
* @class FExtendPosition
* Please read the license
* This class is an extenssion.
* It proposes a position from a FPoint.
*/
class FExtendPosition {
protected:
    FPoint position; //< The position

public:
    /** Default constructor */
    FExtendPosition() {
    }

    /** Copy constructor */
    FExtendPosition(const FExtendPosition& other) : position(other.position) {
    }

    /** Copy operator */
    FExtendPosition& operator=(const FExtendPosition& other) {
        this->position = other.position;
        return *this;
    }

    /** To get the position */
    const FPoint& getPosition() const {
        return this->position;
    }

    /** To set the position */
    void setPosition(const FPoint& inPosition) {
        this->position = inPosition;
    }

    /** To set the position from 3 FReals */
    void setPosition(const FReal inX, const FReal inY, const FReal inZ) {
        this->position.setX(inX);
        this->position.setY(inY);
        this->position.setZ(inZ);
    }

    /** Set Position */
    void incPosition(const FPoint& inPosition) {
        this->position += inPosition;
    }

    /** Set Position with 3 FReals */
    void incPosition(const FReal inPx, const FReal inPy, const FReal inPz) {
        this->position.incX(inPx);
        this->position.incY(inPy);
        this->position.incZ(inPz);
    }

    /** Save current object */
    void save(FBufferWriter& buffer) const {
        position.save(buffer);
    }
    /** Retrieve current object */
    void restore(FBufferReader& buffer) {
        position.restore(buffer);
    }
};


#endif //FEXTENDPOSITION_HPP


