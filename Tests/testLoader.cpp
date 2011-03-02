// /!\ Please, you must read the license at the bottom of this page

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>

#include "../Sources/Containers/FOctree.hpp"
#include "../Sources/Containers/FList.hpp"

#include "../Sources/Utils/FAssertable.hpp"
#include "../Sources/Utils/F3DPosition.hpp"

#include "../Sources/Core/FBasicParticule.hpp"

#include "../Sources/Files/FBasicLoader.hpp"

// Compile by : g++ testLoader.cpp ../Sources/Utils/FAssertable.cpp -O2 -lgomp -fopenmp -o testLoader.exe

// Fake cell class
class TestCell{
};

/**
  * In this file we show an example of BasicParticule and BasicLoader use
* Démarrage de /home/berenger/Dropbox/Personnel/FMB++/FMB++-build-desktop/FMB++...
* Inserting 2000000 particules ...
* Done  (5.77996).
* Deleting particules ...
* Done  (0.171918).
  */

int main(int , char ** ){
    // we store all particules to be able to dealloc
    FList<FBasicParticule*> particules;
    // Use testLoaderCreate.exe to create this file
    const char* const filename = "testLoader.basic.temp";

    // open basic particules loader
    FBasicLoader<FBasicParticule> loader(filename);
    if(!loader.isValide()){
        std::cout << "Loader Error, " << filename << "is missing\n";
        return 1;
    }

    // otree
    FOctree<FBasicParticule, TestCell, 10, 3> tree(loader.getBoxWidth(),loader.getCenterOfBox());

    // -----------------------------------------------------
    std::cout << "Inserting " << loader.getNumberOfParticules() << " particules ..." << std::endl;
    const double InsertingStartTime = omp_get_wtime();
    for(int idx = 0 ; idx < loader.getNumberOfParticules() ; ++idx){
        FBasicParticule* const part = new FBasicParticule();
        particules.pushFront(part);
        loader.fillParticule(part);
        tree.insert(part);
    }
    const double InsertingEndTime = omp_get_wtime();
    std::cout << "Done  " << "(" << (InsertingEndTime-InsertingStartTime) << ")." << std::endl;

    // -----------------------------------------------------
    std::cout << "Deleting particules ..." << std::endl;
    const double DeletingStartTime = omp_get_wtime();
    while(particules.getSize()){
        delete particules.popFront();
    }
    const double DeletingEndTime = omp_get_wtime();
    std::cout << "Done  " << "(" << (DeletingEndTime-DeletingStartTime) << ")." << std::endl;
    // -----------------------------------------------------

    return 0;
}


// [--LICENSE--]
