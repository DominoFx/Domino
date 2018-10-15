#include <cstdlib>
#include <unistd.h>
#include <thread>
#include "dmx/DominoController.h"

using namespace std;

int main(int argc, char** argv)
{
    printf( "DominoFX: Creating DominoController...\n" );
    DominoController controller;
    
    printf( "DominoFX: Initializing DominoController...\n" );
    if(!controller.Init( argc, argv ))
    {
        printf( "DominoFX: Failure initializing DominoController, aborting\n" );
        return -1;
    }
    
    printf( "DominoFX: Starting main loop...\n" );
    while(true)
    {
        controller.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    printf( "DominoFX: Finished\n" );
    return 0;
}

