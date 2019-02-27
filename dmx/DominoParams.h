#ifndef DOMINOPARAMS_H
#define DOMINOPARAMS_H

#include "common.h"
#include <json/json.h>


// 
// Defines
// 

#define CONFIG_SETUP_FILENAME "config_setup.json"
#define CONFIG_SEQUENCES_FILENAME "config_patterns.json"

// 
// Classes
// 

class SensorAddress;

// 
// Globals
// 

extern int signal_shutdown;

// TODO: Convert to params value
static const int muxFieldOrder[] = {2,3,4,6,7}; // mux fields 0, 1 and 5 unused on the mux circuitboard



//
// Class DominoParamEntry
//

struct DominoParamEntry
{
    enum {type_bool, type_int, type_float, type_string};
    int type;
    std::string name;
    void* ptr;
    DominoParamEntry( const char* _name, void* _ptr, int _type)
    { type=_type; name.assign(_name); ptr=_ptr; }
};

//
// Class DominoParams
// Helper: Parameters specified in config.json
// 

struct DominoParams
{
    DominoParams();
    
    void Init( Json::Value& jsonRoot_src );
    
    // Param source data
    Json::Value jsonRoot;
    
    // Param metadata
    // Includes basic type params, does not include vector params
    std::vector<DominoParamEntry> paramEntries;
    
    bool masterMode;
    bool workerMode;
    
    int dominoTotalCount;
    int dominoModuleCount;
    int dominoModuleIndex;

    int soundIntrument;
    int soundModuleIndex;
   
    float continuousFPS;
    float dmxFPS;

    // Hardware identification params
    
    // I2C device name depends on Raspberry Pi model
    // to deterime the correct device, run the following at the command line:
    //   sudo i2cdetect -y 0
    //   sudo i2cdetect -y 1
    // if the first one works, use "/dev/i2c-0"
    // if the second one works, use "/dev/i2c-1"
    std::string i2cDevice;
    
    std::string dmxDevice;
    
    // Networking params
     
    int workerToMasterPort; // Config and Heartbeat messages from Worker->Master
    int masterToWorkerPort; // DMX messages from Master->Worker

    int soundToWorkerPort; // (Unused) messages from Sound->Worker
    int workerToSoundPort; // Sensor messages Worker->Sound
    std::string workerToSoundAddress; // Sensor messages Worker->Sound

    int soundToMasterPort; // Config and Light trigger (Idle Mode) messages from Sound->Master
    int masterToSoundPort; // Heartbeat messages Master->Sound
    std::string masterToSoundAddress; // Config and Heartbeat messages Master->Sound

    int workerToWatchdogPort; // Sensor messages from Worker->Watchdog
    std::string workerToWatchdogAddress; // Sensor messages from Worker->Watchdog
    
    int workerToDiagPort; // Sensor messages from Worker->Diagnostics
    std::string workerToDiagAddress; // Sensor messages from Worker->Diagnostics

    int diagToWorkerPort; // Sensor messages from Diagnostics->Worker
    
    // Sensor config params

    int sensorAxis; // Axis value, 1 = x, 2 = y, 4 = z, 7 = xyz
    float sensorFilterAmountL1;
    float sensorFilterAmountL2;
    float sensorIdleVelocityThresh;
    float sensorWinkVelocityThresh;
    float sensorTapCooldown;
    float sensorTapMagnitudePow;
    float sensorTapMagnitudeMul;
    float sensorTapMagnitudeDelay;
    float sensorTapLookbackDelay;
    float sensorTapLookbackThresh;
    float sensorVelocityMultiplier;
    
    int sensorHWQueue;
    int sensorHWSamplesPerSec;
    int sensorHWAccelRange;
    int sensorHWTapThresh;
    int sensorHWTapTimeLimit;
    int sensorHWTapTimeLatency;
    int sensorHWTapTimeWindow;

    // Multiplexer config params
    
    int multiplexerAddress0;
    int multiplexerAddress1;
    int multiplexerLanes;    

    // DMX config params
    
    bool dmxEnable;
    int dmxInterfaceID; // virtual param, not in config file
    std::string dmxInterface;  
    int dmxBaseline;
    float dmxMultiplier;
    float dmxPlayModeUpRange;
    int dmxPlayModeUp;
    int dmxPlayModeMax;
    int dmxPlayModeMin;
    int dmxPlayModeFallDelay;
    int dmxPlayModeFlashDelay;
    int dmxPlayModeFlashIn;
    int dmxPlayModeFlashOut;

    // OSC communication tags
    
    std::string interactTag;
    std::string heartbeatTag;
	std::string configTag;
	std::string confirmTag;
	std::string idleTag;
	std::string diagTag;
	std::string paramTag;
	std::string clockTag;
	std::string sleepTag;
	std::string awakeTag;
    
    // Mode params
    
    int sleepEnable;
    
    // Utility methods
    void MergeParams( Json::Value& jsonRoot_dest, Json::Value& jsonRoot_src ); // helper
    void InitSensorAddress( SensorAddress* sensorAddress, int sensorIndex );
};

#endif /* DOMINOPARAMS_H */

