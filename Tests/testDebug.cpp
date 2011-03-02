// /!\ Please, you must read the license at the bottom of this page

#include "../Sources/Utils/FDebug.hpp"

// Compile by : g++ testDebug.cpp ../Sources/Utils/FDebug.cpp -o testDebug.exe

/**
* In this file we show how to use the debug module
*/

int main(void){
	// Print data simply
	FDEBUG( FDebug::Controller << "Hello Wordl\n");

	// Print a variable (formated print)
	int i = 50;
	FDEBUG( FDebug::Controller.writeVariableFromLine( "i", i, __LINE__, __FILE__););

	// Write a developer information
	FDEBUG( FDebug::Controller.writeFromLine("Strange things are there!", __LINE__, __FILE__); )

	// Change stream type
	FDEBUG( FDebug::Controller.writeToFile("testDebug.out.temp"); )
	FDEBUG( FDebug::Controller << "Hello Wordl 2 the return\n");

	return 0;
}


// [--LICENSE--]
