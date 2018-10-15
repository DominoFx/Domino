#ifndef DOMINOSTATE_H
#define DOMINOSTATE_H

#include "common.h"
#include "vec3.h"
#include "ISensor.h"

#include <json/json.h>
#include <math.h>


// 
// Defines
// 

#define CONFIG_SETUP_FILENAME "config_setup.json"
#define CONFIG_SEQUENCES_FILENAME "config_patterns.json"


// 
// Globals
// 

extern int signal_shutdown; // zero when running, non-zero for shutdown


//
// Class DominoParams
// Helper: Parameters specified in config.json
// 

struct DominoParams
{
    DominoParams();
    
    Json::Value jsonRoot;
    
    bool masterMode;
    bool workerMode;
    
    int dominoCount;
   
    float continuousFPS;
    float dmxFPS;

    // Hardware identification params

    std::string i2cDevice;
    std::string dmxDevice;
    
    // DMX config params
    
    bool dmxEnable;
    int dmxInterfaceID;
    std::string dmxInterface;  

    // Networking params
     
    int workerToMasterPort; // Config and Heartbeat messages from Worker->Master
    int masterToWorkerPort; // DMX messages from Master->Worker
    std::string workerToMasterAddress; // Testing only
    std::vector<std::string> masterToWorkerAddress; // Testing only

    int soundToWorkerPort; // (Unused) messages from Sound->Worker
    int workerToSoundPort; // Sensor messages Worker->Sound
    std::string workerToSoundAddress; // Sensor messages Worker->Sound

    int soundToMasterPort; // Config and Light trigger (Idle Mode) messages from Sound->Master
    int masterToSoundPort; // Heartbeat messages Master->Sound
    std::string masterToSoundAddress; // Config and Heartbeat messages Master->Sound

    int workerToDiagPort; // Sensor messages from Worker->Diagnostics
    std::string workerToDiagAddress; // Sensor messages from Worker->Diagnostics
    
    // Sensor config params

    int sensorAxis; // Axis value, 1 = x, 2 = y, 4 = z, 7 = xyz
    int sensorSmoothing;
    float sensorActivityBlend;
    float sensorActivityThresh;
    int sensorVelocityMultiplier;
    int sensorTapThresh;
    int sensorTapTimeLimit;
    int sensorTapTimeLatency;
    int sensorTapTimeWindow;

    // Multiplexer config params
    
    int multiplexerAddress1;
    int multiplexerAddress2;
    int multiplexerLanes;    

    // OSC communication tags
    
    std::string interactTag;
    std::string heartbeatTag;
    std::string configTag;
    std::string confirmTag;
    std::string idleTag;
    std::string diagTag;

    // External program params
    
    std::string commandLineSound;
    
    // Debug params
    
    int debugOscConfig0;
    int debugOscConfig1;
    std::vector<std::string> debugOscConfigIP;
    bool debugOutput;
};


//
// Class DominoState
// Helper: Sensor, sound and lighting data
// 

class DominoState
{
public:
    DominoState( DominoParams& params );

    int Init( int index );
    void Update( const SensorData* sensorData, Axis axis );
    
    int Index()       { return m_index; }
    float Angle()     { return m_angleHistory[m_historyIndex];}
    double AngleAvg() { return m_angleAvg;}
    float Veloc()     { return m_velocHistory[m_historyIndex];}
    double VelocAvg() { return m_velocAvg;}
    float ValUnit()   { return m_unitVal;}
    int ValDMX()      { return m_dmxVal;}
    bool Active()     { return (m_activated!=0); }
    bool Tap()        { return (m_tap!=0); }
    int  Err()        { return (m_err); }

    bool AccelSend()  { return (m_accelSend==0); }
    void AccelSent();
    
private:
    DominoParams& m_params;
    int m_index;
    std::vector<float> m_angleHistory;
    std::vector<float> m_velocHistory;
    int m_historyCount, m_historyIndex;
    int m_activated;  // set nonzero when speed exceeds a threshold
    int m_collided;   // set nonzero for short period after tap occurs
    int m_accelSend;  // set nonzero for short period after accel OSC message sent to sound mgr
    int m_accelFreq;  // how many should elapse between accel OSC messages sent to sound mgr
    int m_tap;        // set for single frame when activated and collided become true 
    int m_err;        // set for single frame if error reading from sensor 
    double m_angleAvgSlow;
    double m_angleAvg;
    double m_velocAvgSlow;
    double m_velocAvg;
    float m_unitVal;
    int m_dmxVal;
};


#endif /* DOMINOSTATE_H */

