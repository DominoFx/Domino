#ifndef DOMINOCONTROLLER_H
#define DOMINOCONTROLLER_H

#include "DominoState.h"
#include "DominoPlayer.h"
#include "enttecdmxusb.h"
#include "OscController.h"
#include "ISensor.h"
#include "common.h"
#include <thread>
#include <chrono>
#include <map>
#include <json/json.h>
#include <arpa/inet.h>

class DominoController;
class DominoSensorMaster;
class DominoSensorWorker;
class DominoPlayerMaster;
class DominoPlayerWorker;


//
// Class DominoController
// Main class
// 

class DominoController {
public:
    typedef std::chrono::time_point<std::chrono::system_clock> TimePoint;
    typedef std::chrono::duration<double> Duration;

    DominoController();
    virtual ~DominoController();

    bool Init( int argc, char** argv );
    void Update();
    
    DominoParams& GetParams();
    int GetDominoTotalCount();
    
    int RegisterMasterToWorkerCallback( const char* tag, OscCallback func, void* param );
    int RegisterWorkerToMasterCallback( const char* tag, OscCallback func, void* param );
    int RegisterSoundToMasterCallback( const char* tag, OscCallback func, void* param );
    int RegisterSoundToWorkerCallback( const char* tag, OscCallback func, void* param );

protected:
    // Master Mode or Worker Mode
    DominoSensorMaster* m_sensorMaster;
    DominoPlayerMaster* m_playerMaster;
    DominoSensorWorker* m_sensorWorker;
    DominoPlayerWorker* m_playerWorker;

    // OSC network communication listeners
    OscListener m_masterToWorkerListener;
    OscListener m_workerToMasterListener;
    OscListener m_soundToMasterListener;
    OscListener m_soundToWorkerListener;

    // Params
    DominoParams m_params;

    // Timing
    TimePoint m_lastUpdate;
    TimePoint m_lastConfig;
    float m_continuousDelay;

    // Diagnostics
    OscController m_diagServer;
    std::vector<float> m_diagData;
    bool m_debugOutput;
    int m_debugCmd;
        
    // Input handling
    std::thread m_inputThread;
    int StartInputThread();
    static void InputThread( DominoController* parent );
    
    // Friends
    friend class DominoSensorAgent;
    friend class DominoSensorMaster;
    friend class DominoSensorWorker;
    friend class DominoPlayerAgent;
    friend class DominoPlayerMaster;
    friend class DominoPlayerWorker;
};


//
// Class DominoControllerAgent
// Agent - Base class of Master and Worker
//

class DominoSensorAgent {
public:
    DominoSensorAgent( DominoController& context );

    virtual bool Init();

protected:
    DominoController& m_context;
    DominoParams& m_params;
};


// 
// Class DominoSensorMaster
// Master - Receives heartbeat from Workers, initiates Idle mode
//

class DominoSensorMaster : public DominoSensorAgent {
public:
    DominoSensorMaster( DominoController& context );
    virtual ~DominoSensorMaster();

    virtual bool Init();
    virtual void Update();
    
    int GetDominoTotalCount();
    int SendConfig();

private:
    bool m_initialized;

    // Configuration
    int configWorkerCount;
    char** configWorkerAddress;

    // OSC server, transmits heartbeat and config to OSC
    OscController m_soundServer;
    int m_soundConfigured;

    // Local methods
    void ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
    void ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
};


//
// Class DominoSensorWorker
// Worker - Receives sensor data, transmists heartbeat to Master
//

class DominoSensorWorker : public DominoSensorAgent {
public:
    DominoSensorWorker( DominoController& context );
    virtual ~DominoSensorWorker();

    virtual bool Init();
    virtual void Update( DmxFrame& dmxout );
    
    DominoState* GetState( int index );
    ISensor* GetSensor( int index );
    int SendConfig();

private:
    bool m_initialized;

    // Sound handling
    OscController m_soundServer;
    int m_soundConfigured;
    int m_multiplexerAvailable1;
    int m_multiplexerAvailable2;

    // Hearbeat handling
    OscController m_heartbeatServer;
    int m_heartbeatCountdown;
    int m_heartbeatTotal;
    float m_activity;

    // Sensor and DMX data
    std::vector<DominoState*> m_state;
    
    // Sensor Thread
    std::thread m_sensorThread;
    I2CBus m_bus;
    std::vector<ISensor*> m_sensor;

    // Local methods
    bool Update(uint8_t sensorIndex);
    int StartSensorThread();
    static void SensorThread( DominoSensorWorker* parent );
    void ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
};

#endif /* DMXCONTROLLER_H */

