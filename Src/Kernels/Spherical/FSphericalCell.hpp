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
#ifndef FSPHERICALCELL_HPP
#define FSPHERICALCELL_HPP

#include "../../Utils/FComplex.hpp"
#include "../../Utils/FMemUtils.hpp"

#include "../../Extensions/FExtendCellType.hpp"

#include "../../Components/FBasicCell.hpp"

/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
*/
template <class FReal>
class FSphericalCell : public FBasicCell {
protected:
    static int DevP;
    static int LocalSize;
    static int PoleSize;
    static bool UseBlas;

    FComplex<FReal>* multipole_exp; //< For multipole extenssion
    FComplex<FReal>* local_exp;     //< For local extenssion

public:
    static void Init(const int inDevP, const bool inUseBlas = false){
        DevP  = inDevP;
        const int ExpP  = int((inDevP+1) * (inDevP+2) * 0.5);
        const int NExpP = (inDevP+1) * (inDevP+1);

        LocalSize = ExpP;
        if(inUseBlas) {
            PoleSize = NExpP;
        }
        else{
            PoleSize = ExpP;
        }
    }

    static int GetLocalSize(){
        return LocalSize;
    }

    static int GetPoleSize(){
        return PoleSize;
    }

    /** Default constructor */
    FSphericalCell()
        : multipole_exp(nullptr), local_exp(nullptr){
        multipole_exp = new FComplex<FReal>[PoleSize];
        local_exp = new FComplex<FReal>[LocalSize];
    }

    /** Constructor */
    FSphericalCell(const FSphericalCell<FReal>& other)
        : multipole_exp(nullptr), local_exp(nullptr){
        multipole_exp = new FComplex<FReal>[PoleSize];
        local_exp = new FComplex<FReal>[LocalSize];
        (*this) = other;
    }

    /** Default destructor */
    virtual ~FSphericalCell(){
        delete[] multipole_exp;
        delete[] local_exp;
    }

    /** Copy constructor */
    FSphericalCell<FReal>& operator=(const FSphericalCell<FReal>& other) {
        FMemUtils::copyall(multipole_exp, other.multipole_exp, PoleSize);
        FMemUtils::copyall(local_exp, other.local_exp, LocalSize);
        return *this;
    }

    /** Get Multipole */
    const FComplex<FReal>* getMultipole() const {
        return multipole_exp;
    }
    /** Get Local */
    const FComplex<FReal>* getLocal() const {
        return local_exp;
    }

    /** Get Multipole */
    FComplex<FReal>* getMultipole() {
        return multipole_exp;
    }
    /** Get Local */
    FComplex<FReal>* getLocal() {
        return local_exp;
    }

    /** Make it like the begining */
    void resetToInitialState(){
        for(int idx = 0 ; idx < PoleSize ; ++idx){
            multipole_exp[idx].setRealImag(FReal(0.0), FReal(0.0));
        }
        for(int idx = 0 ; idx < LocalSize ; ++idx){
            local_exp[idx].setRealImag(FReal(0.0), FReal(0.0));
        }
    }

    ///////////////////////////////////////////////////////
    // to extend FAbstractSendable
    ///////////////////////////////////////////////////////
    template <class BufferWriterClass>
    void serializeUp(BufferWriterClass& buffer) const{
        buffer.write(multipole_exp, PoleSize);
    }
    template <class BufferReaderClass>
    void deserializeUp(BufferReaderClass& buffer){
        buffer.fillArray(multipole_exp, PoleSize);
    }

    template <class BufferWriterClass>
    void serializeDown(BufferWriterClass& buffer) const{
        buffer.write(local_exp, LocalSize);
    }
    template <class BufferReaderClass>
    void deserializeDown(BufferReaderClass& buffer){
        buffer.fillArray(local_exp, LocalSize);
    }

    ///////////////////////////////////////////////////////
    // to extend Serializable
    ///////////////////////////////////////////////////////
    template <class BufferWriterClass>
    void save(BufferWriterClass& buffer) const{
        FBasicCell::save(buffer);
        buffer.write(multipole_exp, PoleSize);
        buffer.write(local_exp, LocalSize);
    }
    template <class BufferReaderClass>
    void restore(BufferReaderClass& buffer){
        FBasicCell::restore(buffer);
        buffer.fillArray(multipole_exp, PoleSize);
        buffer.fillArray(local_exp, LocalSize);
    }

    int getSavedSize() const {
        return ((int) sizeof(FComplex<FReal>)) * (PoleSize+LocalSize)
                + FBasicCell::getSavedSize();
    }

    int getSavedSizeUp() const {
        return ((int) sizeof(FComplex<FReal>)) * (PoleSize);
    }

    int getSavedSizeDown() const {
        return ((int) sizeof(FComplex<FReal>)) * (LocalSize);
    }
};

template <class FReal>
int FSphericalCell<FReal>::DevP(-1);
template <class FReal>
int FSphericalCell<FReal>::LocalSize(-1);
template <class FReal>
int FSphericalCell<FReal>::PoleSize(-1);


/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
*/
template <class FReal>
class FTypedSphericalCell : public FSphericalCell<FReal>, public FExtendCellType {
public:
    template <class BufferWriterClass>
    void save(BufferWriterClass& buffer) const{
        FSphericalCell<FReal>::save(buffer);
        FExtendCellType::save(buffer);
    }
    template <class BufferReaderClass>
    void restore(BufferReaderClass& buffer){
        FSphericalCell<FReal>::restore(buffer);
        FExtendCellType::restore(buffer);
    }
    void resetToInitialState(){
        FSphericalCell<FReal>::resetToInitialState();
        FExtendCellType::resetToInitialState();
    }

    int getSavedSize() const {
        return FExtendCellType::getSavedSize() + FSphericalCell<FReal>::getSavedSize();
    }
};



#endif //FSPHERICALCELL_HPP


