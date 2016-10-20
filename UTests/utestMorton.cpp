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
#include "FUTester.hpp"

#include "Containers/FTreeCoordinate.hpp"

// compile by g++ utestMorton.cpp -o utestMorton.exe

/**
* This file is a unit test for the FTreeCoordinate class
*/


/** this class test the list container */
class TestMorton : public FUTester<TestMorton> {
        void Morton(){
            {
                FTreeCoordinate pos(5,1,7);
                FTreeCoordinate cp;
                cp.setPositionFromMorton(pos.getMortonIndex());
                uassert(pos == cp);
                uassert(cp.getMortonIndex() == pos.getMortonIndex());
            }
            {
                FTreeCoordinate pos(2,8,3);
                FTreeCoordinate cp;
                cp.setPositionFromMorton(pos.getMortonIndex());
                uassert(pos == cp);
                uassert(cp.getMortonIndex() == pos.getMortonIndex());
            }
            {
                FTreeCoordinate pos(51,11,47);
                FTreeCoordinate cp;
                cp.setPositionFromMorton(pos.getMortonIndex());
                uassert(pos == cp);
                uassert(cp.getMortonIndex() == pos.getMortonIndex());
            }
	}

        void Position(){
            {
                FTreeCoordinate pos(0,0,0);
                uassert(pos.getMortonIndex() == 0);
            }
            {
                FTreeCoordinate pos(1,1,1);
                uassert(pos.getMortonIndex() == 7);
            }
            {
                FTreeCoordinate pos(0,1,1);
                uassert(pos.getMortonIndex() == 3);
            }
            {
                FTreeCoordinate pos(2,2,2);
                uassert(pos.getMortonIndex() == (7 << 3) );
            }
            {
                FTreeCoordinate pos(1,2,4);
                uassert(pos.getMortonIndex() == 84 );// 001 010 100 =>> 001010100 => 84d
            }
	}

	// set test
	void SetTests(){
            AddTest(&TestMorton::Morton,"Test Morton");
            AddTest(&TestMorton::Position,"Test Position");
	}
};

// You must do this
TestClass(TestMorton)
