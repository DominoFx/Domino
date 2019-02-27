#ifndef DOMINOCONTROLLER_H
#define DOMINOCONTROLLER_H

#include "DominoParams.h"
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
    typedef struct { int weekday, hour, minute, second; } Clock;

    DominoController();
    virtual ~DominoController();

    bool  Init( int argc, char** argv );
    int   Load( Json::Value& jsonRoot );
    
    // Update, runs each frame
    void  Update();
    
    // Parameters
    DominoParams& GetParams();
    int   GetDominoTotalCount();
    int   GetDominoLocalCount();
    int   GetUpdateCount();

    // Worker registration, only in Master mode
    int GetWorkerCount();
    OscController* GetWorkerSock( int index );
    
    // Modes
    int   GetSleep();

    // Diagnostics
    enum { debug_verbose=1, debug_diag_server=2, debug_diag_stream=4 };
    int   GetDebugFlags();
    void  SetDebugFlags( int flags );
    bool  GetDebugVerbose();
    bool  GetDebugDiagServer();
    bool  GetDebugDiagStream();
    
    int   RegisterMasterToWorkerCallback( const char* tag, OscCallback func, void* param );
    int   RegisterWorkerToMasterCallback( const char* tag, OscCallback func, void* param );
    int   RegisterSoundToMasterCallback( const char* tag, OscCallback func, void* param );
    int   RegisterSoundToWorkerCallback( const char* tag, OscCallback func, void* param );

protected:
    bool m_initialized;

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
    OscListener m_diagToWorkerListener; // to update configuration params via OSC

    // Params
    DominoParams m_params;
    void ProcessParamChange( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );    
    static void ProcessParamChange( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
    int m_workerCount;

    // Timing
    int m_updateCount;
    TimePoint m_lastUpdate;
    TimePoint m_lastConfig;
    float m_continuousDelay;

    // Modes
    int m_sleep;

    // Diagnostics
    int m_debugCmd;
    int m_debugFlags;
    
    // Watchdog
    OscController* m_watchdogServer;
        
    // Input handling
    std::thread m_inputThread;
    int StartInputThread();
    void InputThread();
    static void InputThreadBootstrap( DominoController* parent ) {parent->InputThread();}
    
    // Friends
    friend class DominoSensorAgent;
    //friend class DominoSensorMaster;
    //friend class DominoSensorWorker;
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
    typedef DominoController::Clock Clock;
    DominoSensorAgent( DominoController& context );

    virtual bool Init();
    virtual void Reload() = 0;

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
    virtual void Reload();
    virtual void Update();
    
    // Worker registration
    int GetWorkerCount();
    OscController* GetWorkerSock( int index );
    bool RegisterWorker( const char* workerAddress, int workerPort, int workerIndex );

    // Config
    int SendConfig();
    int GetSleep();

    // Helpers
    void UpdateClock();
    void UpdateSleep();

private:
    bool m_initialized;

    // Workers
    char** m_workerAddress;
    std::vector<OscController*> m_workerSock;
    int m_workerCount;
    int m_workersConfigured;
    
    // Activity
    float m_activity;

    // Sleep
    int m_sleep;
    Clock m_clock;

    // Hearbeat
    int m_heartbeatRampout;
    int m_heartbeatCount;

    // Sound
    OscController* m_soundServer;
    int m_soundConfigured;

    // Local methods
    void ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
    void ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
    void ProcessConfigDomino( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessConfigDomino( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
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
    virtual void Reload();
    virtual void Update( DmxFrame& dmxout );
    
    DominoState* GetState( int index );
    ISensor* GetSensor( int index );
    bool IsRemote();
    
    // Config
    int  SendConfig();
    int  GetSleep();

private:
    bool m_initialized;

    // Diagnostics
    OscController* m_diagServer;
    std::vector<float> m_diagData;
    int m_diagMode;

    // Config handling
    OscBroadcaster* m_masterBroadcast;
    OscController* m_masterServer;
    int m_masterRemote;

    // Activity
    float m_activity;
    
    // Sleep
    int m_sleep;
    Clock m_clock;

    // Sound handling
    OscController* m_soundServer;
    int m_soundConfigured;
    int m_multiplexerAvailable[2];

    // Timing
    int m_updateCount;
    
    // Sensor and DMX data
    int m_dominoCount;
    std::vector<DominoState*> m_state;
    
    // Sensor Thread
    I2CBus m_bus;
    std::thread m_sensorThread;
    std::mutex m_mutex;
    std::vector<ISensor*> m_sensor;

    // Local methods
    bool Update(uint8_t sensorIndex);
    int StartSensorThread();
    void SensorThread();
    static void SensorThreadBootstrap( DominoSensorWorker* parent ) {parent->SensorThread();}
    void ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
    void ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
    void ProcessClock( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessClock( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
};

#endif /* DMXCONTROLLER_H */

