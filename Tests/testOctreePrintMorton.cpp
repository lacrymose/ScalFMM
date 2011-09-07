// /!\ Please, you must read the license at the bottom of this page

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>

#include "../Src/Utils/FGlobal.hpp"
#include "../Src/Containers/FTreeCoordinate.hpp"
#include "../Src/Utils/F3DPosition.hpp"
#include "../Src/Utils/FMath.hpp"

// Compile by : g++ testOctreePrintMorton.cpp -O2 -o testOctreePrintMorton.exe

/**
* In this file we show the morton indexed for each boxes
* in a tree (but we did not build the tree because if we build
* an entire tree it may be tooo big!)
*/


/** Basic function to convert a morton index in decimal string */
std::string MortonToBinary(MortonIndex index, int level){
    std::string str;
    int bits = 1 << ((level * 3) - 1);
    int dim = 0;
    while(bits){
        if(index & bits) str.append("1");
        else str.append("0");
        bits >>= 1;
        // we put a dot each 3 values
        if(++dim == 3){
            str.append(".");
            dim = 0;
        }
    }
    return str;
}


int main(int , char ** ){
    ///////////////////////What we do/////////////////////////////
    std::cout << ">> This executable has to be used to test morton index.\n";
    //////////////////////////////////////////////////////////////

    bool stop = false;
    char buffer[256];

    int treeLevel = 10;
    F3DPosition centerOfBox(0.5,0.5,0.5);
    double rootBoxWidth = 1;

    std::cout << "Welcome in the morton index test." << std::endl;

    while(!stop){
        /////////////////////////////////////////////////////////////////////////
        // Menu
        /////////////////////////////////////////////////////////////////////////
        std::cout << "-- Current states :\n";
        std::cout << "     tree level = " << treeLevel << "\n";
        std::cout << "     box width = " << rootBoxWidth << "\n";
        std::cout << "     center of the box ; x = " << centerOfBox.getX() << " y = " << centerOfBox.getY() << " z = " << centerOfBox.getZ() << "\n";

        std::cout << "-- Menu :\n";
        std::cout << "   1 - Print a morton index from position in tree\n";
        std::cout << "   2 - Print a morton index from real position\n";
        std::cout << "   3 - Print an interval of morton index\n";
        std::cout << "   4 - Print all morton index for a level\n";
        std::cout << "   5 - Change tree properties\n";
        std::cout << "   6 - Find tree coordinate & position from morton index & level\n";
        std::cout << "   0 - Quit current app\n";

        int userChoice = 0;
        do{
            std::cout << "Select action : ";
            std::cin.getline( buffer , sizeof(buffer) , '\n');
            userChoice = -1;
            sscanf(buffer,"%d",&userChoice);
        } while( userChoice < 0 || userChoice > 7);

        /////////////////////////////////////////////////////////////////////////
        // Actions
        /////////////////////////////////////////////////////////////////////////
        switch(userChoice){
        case 0: stop = true;
            break;
        case 1: /////////////////////////////////////////////////////////////////
            {
                std::cout << "-- Morton index from position :\n";
                std::cout << "    You will now give the position in the tree and the level to compute morton index.\n";

                int requiredlevel = treeLevel - 1;
                std::cout << "    level (default is leaf level = " << (treeLevel-1) << ") : ";
                std::cin.getline( buffer , sizeof(buffer));
                if( buffer[0] != '\0' ){
                    sscanf(buffer,"%d",&requiredlevel);
                }

                const int maxBoxAtThisLevel = 1 << requiredlevel;
                std::cout << "    At level "<< requiredlevel << " there is a grid of [" << maxBoxAtThisLevel << " x " << maxBoxAtThisLevel << " x " << maxBoxAtThisLevel << "] boxes\n";

                int x,y,z;
                do{
                    std::cout << "    Tapes x y z = ";
                    std::cin.getline( buffer , sizeof(buffer));
                }while(sscanf(buffer,"%d %d %d",&x,&y,&z) != 3);

                FTreeCoordinate coord(x,y,z);
                const MortonIndex index = coord.getMortonIndex(requiredlevel) ;
                std::cout << "    Morton Index is " << index << " \t " << std::hex << index << "H \t " << MortonToBinary(index,requiredlevel) << "D\n\n";
            }
            break;
        case 2: /////////////////////////////////////////////////////////////////
            {
                std::cout << "-- Morton index from real position :\n";
                std::cout << "    You will now give the position in the tree and the level to compute morton index.\n";

                int requiredlevel = treeLevel - 1;
                std::cout << "    level (default is leaf level = " << (treeLevel-1) << ") : ";
                std::cin.getline( buffer , sizeof(buffer));
                if( buffer[0] != '\0' ){
                    sscanf(buffer,"%d",&requiredlevel);
                }

                double boxWidthAtThisLevel = rootBoxWidth;
                for(int idx = 0 ; idx < requiredlevel ; ++idx) boxWidthAtThisLevel /= 2.0;
                std::cout << "    At level "<< requiredlevel << " boxes width is " << boxWidthAtThisLevel << "\n";

                float x,y,z;
                do{
                    std::cout << "    Tapes x y z = ";
                    std::cin.getline( buffer , sizeof(buffer));
                }while(sscanf(buffer,"%f %f %f",&x,&y,&z) != 3);

                FTreeCoordinate host;
                // position has to be relative to corner not center
                host.setX( long(FMath::dfloor(( x - centerOfBox.getX() - rootBoxWidth/2) / boxWidthAtThisLevel ) ));
                host.setY( long(FMath::dfloor(( y - centerOfBox.getY() - rootBoxWidth/2) / boxWidthAtThisLevel ) ));
                host.setZ( long(FMath::dfloor(( z - centerOfBox.getZ() - rootBoxWidth/2) / boxWidthAtThisLevel ) ));

                const MortonIndex index = host.getMortonIndex(requiredlevel);
                std::cout << "    Morton Index is " << index << " \t " << std::hex << index << "h \t " << MortonToBinary(index,requiredlevel) << "d\n\n";
            }
            break;
        case 3: /////////////////////////////////////////////////////////////////
            {
                std::cout << "-- Morton for an interval :\n";
                std::cout << "    You will now give 2 positions and the level.\n";

                int requiredlevel = treeLevel - 1;
                std::cout << "    level (default is leaf level = " << (treeLevel-1) << ") : ";
                std::cin.getline( buffer , sizeof(buffer));
                if( buffer[0] != '\0' ){
                    sscanf(buffer,"%d",&requiredlevel);
                }

                const int maxBoxAtThisLevel = 1 << requiredlevel;
                std::cout << "    At level "<< requiredlevel << " there is a grid of [" << maxBoxAtThisLevel << " x " << maxBoxAtThisLevel << " x " << maxBoxAtThisLevel << "] boxes\n";

                int sx,sy,sz;
                do{
                    std::cout << "    Tapes start index x y z = ";
                    std::cin.getline( buffer , sizeof(buffer));
                }while(sscanf(buffer,"%d %d %d",&sx,&sy,&sz) != 3);

                int ex,ey,ez;
                do{
                    std::cout << "    Tapes end index x y z = ";
                    std::cin.getline( buffer , sizeof(buffer));
                }while(sscanf(buffer,"%d %d %d",&ex,&ey,&ez) != 3);

                for(int z = sz ; z <= ez ; ++z){
                    for(int y = sy ; y <= ey ; ++y){
                        for(int x = sx ; x <= ex ; ++x){
                            FTreeCoordinate coord(x,y,z);
                            const MortonIndex index = coord.getMortonIndex(requiredlevel);
                            std::cout << "[x = " << x << " y = " << y << " z = " << z << "]\n";
                            std::cout << "    Morton Index is " << index << " \t " << std::hex << index << "H \t " << MortonToBinary(index,requiredlevel) << "D\n\n";
                        }
                    }
                }
            }
            break;
        case 4: /////////////////////////////////////////////////////////////////
            {
                std::cout << "-- Morton for a level :\n";
                std::cout << "    You will now give 2 positions and the level.\n";

                int requiredlevel = treeLevel - 1;
                std::cout << "    level (default is leaf level = " << (treeLevel-1) << ") : ";
                std::cin.getline( buffer , sizeof(buffer));
                if( buffer[0] != '\0' ){
                    sscanf(buffer,"%d",&requiredlevel);
                }

                const int maxBoxAtThisLevel = 1 << requiredlevel;

                for(int z = 0 ; z < maxBoxAtThisLevel ; ++z){
                    for(int y = 0 ; y < maxBoxAtThisLevel ; ++y){
                        for(int x = 0 ; x < maxBoxAtThisLevel ; ++x){
                            FTreeCoordinate coord(x,y,z);
                            const MortonIndex index = coord.getMortonIndex(requiredlevel);
                            std::cout << "[x = " << x << " y = " << y << " z = " << z << "]\n";
                            std::cout << "    Morton Index is " << index << " \t " << std::hex << index << "H \t " << MortonToBinary(index,requiredlevel) << "D\n\n";
                        }
                    }
                }
            }
            break;
        case 5: /////////////////////////////////////////////////////////////////
            {
                std::cout << "-- Change properties :\n";
                std::cout << "    You will now give tree height, the center of the box & the box width.\n";

                std::cout << "    height (default is = " << treeLevel << ") : ";
                std::cin.getline( buffer , sizeof(buffer));
                if( buffer[0] != '\0' ){
                    sscanf(buffer,"%d",&treeLevel);
                }

                float x,y,z;
                do{
                    std::cout << "    Center of boxe Tapes x y z = ";
                    std::cin.getline( buffer , sizeof(buffer));
                }while(sscanf(buffer,"%f %f %f",&x,&y,&z) != 3);
                centerOfBox = F3DPosition(x,y,z);

                std::cout << "    boxe width (default is = " << rootBoxWidth << ") : ";
                std::cin.getline( buffer , sizeof(buffer));
                if( buffer[0] != '\0' ){
                    sscanf(buffer,"%lf",&rootBoxWidth);
                }

                std::cout << "\n";
            }
            break;
        case 6: /////////////////////////////////////////////////////////////////
            {
                std::cout << "-- Morton index to position :\n";
                std::cout << "    You will now give the morton index and the level.\n";

                int requiredlevel;
                do{
                    std::cout << "    level = ";
                    std::cin.getline( buffer , sizeof(buffer));
                }while(sscanf(buffer,"%d",&requiredlevel) != 1);

                MortonIndex index;
                do{
                    std::cout << "    index = ";
                    std::cin.getline( buffer , sizeof(buffer));
                }while(sscanf(buffer,"%lld",&index) != 1);

                FTreeCoordinate coord;
                coord.setPositionFromMorton(index,requiredlevel);

                std::cout << "    This position is in the boxe x = "<< coord.getX() << " y = " << coord.getY() << " z = " << coord.getZ() << "\n";

                double boxWidthAtThisLevel = rootBoxWidth;
                for(int idx = 0 ; idx < requiredlevel ; ++idx) boxWidthAtThisLevel /= 2.0;
                std::cout << "    This center of this boxe is"
                        << " x = " << (coord.getX()*boxWidthAtThisLevel) + centerOfBox.getX() + boxWidthAtThisLevel/2 - rootBoxWidth/2
                        << " y = " << (coord.getY()*boxWidthAtThisLevel) + centerOfBox.getX() + boxWidthAtThisLevel/2 - rootBoxWidth/2
                        << " z = " << (coord.getZ()*boxWidthAtThisLevel) + centerOfBox.getX() + boxWidthAtThisLevel/2 - rootBoxWidth/2 << "\n\n";
            }
            break;
        default:
            ;
        }
    }

    return 0;
}


// [--LICENSE--]
