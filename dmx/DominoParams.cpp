#include "DominoParams.h"
#include "DominoState.h"
#include "ISensor.h"
#include "enttecdmxusb.h"

// 
// Globals
// 

int signal_shutdown = 0;


//
// Class DominoParams
// Helper: Parameters specified in config.json
//

void DominoCheckType( bool&, const char* type, const char* name )
{ if( strcmp(type,"bool")!=0 ) printf("DominoFX: Type mismatch, param %s not bool\n",name); }
void DominoCheckType( int&, const char* type, const char* name )
{ if( strcmp(type,"int")!=0 ) printf("DominoFX: Type mismatch, param %s not int\n",name); }
void DominoCheckType( float&, const char* type, const char* name )
{ if( strcmp(type,"float")!=0 ) printf("DominoFX: Type mismatch, param %s not float\n",name); }
void DominoCheckType( std::string&, const char* type, const char* name )
{ if( strcmp(type,"string")!=0 ) printf("DominoFX: Type mismatch, param %s not string\n",name); }

#define DOMINO_PARAM( _name, _type, _val  ) \
    this->_name = _val; \
    this->paramEntries.push_back( DominoParamEntry( #_name, &(this->_name), DominoParamEntry::type_##_type ) ); \
    DominoCheckType( this->_name, #_type, #_name );


DominoParams::DominoParams() :
  dmxInterfaceID(0)
{
    DOMINO_PARAM( masterMode,               bool,   false );
    DOMINO_PARAM( workerMode,               bool,   false );
    DOMINO_PARAM( dominoTotalCount,         int,    0 );
    DOMINO_PARAM( dominoModuleCount,        int,    0 );
    DOMINO_PARAM( dominoModuleIndex,        int,    0 );
    DOMINO_PARAM( soundIntrument,           int,    0 );
    DOMINO_PARAM( soundModuleIndex,         int,    0 );
    DOMINO_PARAM( continuousFPS,            float,  100 );
    DOMINO_PARAM( dmxFPS,                   float,  40 );
    DOMINO_PARAM( i2cDevice,                string, "/dev/i2c-1" );
    DOMINO_PARAM( dmxDevice,                string, "/dev/ttyUSB0" );
    DOMINO_PARAM( workerToMasterPort,       int,    8880 );
    DOMINO_PARAM( masterToWorkerPort,       int,    8881 );
    DOMINO_PARAM( workerToSoundPort,        int,    8000 );
    DOMINO_PARAM( workerToSoundAddress,     string, "127.0.0.1" );
    DOMINO_PARAM( soundToWorkerPort,        int,    8800 );
    DOMINO_PARAM( masterToSoundPort,        int,    8001 );
    DOMINO_PARAM( masterToSoundAddress,     string, "127.0.0.1" );
    DOMINO_PARAM( soundToMasterPort,        int,    8801 );
    DOMINO_PARAM( workerToWatchdogPort,     int,    8002 );
    DOMINO_PARAM( workerToWatchdogAddress,  string, "127.0.0.1" );
    DOMINO_PARAM( workerToDiagPort,         int,    12345 );
    DOMINO_PARAM( workerToDiagAddress,      string, "127.0.0.1" );
    DOMINO_PARAM( diagToWorkerPort,         int,    12345 );
    DOMINO_PARAM( sensorAxis,               int,    Axis::Z );
    DOMINO_PARAM( sensorFilterAmountL1,     float,  0.3 );
    DOMINO_PARAM( sensorFilterAmountL2,     float,  0.1 );
    DOMINO_PARAM( sensorIdleVelocityThresh, float,  0.07 );
    DOMINO_PARAM( sensorWinkVelocityThresh, float,  0.001 );
    DOMINO_PARAM( sensorTapCooldown,        float,  70 );
    DOMINO_PARAM( sensorTapMagnitudePow,    float,  1.0 );
    DOMINO_PARAM( sensorTapMagnitudeMul,    float,  20.0 );
    DOMINO_PARAM( sensorTapMagnitudeDelay,  float,  30 );
    DOMINO_PARAM( sensorTapLookbackDelay,   float,  20 );
    DOMINO_PARAM( sensorTapLookbackThresh,  float,  0.3 );
    DOMINO_PARAM( sensorVelocityMultiplier, float,  1.0f );
    DOMINO_PARAM( sensorHWQueue,            int,    5 );
    DOMINO_PARAM( sensorHWSamplesPerSec,    int,    200 );
    DOMINO_PARAM( sensorHWAccelRange,       int,    2 );
    DOMINO_PARAM( sensorHWTapThresh,        int,    80 );
    DOMINO_PARAM( sensorHWTapTimeLimit,     int,    10 );
    DOMINO_PARAM( sensorHWTapTimeLatency,   int,    20 );
    DOMINO_PARAM( sensorHWTapTimeWindow,    int,    255 );
    DOMINO_PARAM( multiplexerAddress0,      int,    0x70 ); //112
    DOMINO_PARAM( multiplexerAddress1,      int,    0x72 ); //114
    DOMINO_PARAM( multiplexerLanes,         int,    6 );
    DOMINO_PARAM( dmxEnable,                bool,   true );
    DOMINO_PARAM( dmxInterface,             string, "" );
    DOMINO_PARAM( dmxBaseline,              int,    30 );
    DOMINO_PARAM( dmxMultiplier,            float,  1.0 );
    DOMINO_PARAM( dmxPlayModeUpRange,       float,  0.15 );
    DOMINO_PARAM( dmxPlayModeUp,            int,    60 );
    DOMINO_PARAM( dmxPlayModeMax,           int,    150 );
    DOMINO_PARAM( dmxPlayModeMin,           int,    5 );
    DOMINO_PARAM( dmxPlayModeFallDelay,     int,    200 );
    DOMINO_PARAM( dmxPlayModeFlashDelay,    int,    100 );
    DOMINO_PARAM( dmxPlayModeFlashIn,       int,    12 );
    DOMINO_PARAM( dmxPlayModeFlashOut,      int,    100 );
    DOMINO_PARAM( interactTag,              string, "/domino" );
    DOMINO_PARAM( heartbeatTag,             string, "/moving" );
    DOMINO_PARAM( configTag,                string, "/config" );
    DOMINO_PARAM( confirmTag,               string, "/confirm" );
    DOMINO_PARAM( idleTag,                  string, "/idle"  );
    DOMINO_PARAM( diagTag,                  string, "/diag"  );
    DOMINO_PARAM( paramTag,                 string, "/param"  );
    DOMINO_PARAM( clockTag,                 string, "/clock"  );
    DOMINO_PARAM( sleepTag,                 string, "/sleep"  );
    DOMINO_PARAM( awakeTag,                 string, "/awake"  );
    DOMINO_PARAM( sleepEnable,              int,    1 );
}

void DominoParams::Init( Json::Value& jsonRoot_src )
{
    MergeParams( jsonRoot, jsonRoot_src );
    
    //
    // Basic type params - Read using the param entry list
    //
    
    for( int i=0; i<paramEntries.size(); i++ )
    {
        DominoParamEntry& entry = paramEntries[i];
        if(jsonRoot.isMember( entry.name.c_str() ) )
        {
            switch( entry.type )
            {
            case DominoParamEntry::type_bool:
                (*(bool*)(entry.ptr)) = jsonRoot[entry.name].asBool();
                //printf("DominoFX: param \"%s\" boolean -> %s\n",
                //    entry.name.c_str(), (jsonRoot[entry.name].asBool()? "true":"false") );
                break;
                
            case DominoParamEntry::type_int:
                (*(int*)(entry.ptr)) = jsonRoot[entry.name].asInt();
                //printf("DominoFX: param \"%s\" integer -> %i\n",
                //    entry.name.c_str(), jsonRoot[entry.name].asInt() );                
                break;
                
            case DominoParamEntry::type_float:
                (*(float*)(entry.ptr)) = jsonRoot[entry.name].asFloat();
                //printf("DominoFX: param \"%s\" float -> %.4f\n",
                //    entry.name.c_str(), jsonRoot[entry.name].asFloat() );                
                break;
                
            case DominoParamEntry::type_string:
                (*(std::string*)(entry.ptr)) = jsonRoot[entry.name].asString();
                //printf("DominoFX: param \"%s\" string -> \"%s\"\n",
                //    entry.name.c_str(), jsonRoot[entry.name].asString().c_str() );                
                break;
            }
        }
        else
        {
            printf("DominoFX: No config entry for param %s ...\n", entry.name.c_str() );
        }
    }
    
    //
    // Special type params - Read manually from json
    // 

    //
    // DMX params  
    //

    dmxInterfaceID = DMX_USB_PRO; // default
    if(dmxInterface == "open")
    {
        dmxInterfaceID = OPEN_DMX_USB;
    }    

}

// Helper: Merge entries from src to desc
void DominoParams::MergeParams( Json::Value& jsonRoot_dest, Json::Value& jsonRoot_src )
{
    bool isNullOrObject_dest =  (jsonRoot_dest.isNull() || jsonRoot_dest.isObject());
    bool isObject_src = jsonRoot_src.isObject();
    
    if( isNullOrObject_dest && isObject_src )
    {
        std::vector<std::string> keys = jsonRoot_src.getMemberNames();
        for( int i=0; i<keys.size(); i++ )
        {
            std::string& key   = keys[i];
            Json::Value& dest  = jsonRoot_dest[key];
            Json::Value& src   = jsonRoot_src[key];
            bool isNull_dest   = dest.isNull();
            bool isNull_src    = src.isNull();
            bool isObject_dest = dest.isObject();
            bool isObject_src  = src.isObject();
            bool isArray_dest  = dest.isArray();
            bool isArray_src   = src.isArray();
            
            if( isNull_src )
                jsonRoot_src.removeMember(key);
            else if( (!isNull_src) && (!isNull_dest) && (isObject_src) && (isObject_dest) ) 
                MergeParams( jsonRoot_dest[key], jsonRoot_src[key] );
            else
            {
                jsonRoot_dest[key] = jsonRoot_src[key];
                //printf("DominoFX: Param [%s] -> ", key.c_str());
                //if( isObject_src )
                //     printf("{object}\n");
                //else if( isArray_src )
                //     printf("{array}\n");
                //else printf("[%s]\n", jsonRoot_src[key].asString().c_str() );
            }
        }
    }
}

void DominoParams::InitSensorAddress( SensorAddress* address, int sensorIndex )
{
    address->index = sensorIndex;

    int muxIndex = 0;
    if( sensorIndex < this->multiplexerLanes )
    {
        address->muxWhich = 0;
        address->muxAddress = this->multiplexerAddress0;
        muxIndex = (sensorIndex);
    }
    else
    {
        address->muxWhich = 1;
        address->muxAddress = this->multiplexerAddress1;
        muxIndex = (sensorIndex - this->multiplexerLanes);
    }
    address->muxField = (1<<(muxFieldOrder[muxIndex]));
    
}
