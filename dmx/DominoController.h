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
#include "enums.h"

class DominoController;


//
// Class DominoParams
// Helper: Parameters specified in config.json
// 

struct DominoParams
{
    DominoParams();

    std::string i2cDevice;
    std::string dmxDevice;

    bool dmxEnable;
    std::string dmxInterface;    

    std::string oscAddress;
    int oscPort;
    std::string oscTag;

    std::string diagAddress;    
    int diagPort;

    int multiplexerAddress1;
    int multiplexerAddress2;
    int multiplexerLanes;    
     
    Axis sensorAxis;// 1 = x, 2 = y, 4 = z, 7 = xyz
    int sensorCount;
    int sensorSmoothing;
    int sensorVelocityMultiplier;
    int sensorTapThresh;
    int sensorTapTimeLimit;
    int sensorTapTimeLatency;
    int sensorTapTimeWindow;

    float continuousFPS;

    bool debugOutput;
};


//
// Class DominoState
// Helper: Sensor, sound and lighting data
// 

class DominoState
{
public:
    DominoState();

    int Init( int historyCount );
    void Update(float angle, float speed );
    
    double Angle() { return m_angleAvg;}
    double Speed() { return m_speedAvg;}
    int DMXVal() { return m_dmxVal;}
    
private:

    std::vector<float> m_angleHistory;
    std::vector<float> m_speedHistory;
    int m_historyCount, m_historyIndex;
    double m_angleAvg;
    double m_speedAvg;
    int m_dmxVal;
};


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

