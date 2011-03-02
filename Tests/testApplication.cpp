// /!\ Please, you must read the license at the bottom of this page

//#define FUSE_MPI

#ifdef FUSE_MPI
// Compile by mpic++ testApplication.cpp -o testApplication.exe
// run by mpirun -np 4 ./testApplication
#include "../Sources/Utils/FMpiApplication.hpp"
#define ApplicationImplementation FMpiApplication
#else
// Compile by g++ testApplication.cpp -o testApplication.exe
#include "../Sources/Utils/FSingleApplication.hpp"
#define ApplicationImplementation FSingleApplication
#endif

#include <stdio.h>


/**
* In this file we show how to use the application module
* please refere to the source of testApplication.cpp directly to know more
*/

/**
* FApp is an example of the FApplication
*/
class FApp : public ApplicationImplementation{
public:
	FApp(const int inArgc, char ** const inArgv )
		: ApplicationImplementation(inArgc,inArgv) {
	}

protected:
	void initMaster(){
		printf("I am %d on %d, I am master\n", processId(), processCount());

		const std::string argStr = userParemeterAt<std::string>(0);
		printf("[Master] arg str = %s\n", argStr.c_str());	// will print ./testApplication
		const int argInt = userParemeterAt<int>(0,-1);
		printf("[Master] arg int = %d\n", argInt);		// will print -1
	}
	void initSlave(){
		printf("I am %d on %d, I am slave\n", processId(), processCount());
	}

	void run(){
		printf("I am %d, I start to work\n",processId());		
		for(long idx = 0 ; idx < 50000000 ; ++idx) {++idx;--idx;}
		processBarrier();
		printf("I am %d, I just finished\n",processId());
	}
};

// Usual Main
int main(int argc, char ** argv){
	FApp app(argc,argv);
	return app.execute();
}


// [--LICENSE--]
