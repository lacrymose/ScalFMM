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
#include <iostream>
#include "../../Src/Utils/FTic.hpp"

#include <cstdlib>
#include <unistd.h>

/**
* Here we show an example of using FTic
*/

int main(){
    ///////////////////////What we do/////////////////////////////
    std::cout << ">> This executable is useless to execute.\n";
    std::cout << ">> It is only interesting to wath the code to understand\n";
    std::cout << ">> how to use FTic time counter.\n";
    //////////////////////////////////////////////////////////////
    {
	FTic counter;	
	counter.tic();
	usleep(1500000);
	//Sleep(1500); //on windows
	counter.tac();
	std::cout << counter.elapsed() << " (s)\n";
    }
    {
        FTic counter;
        usleep(1500000);
        //Sleep(1500); //on windows
        std::cout << counter.tacAndElapsed() << " (s)\n";
    }
    {
        FTic counter;
        usleep(1500000);
        //Sleep(1500); //on windows
        counter.tac();
        counter.tic();
        usleep(1500000);
        //Sleep(1500); //on windows
        std::cout << counter.tacAndElapsed() << " (s)\n";
        std::cout << counter.cumulated() << " (s)\n";
    }
    return 0;
}

