// ===================================================================================
// Copyright ScalFmm 2016 INRIA, Olivier Coulaud, Bérenger Bramas,
// Matthias Messner olivier.coulaud@inria.fr, berenger.bramas@inria.fr
// This software is a computer program whose purpose is to compute the
// FMM.
//
// This software is governed by the CeCILL-C and LGPL licenses and
// abiding by the rules of distribution of free software.
// An extension to the license is given to allow static linking of scalfmm
// inside a proprietary application (no matter its license).
// See the main license file for more details.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public and CeCILL-C Licenses for more details.
// "http://www.cecill.info".
// "http://www.gnu.org/licenses".
// ===================================================================================
#include "FUTester.hpp"
#include "Utils/FQuickSort.hpp"

#include <unistd.h>

/**
* This file is a unit test for the quick sort
*/


/** this class test the list container */
class TestQuickSort : public FUTester<TestQuickSort> {
    static bool IsSorted(long long array[], const long size){
        for(int idx = 1; idx < size ; ++idx){
            if(array[idx-1] > array[idx]){
                return false;
            }
        }
        return true;
    }

    void manyThreads(){
        const long Size = 100000;
        long long* const array = new long long[Size];
        srand48(0);
        const int originalThreadsNumber = omp_get_num_threads();

        for(int idxThread = 1 ; idxThread <= omp_get_max_threads() ; idxThread *= 2){
            omp_set_num_threads(idxThread);

            for(long idx = 0 ; idx < Size ; ++idx){
                array[idx] = rand();
            }

            FQuickSort<long long, long>::QsOmp(array, Size);

            uassert(IsSorted(array,Size));
        }

        omp_set_num_threads(originalThreadsNumber);
        delete [] array;
    }

    void bigSize(){
        const long Size = 10000000;//100000000;
        long long* const array = new long long[Size];

        for(long idx = 0 ; idx < Size ; ++idx){
            array[idx] = rand();
        }

        FQuickSort<long long, long>::QsOmp(array, Size);
        uassert(IsSorted(array,Size));

        delete [] array;
    }

    void reversed(){
        const long Size = 10000000;//100000000;
        long long* const array = new long long[Size];

        for(long idx = 0 ; idx < Size ; ++idx){
            array[idx] = Size-idx;
        }

        FQuickSort<long long, long>::QsOmp(array, Size);
        uassert(IsSorted(array,Size));

        delete [] array;
    }

    void alreadySorted(){
        const long Size = 10000000;//100000000;
        long long* const array = new long long[Size];

        for(long idx = 0 ; idx < Size ; ++idx){
            array[idx] = idx;
        }

        FQuickSort<long long, long>::QsOmp(array, Size);
        uassert(IsSorted(array,Size));

        delete [] array;
    }

    void verySmallParts(){
        {
            long long values[2] = {0, 1};
            FQuickSort<long long, long>::QsSequential(values, 2);
            uassert(values[0] == 0);
            uassert(values[1] == 1);
        }
        {
            long long values[2] = {1, 0};
            FQuickSort<long long, long>::QsSequential(values, 2);
            uassert(values[0] == 0);
            uassert(values[1] == 1);
        }
        {
            long long values[2] = {0, 0};
            FQuickSort<long long, long>::QsSequential(values, 2);
            uassert(values[0] == 0);
            uassert(values[1] == 0);
        }
    }

    // set test
    void SetTests(){
        AddTest(&TestQuickSort::manyThreads,"Many threads");
        AddTest(&TestQuickSort::bigSize,"Big sort");
        AddTest(&TestQuickSort::reversed,"Reversed");
        AddTest(&TestQuickSort::alreadySorted,"Already Sorted");
        AddTest(&TestQuickSort::verySmallParts,"Small Parts");
    }
};

// You must do this
TestClass(TestQuickSort)



