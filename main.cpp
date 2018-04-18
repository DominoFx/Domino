#include <cstdlib>
#include <unistd.h>
#include <thread>
#include "dmx/DmxController.h"

using namespace std;

int main(int argc, char** argv)
{
    DmxController controller;
    
    if(!controller.Init())
    {
        return -1;
    }
    
    while(true)
    {
        controller.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    return 0;
}

