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
#include "FUTester.hpp"

#include "../Src/Containers/FList.hpp"

// compile by g++ utestList.cpp -o utestList.exe

/**
* This file is a unit test for the FList class
*/

/**
* This class is simply used to count alloc dealloc
*/
class TestObject{
public:
	static int counter;
	static int dealloced;

	TestObject(){
		++counter;
	}
	TestObject(const TestObject&){
		++counter;
	}
	~TestObject(){
		++dealloced;
	}
};

int TestObject::counter(0);
int TestObject::dealloced(0);


/** this class test the list container */
class TestList : public FUTester<TestList> {
	// Called before each test : simply set counter to 0
	void PreTest(){
		TestObject::counter = 0;
		TestObject::dealloced = 0;
	}

	// test size
	void TestSize(){
		FList<TestObject> list;
		list.push(TestObject());
		list.push(TestObject());
		list.push(TestObject());
                uassert(list.getSize() == 3);
		
                uassert((TestObject::counter - TestObject::dealloced) == list.getSize());

		list.clear();
                uassert(list.getSize() == 0);

                uassert(TestObject::counter == TestObject::dealloced);
	}
	
	// test copy
	void TestCopy(){
		FList<TestObject> list;
		list.push(TestObject());
		list.push(TestObject());
		list.push(TestObject());

		FList<TestObject> list2 = list;
                uassert(list.getSize() == list2.getSize());
		
                uassert((TestObject::counter - TestObject::dealloced) == (list.getSize() + list2.getSize()));
	}

	// test iter
	void TestIter(){		
		FList<TestObject> list;
		{
                        FList<TestObject>::ConstBasicIterator iter(list);
                        uassert(!iter.hasNotFinished());
		}
		{
			list.push(TestObject());
			list.push(TestObject());
			list.push(TestObject());

                        FList<TestObject>::ConstBasicIterator iter(list);
                        uassert(iter.hasNotFinished());

			int counter = 0;
                        while(iter.hasNotFinished()){
                            iter.gotoNext();
                            ++counter;
                        }

                        uassert(!iter.hasNotFinished());
                        uassert(counter == list.getSize());
		}
	}
		
	// set test
	void SetTests(){
            AddTest(&TestList::TestSize,"Test Size");
            AddTest(&TestList::TestCopy,"Test Copy");
            AddTest(&TestList::TestIter,"Test Iter");
	}
};

// You must do this
TestClass(TestList)


