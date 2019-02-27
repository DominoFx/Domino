#include <cstdlib>
#include <unistd.h>
#include <thread>
#include "dmx/DominoController.h"

using namespace std;

int main(int argc, char** argv)
{
    //OscBroadcaster broadcast;
    //int result = broadcast.Init( 12345 ); //broadcast.Init( "192.168.0.119", 12345 );
    //printf( "DominoFX: broadcast test init %s...\n", (result?"ok":"fail") );
    //while( true )
    //{
    //    int testdata[] = {1,2,3};
    //    std::string testtag("/broadcast");
    //    printf( "DominoFX: ...marco...\n" );
    //    broadcast.Send( testtag, 3, testdata );
    //    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //}
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

