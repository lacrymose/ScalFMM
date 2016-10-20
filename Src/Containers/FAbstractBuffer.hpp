// ===================================================================================
// Copyright ScalFmm 2016 INRIA
//
// This software is a computer program whose purpose is to compute the FMM.
//
// This software is governed by Mozilla Public License Version 2.0 (MPL 2.0) and
// abiding by the rules of distribution of free software.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Mozilla Public License Version 2.0 (MPL 2.0) for more details.
// https://www.mozilla.org/en-US/MPL/2.0/
// ===================================================================================
#ifndef FABSTRACTBUFFER_HPP
#define FABSTRACTBUFFER_HPP

/**
 * @brief The FAbstractBufferReader class defines what is an abstract buffer reader.
 * The buffer used by the mpi algorithm for example should defines this methods.
 */
class FAbstractBufferReader {
public:
    virtual ~FAbstractBufferReader(){
    }

    virtual char*       data()      = 0;
    virtual const char* data()      const  = 0;
    virtual FSize         getSize()   const = 0;
    virtual void        seek(const FSize inIndex) = 0;
    virtual FSize         tell()      const  = 0;

    template <class ClassType>
    ClassType getValue(){
        static_assert(sizeof(ClassType) == 0, "Your Buffer should implement getValue.");
        return ClassType();
    }
    template <class ClassType>
    void fillValue(ClassType* const){
        static_assert(sizeof(ClassType) == 0, "Your Buffer should implement fillValue.");
    }
    template <class ClassType>
    void fillArray(ClassType* const , const FSize ){
        static_assert(sizeof(ClassType) == 0, "Your Buffer should implement fillArray.");
    }
    template <class ClassType>
    FAbstractBufferReader& operator>>(ClassType& ){
        static_assert(sizeof(ClassType) == 0, "Your Buffer should implement operator>>.");
        return *this;
    }
};


/**
 * @brief The FAbstractBufferWriter class defines what is an abstract buffer writer.
 * The buffer used by the mpi algorithm for example should defines this methods.
 */
class FAbstractBufferWriter {
public:
    virtual ~FAbstractBufferWriter(){
    }

    virtual char*       data()  = 0;
    virtual const char* data()  const = 0;
    virtual FSize         getSize() const = 0;
    virtual void        reset() = 0;

    template <class ClassType>
    void write(const ClassType& object){
        static_assert(sizeof(ClassType) == 0, "Your Buffer should implement write.");
    }
    template <class ClassType>
    void writeAt(const FSize position, const ClassType& object){
        static_assert(sizeof(ClassType) == 0, "Your Buffer should implement writeAt.");
    }
    template <class ClassType>
    void write(const ClassType* const objects, const FSize inSize){
        static_assert(sizeof(ClassType) == 0, "Your Buffer should implement write.");
    }
    template <class ClassType>
    FAbstractBufferWriter& operator<<(const ClassType& ){
        static_assert(sizeof(ClassType) == 0, "Your Buffer should implement operator<<.");
        return *this;
    }
};

#endif // FABSTRACTBUFFER_HPP
