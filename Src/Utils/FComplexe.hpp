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
#ifndef FCOMPLEXE_HPP
#define FCOMPLEXE_HPP


#include "FMath.hpp"

/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
* @class 
* Please read the license
*
* Propose basic complexe class.
* Do not modify the attributes of this class.
* It can be passed to blas fonction and has to be
* 2 x complex[0] size only.
*/
class FComplexe {
    FReal complex[2];    //< Real & Imaginary

public:
    /** Default Constructor (set complex[0]&imaginary to 0) */
    FComplexe() {
        complex[0] = 0;
        complex[1] = 0;
    }

    /** Constructor with values
      * @param inImag the imaginary
      * @param inReal the complex[0]
      */
    explicit FComplexe(const FReal inReal, const FReal inImag) {
        complex[0] = inReal;
        complex[1] = inImag;
    }

    /** Copy constructor */
    FComplexe(const FComplexe& other){
        complex[0] = other.complex[0];
        complex[1] = other.complex[1];
    }

    /** Copy operator */
    FComplexe& operator=(const FComplexe& other){
        this->complex[1] = other.complex[1];
        this->complex[0] = other.complex[0];
        return *this;
    }

    /** Equality operator */
    bool operator==(const FComplexe& other){
        return FMath::LookEqual(this->complex[1],other.complex[1])
                       && FMath::LookEqual(this->complex[0],other.complex[0]);
    }

    /** Different equal */
    bool operator!=(const FComplexe& other){
        return !(*this == other);
    }

    /** Get imaginary */
    FReal getImag() const{
        return this->complex[1];
    }

    /** Get complex[0] */
    FReal getReal() const{
        return this->complex[0];
    }

    /** Set Imaginary */
    void setImag(const FReal inImag) {
        this->complex[1] = inImag;
    }

    /** Set Real */
    void setReal(const FReal inReal) {
        this->complex[0] = inReal;
    }

    /** Set Real and imaginary */
    void setRealImag(const FReal inReal, const FReal inImag) {
        this->complex[0] = inReal;
        this->complex[1] = inImag;
    }

    /** Return the conjugate */
    FComplexe conjugate() const{
        return FComplexe(complex[0],-complex[1]);
    }

    /** Return the conjugate */
    FComplexe negate() const{
        return FComplexe(-complex[0],-complex[1]);
    }

    /**
     * Operator +=
     * in complex[0] with other complex[0], same for complex[1]
     * @param other the complexe to use data
     */
    FComplexe& operator+=(const FComplexe& other){
        this->complex[0] += other.complex[0];
        this->complex[1] += other.complex[1];
        return *this;
    }   

    /** Inc complex[0] and imaginary by values
      * @param inIncReal to inc the complex[0]
      * @param inIncImag to inc the complex[1]
      */
    void inc(const FReal inIncReal, const FReal inIncImag){
        this->complex[0] += inIncReal;
        this->complex[1] += inIncImag;
    }

    /** Inc complex[0] by FReal
      * @param inIncReal to inc the complex[0]
      */
    void incReal(const FReal inIncReal){
        this->complex[0] += inIncReal;
    }

    /** Inc imaginary by FReal
      * @param inIncImag to inc the complex[1]
      */
    void incImag(const FReal inIncImag){
        this->complex[1] += inIncImag;
    }

    /** Dec complex[0] by FReal
      * @param inDecReal to dec the complex[0]
      */
    void decReal(const FReal inIncReal){
        this->complex[0] -= inIncReal;
    }

    /** Dec imaginary by FReal
      * @param inDecImag to dec the complex[1]
      */
    void decImag(const FReal inIncImag){
        this->complex[1] -= inIncImag;
    }

    /** Mul complex[0] and imaginary by a FReal
      * @param inValue the coef to mul data
      */
    void mulRealAndImag(const FReal inValue){
        this->complex[1] *= inValue;
        this->complex[0] *= inValue;
    }

    /** Mul a complexe by another "c*=c2" */
    FComplexe& operator*=(const FComplexe& other){
        const FReal tempReal = this->complex[0];
        this->complex[0] = (tempReal * other.complex[0]) - (this->complex[1] * other.complex[1]);
        this->complex[1] = (tempReal * other.complex[1]) + (this->complex[1] * other.complex[0]);
        return *this;
    }

    /** Mul a complexe by another "c*=c2" */
    FComplexe& operator*=(const FReal& real){
        this->complex[0] *= real;
        this->complex[1] *= real;
        return *this;
    }

    /** Test if a complex is not a number */
    bool isNan() const {
        return FMath::IsNan(complex[1]) || FMath::IsNan(complex[0]);
    }

    /** Mul other and another and add the result to current complexe */
    void addMul(const FComplexe& other, const FComplexe& another){
        this->complex[0] += (other.complex[0] * another.complex[0]) - (other.complex[1] * another.complex[1]);
        this->complex[1] += (other.complex[0] * another.complex[1]) + (other.complex[1] * another.complex[0]);
    }

    /** Mul other and another and add the result to current complexe */
    void equalMul(const FComplexe& other, const FComplexe& another){
        this->complex[0] = (other.complex[0] * another.complex[0]) - (other.complex[1] * another.complex[1]);
        this->complex[1] = (other.complex[0] * another.complex[1]) + (other.complex[1] * another.complex[0]);
    }

    /** To cast to FReal */
    static FReal* ToFReal(FComplexe*const array){
        return reinterpret_cast<FReal*>(array);
    }

    /** To cast to FReal */
    static const FReal* ToFReal(const FComplexe*const array){
        return reinterpret_cast<const FReal*>(array);
    }
    /**
     * Operator stream FComplexe to std::ostream
     * This can be used to simpldata[1] write out a complex with format (Re,Im)
     * @param[in,out] output where to write the position
     * @param[in] inPosition the position to write out
     * @return the output for multiple << operators
     */
    template <class StreamClass>
    friend StreamClass& operator<<(StreamClass& output, const FComplexe& inC){
        output << "(" <<  inC.getReal() << ", " << inC.getImag() << ")";
        return output;  // for multiple << operators.
    }

};

/** Global operator Mul a complexe by another "c=c1*c2" */
inline FComplexe operator*=(const FComplexe& first, const FComplexe& second){
    return FComplexe(
            (first.getReal() * second.getImag()) + (first.getImag() * second.getReal()),
            (first.getReal() * second.getReal()) - (first.getImag() * second.getImag())
            );
}

#endif //FCOMPLEXE_HPP


