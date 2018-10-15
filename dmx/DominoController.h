#ifndef DMXCONTROLLER_H
#define DMXCONTROLLER_H

#include "enttecdmxusb.h"
#include <thread>
#include <chrono>
#include <map>
#include <json/json.h>
#include <arpa/inet.h>
#include "OscController.h"
#include "ISensor.h"
#include "common.h"
#include "DominoState.h"

class DominoController;


//
// Class DominoController
// Main class
// 

class DominoController {
public:
    DominoController();
    virtual ~DominoController();

    bool Init();
    void Update();
    const DominoParams& GetDominoParams();

private:
  
    
    bool Send(uint8_t sensorIndex);
          
    EnttecDMXUSB* m_dmxInterface;
    bool m_initialized;
    OscController m_oscController;
    int m_diagSock;
    struct sockaddr_in m_diagAddr;

    DominoParams m_params;
    std::vector<DominoState> m_state;
    float m_continuousDelay;
    std::chrono::time_point<std::chrono::system_clock> m_lastContinuousSend;
    
    bool m_debugOutput;
        
    // Sensor Thread
    std::thread m_sensorThread;
    I2CBus m_bus;
    std::vector<ISensor*> m_sensor;
    int StartSensorThread();
    static void SensorThread( DominoController* parent );

    // Input Thread
    std::thread m_inputThread;
    int m_debugCmd;
    int StartInputThread();
    static void InputThread( DominoController* parent );
};

#endif /* DMXCONTROLLER_H */

