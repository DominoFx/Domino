#include "DominoController.h"
#include "DominoPlayer.h"
#include "BMA220.h"
#include "LISD3H.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <algorithm>
#include <thread>

// TODO: Convert to params value
int muxFieldOrder[] = {2,3,4,6,7}; // mux fields 0, 1 and 5 unused on the mux circuitboard

//
// Helpers
//

#define GET_JSON( j, p, _name_, _type_ )                                       \
if( j.isMember(#_name_) )                                                      \
{                                                                              \
    p._name_ = j[#_name_].as##_type_();                                        \
}                                                                              \
else                                                                           \
{                                                                              \
    printf( "DominoFX: No config entry for \"%s\" \n", #_name_ );              \
}

bool strcmp_s( const char* a, int aSizeMax, const char* b, int bSizeMax)
{
    int i = 0;
    for( i=0; (i<aSizeMax) && (a[i]!='\0') && (i<bSizeMax) && (b[i]!='\0'); i++ )
    {
        if( a[i]!=b[i] )
            break;
    }
    return ((a[i]=='\0') && (b[i]=='\0'));
}

int little_endian( int big )
{
    int little;
    char* dest = (char*)(&little);
    char* src  = (char*)(&big);
    dest[3] = src[0];
    dest[2] = src[1];
    dest[1] = src[2];
    dest[0] = src[3];
    return little;
}


// 
// Class DominoController
// Main class - Container for Master and Worker agents
//

DominoController::DominoController():
  m_sensorMaster(nullptr)
, m_playerMaster(nullptr)
, m_sensorWorker(nullptr)
, m_playerWorker(nullptr)
, m_masterToWorkerListener("Master to Worker")
, m_workerToMasterListener("Worker to Master")
, m_soundToMasterListener("Sound to Master")
, m_soundToWorkerListener("Sound to Worker")
, m_continuousDelay(1.0f / 30.0f) //Default to 30fps
, m_debugOutput(false)
, m_debugCmd(0)
{
}


DominoController::~DominoController()
{
    SAFE_DELETE( m_sensorMaster );
    SAFE_DELETE( m_playerMaster );
    SAFE_DELETE( m_sensorWorker );
    SAFE_DELETE( m_playerWorker );
    m_diagData.resize(0);
    
    signal_shutdown = 1;
}

bool DominoController::Init( int argc, char** argv )
{
    Json::Value& jsonRoot = m_params.jsonRoot;
    std::ifstream configfile( CONFIG_SETUP_FILENAME );
    configfile >> jsonRoot;
    
    if(jsonRoot.empty())
    {
        printf( "DominoFX: Unable to read config file \n" );
        return false;
    }
    printf( "DominoFX: Reading config file... \n" );

    //
    // Master & Worker mode
    //

    if( argc>1 )
    {
        m_params.masterMode = false,  m_params.workerMode = false;
        for( int i=1; i<argc; i++ )
        {
            // TODO: Add more robust handling for command-line parameters
            if( (strcmp(argv[i],"-m")==0) || (strcmp(argv[i],"-M")==0) ) // Master mode only
                m_params.masterMode = true;
            if( (strcmp(argv[i],"-w")==0) || (strcmp(argv[i],"-W")==0) ) // Worker mode only
                m_params.workerMode = true;
            if( (strcmp(argv[i],"-h")==0) || (strcmp(argv[i],"-h")==0) ) // Both
                m_params.masterMode = true,  m_params.workerMode = true;
        }
    }
    else
    {
        if(jsonRoot.isMember("masterMode"))
        {
            m_params.masterMode = jsonRoot["masterMode"].asBool();
            m_params.workerMode = (!m_params.masterMode); // by default, master is not a worker
        }
        else
        {
            m_params.masterMode = false;
            m_params.workerMode = true;  // default to worker mode
        }
    }
    
    //
    // Basics
    //
    
    GET_JSON( jsonRoot, m_params, dominoCount, Int );
                                  
    GET_JSON( jsonRoot, m_params, continuousFPS, Float );
    this->m_continuousDelay = 1.0f / m_params.continuousFPS;

    GET_JSON( jsonRoot, m_params, dmxFPS, Float );
    
    //
    // Device bus names
    //
    
    // I2C device name depends on Raspberry Pi model
    // to deterime the correct device, run the following at the command line:
    //   sudo i2cdetect -y 0
    //   sudo i2cdetect -y 1
    // if the first one works, use "/dev/i2c-0"
    // if the second one works, use "/dev/i2c-1"

    GET_JSON( jsonRoot, m_params, i2cDevice, String );

    GET_JSON( jsonRoot, m_params, dmxDevice, String );

    //
    // Networking params
    //
    
    GET_JSON( jsonRoot, m_params, masterToWorkerPort, Int );
    GET_JSON( jsonRoot, m_params, workerToMasterPort, Int );
    GET_JSON( jsonRoot, m_params, workerToMasterAddress, String );
    { // masterToWorkerAddress list
        int count = jsonRoot["masterToWorkerAddress"].size();
        for( int i=0; i<count; i++ )
        {
            Json::Value& jsonItem = jsonRoot["masterToWorkerAddress"][i];
            m_params.masterToWorkerAddress.push_back( jsonItem.asString() );
        }
    }
    
    GET_JSON( jsonRoot, m_params, workerToSoundPort, Int );
    GET_JSON( jsonRoot, m_params, workerToSoundAddress, String );
    GET_JSON( jsonRoot, m_params, soundToWorkerPort, Int );

    GET_JSON( jsonRoot, m_params, masterToSoundPort, Int );
    GET_JSON( jsonRoot, m_params, masterToSoundAddress, String );
    GET_JSON( jsonRoot, m_params, soundToMasterPort, Int );

    GET_JSON( jsonRoot, m_params, workerToDiagPort, Int );
    GET_JSON( jsonRoot, m_params, workerToDiagAddress, String );

    //
    // Sensor params
    //
    
    GET_JSON( jsonRoot, m_params, sensorAxis, Int );
    GET_JSON( jsonRoot, m_params, sensorSmoothing, Int );
    GET_JSON( jsonRoot, m_params, sensorActivityBlend, Float );
    GET_JSON( jsonRoot, m_params, sensorActivityThresh, Float );
    GET_JSON( jsonRoot, m_params, sensorVelocityMultiplier, Float );
    GET_JSON( jsonRoot, m_params, sensorTapThresh, Int );
    GET_JSON( jsonRoot, m_params, sensorTapTimeLimit, Int );
    GET_JSON( jsonRoot, m_params, sensorTapTimeLatency, Int );
    GET_JSON( jsonRoot, m_params, sensorTapTimeWindow, Int );
    
    GET_JSON( jsonRoot, m_params, multiplexerAddress1, Int );
    GET_JSON( jsonRoot, m_params, multiplexerAddress2, Int );
    GET_JSON( jsonRoot, m_params, multiplexerLanes, Int );

    //
    // DMX params  
    //

    GET_JSON( jsonRoot, m_params, dmxEnable, Bool );
    GET_JSON( jsonRoot, m_params, dmxInterface, String );
    
    m_params.dmxInterfaceID = DMX_USB_PRO; // default
    if(m_params.dmxInterface == "open")
    {
        m_params.dmxInterfaceID = OPEN_DMX_USB;
    }    

    //
    // OSC params
    //
    
    GET_JSON( jsonRoot, m_params, interactTag, String );
    GET_JSON( jsonRoot, m_params, heartbeatTag, String );
    GET_JSON( jsonRoot, m_params, configTag, String );
    GET_JSON( jsonRoot, m_params, confirmTag, String );
    GET_JSON( jsonRoot, m_params, idleTag, String );
    GET_JSON( jsonRoot, m_params, diagTag, String );

    //
    // External program params
    //

    GET_JSON( jsonRoot, m_params, commandLineSound, String );
    
    //
    // Diagnostic params
    //
    
    GET_JSON( jsonRoot, m_params, debugOutput, Bool );
    
    if(jsonRoot.isMember("debugOscConfig"))
    {
        int count = jsonRoot["debugOscConfig"].size();
        Json::Value& jsonItem0 = jsonRoot["debugOscConfig"][0];
        m_params.debugOscConfig0 = ((count<=0)?  0 : jsonItem0.asInt());
        Json::Value& jsonItem1 = jsonRoot["debugOscConfig"][1];
        m_params.debugOscConfig1 = ((count<=1)?  0 : jsonItem1.asInt());
        for( int i=2; i<count; i++ )
        {
            Json::Value& jsonItemIP = jsonRoot["debugOscConfig"][i];
            m_params.debugOscConfigIP.push_back( jsonItemIP.asString() );
        }
    }

    if( (m_params.workerToDiagAddress.length()!=0) && (m_params.workerToDiagPort!=0) )
    {
        printf( "DominoFX: Connecting to diagnostics at %s : %i \n",
            m_params.workerToDiagAddress.data(), m_params.workerToDiagPort );
        m_diagServer.Init( m_params.workerToDiagAddress, m_params.workerToDiagPort );
    }

    
    //
    // Initialize external programs
    //
    
    printf( "DominoFX: Launching external programs... \n" );
    if( m_params.commandLineSound.length()!=0 )
    {
        std::system( m_params.commandLineSound.data() );
    }
     
    //
    // Initialize sensor and player agents
    //
    
    m_lastUpdate = std::chrono::system_clock::now();
    m_lastConfig = std::chrono::system_clock::now();
    
   
    bool success = true;
    if( m_params.masterMode && m_params.workerMode )
    {
        printf( "DominoFX: Configured as MASTER and WORKER hybrid \n" );
    }
    if( m_params.masterMode && success )
    {
        printf( "DominoFX: Launching MASTER \n" );
        m_sensorMaster = new DominoSensorMaster( *this );
        m_playerMaster = new DominoPlayerMaster( *this );
        success = success && (m_sensorMaster->Init());
        success = success && (m_playerMaster->Init());
    }
    
    if( m_params.workerMode && success )
    {
        printf( "DominoFX: Launching WORKER \n" );
        m_sensorWorker = new DominoSensorWorker( *this );
        m_playerWorker = new DominoPlayerWorker( *this );
        success = success && (m_sensorWorker->Init());
        success = success && (m_playerWorker->Init());
    }

    if( !success )
    {
        SAFE_DELETE( m_sensorMaster );
        SAFE_DELETE( m_playerMaster );
        SAFE_DELETE( m_sensorWorker );
        SAFE_DELETE( m_playerWorker );
    }
    else
    {
        StartInputThread();
    }

    printf( "DominoFX: Initialization %s \n", (success?"done":"failed") );
    
    // Initialized
    return success;
}

void DominoController::Update()
{
    //printf("DominoController::Update() -> \n");
    
    TimePoint now = std::chrono::system_clock::now();
    Duration elapsedUpdate = now - m_lastUpdate;
    Duration elapsedConfig = now - m_lastConfig;
    
    bool sendUpdate = (elapsedUpdate.count() >= m_continuousDelay);
    if( sendUpdate )
    {
        bool sendConfig = (elapsedConfig.count() >= 1.0f);
        if( sendConfig )
        {
            m_lastConfig = now;
        }
        
        // Begin debug update if user requested
        if( m_debugCmd>0 )
        {
            // user input debugging command            
            m_debugOutput = true;
            m_debugCmd--;
        }


        // MASTER AGENT UPDATE
        if( m_sensorMaster!=nullptr )
        {
            if( sendConfig )
                m_sensorMaster->SendConfig();
                
            m_sensorMaster->Update();
            m_playerMaster->Update();
        }
        
        // WORKER AGENT UPDATE
        if( m_sensorWorker!=nullptr )
        {
            DmxFrame dmxout;
            ZERO_ARRAY( dmxout, m_params.dominoCount, 0 );
            
            if( sendConfig )
                m_sensorWorker->SendConfig();
                
            m_sensorWorker->Update( dmxout);
            m_playerWorker->Update( dmxout );
            
            // Send diagnostic signal
            int elementCount = 6;
            int diagDataCount = elementCount * m_params.dominoCount;
            m_diagData.resize( diagDataCount );
            for (int sensorIndex = 0; sensorIndex<m_params.dominoCount; sensorIndex++)
            {
                DominoState* state = m_sensorWorker->GetState(sensorIndex);
                ISensor* sensor = m_sensorWorker->GetSensor(sensorIndex);
                int a = sensorIndex*elementCount;
                m_diagData[a+0] = (state==nullptr?  0 : state->Angle());
                m_diagData[a+1] = (sensor==nullptr? 0 : sensor->GetData()->acceleration.x);
                m_diagData[a+2] = (sensor==nullptr? 0 : sensor->GetData()->acceleration.y);
                m_diagData[a+3] = (state==nullptr?  0 : (state->Tap()? 100.0f : (state->Active()? 1.0f : 0.0f)));
                m_diagData[a+4] = (float)(dmxout[sensorIndex]/255.0f);
                m_diagData[a+5] = (state==nullptr?  0 : (float)(state->Err()));
            }
            m_diagServer.Send( m_params.diagTag, diagDataCount, m_diagData.data() );
        }

        // Print current FPS
        if( m_debugOutput )
        {
            printf( "---------- ----------\n" );
            printf("FPS core: %f\n", (float)(1.0f/elapsedUpdate.count()) );
        }

        // End debug update
        m_debugOutput = false; // disable debugging until next update
        
        m_lastUpdate = now;
    }
    else
    {
    }
    //printf("DominoController::Update() <- \n");    
}

DominoParams& DominoController::GetParams()
{
    return this->m_params;
}

int DominoController::GetDominoTotalCount()
{
    if( m_sensorMaster!=nullptr )
        return m_sensorMaster->GetDominoTotalCount();
    return m_params.dominoCount; // default handling
}

int DominoController::RegisterMasterToWorkerCallback( const char* tag, OscCallback func, void* param )
{
    int result = 0;
    if( !m_masterToWorkerListener.Running() )
        m_masterToWorkerListener.Run( m_params.masterToWorkerPort );
    m_masterToWorkerListener.Register( tag, func, param );
    return result;
}

int DominoController::RegisterWorkerToMasterCallback( const char* tag, OscCallback func, void* param )
{
    int result = 0;
    if( !m_workerToMasterListener.Running() )
        m_workerToMasterListener.Run( m_params.workerToMasterPort );
    m_workerToMasterListener.Register( tag, func, param );
    return result;
}

int DominoController::RegisterSoundToMasterCallback( const char* tag, OscCallback func, void* param )
{
    int result = 0;
    if( !m_soundToMasterListener.Running() )
        m_soundToMasterListener.Run( m_params.soundToMasterPort );
    m_soundToMasterListener.Register( tag, func, param );
    return result;
}

int DominoController::RegisterSoundToWorkerCallback( const char* tag, OscCallback func, void* param )
{
    int result = 0;
    if( !m_soundToWorkerListener.Running() )
        m_soundToWorkerListener.Run( m_params.soundToWorkerPort );
    m_soundToWorkerListener.Register( tag, func, param );
    return result;
}

// 
// User input thread
// 

int DominoController::StartInputThread()
{
    int result = 0;
    
    printf( "DominoFX: Launching Input thread...\n" );
    m_inputThread = std::thread(InputThread,this);
    
    return result;
}

void DominoController::InputThread( DominoController* parent )
{
    printf( "DominoFX: Input thread started\n" );
    while( (!feof(stdin)) && (!signal_shutdown) )
    {
        int c = getc(stdin);
        parent->m_debugCmd++;
    }
    printf( "DominoFX: Input thread stopped\n" );
}


// 
// Class DominoSensorAgent
// Base class for Master and Worker agents
//


DominoSensorAgent::DominoSensorAgent( DominoController& context ):
  m_context(context)
, m_params(context.GetParams())
{
}

bool DominoSensorAgent::Init()
{
    return true;
}


// 
// Class DominoSensorMaster
// Master agent 
//

DominoSensorMaster::DominoSensorMaster( DominoController& context ):
  DominoSensorAgent(context)
, m_initialized(false)  
, m_soundConfigured(false)
, configWorkerCount(-1)
, configWorkerAddress(nullptr)
{
}
DominoSensorMaster::~DominoSensorMaster()
{
    for( int i=0; i<configWorkerCount; i++ )
        SAFE_DELETE_ARRAY( configWorkerAddress[i] );
    SAFE_DELETE_ARRAY( configWorkerAddress );
}

bool DominoSensorMaster::Init()
{
    DominoSensorAgent::Init(); // base class method
    
    if( (m_params.masterToSoundAddress.length()!=0) && (m_params.masterToSoundPort!=0) )
    {
        printf( "DominoFX: Connecting to sound server at %s : %i ...\n",
            m_params.masterToSoundAddress.data(), m_params.masterToSoundPort );
        m_soundServer.Init( m_params.masterToSoundAddress, m_params.masterToSoundPort );
    }
    
    // Transmit config data to OSC
    if( m_soundServer.IsInitialized() )
    {
        m_context.RegisterSoundToMasterCallback( m_params.confirmTag.data(), ProcessConfirmSound, this );
    }
    
    m_context.RegisterWorkerToMasterCallback( m_params.heartbeatTag.data(), ProcessHeartbeat, this );

    m_initialized = true;
    return m_initialized;
}

void DominoSensorMaster::Update()
{
}

int DominoSensorMaster::GetDominoTotalCount()
{
    if( configWorkerCount<0 )
        return -1; // ERROR: Requesting total workers but not yet initialized with workers!
    return (configWorkerCount * m_params.dominoCount);
}

int DominoSensorMaster::SendConfig()
{
    // Send config to sound master via OSC, until it has acknoledged with a confirm
    if( (!m_soundConfigured) && m_soundServer.IsInitialized() )
    {
        printf( "DominoFX: Sending configuration init to sound master ...\n" );
        
        // TODO: Implement startup handshake, don't use this temp solution
            
        // Populate array with IP addresses
        if( configWorkerCount<0 )
        {
            configWorkerCount = m_params.debugOscConfigIP.size();
            configWorkerAddress = new char*[configWorkerCount];
            for( int i=0; i<configWorkerCount; i++ )
            {
                int length = m_params.debugOscConfigIP[i].length();
                configWorkerAddress[i] = new char[1+length];
                if( length>0 )
                    memcpy( configWorkerAddress[i], m_params.debugOscConfigIP[i].data(), length*sizeof(char) );
                configWorkerAddress[i][length] = '\0';
            }
        }
    
        static std::string configTag("/configMaster");
        int param0 = m_params.debugOscConfig0;
        int param1 = -1; // hard coded because mother is always workerIndex -1
        int paramIPCount = configWorkerCount;
        const char** paramIP = (const char**)configWorkerAddress;
        
        m_soundServer.Send( configTag, param0, param1, paramIPCount, paramIP );
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void DominoSensorMaster::ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    float param = 1.0f;
    m_soundServer.Send(m_params.heartbeatTag, 1, &param);
}

void DominoSensorMaster::ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoSensorMaster* parent = (DominoSensorMaster*)param;
    parent->ProcessHeartbeat(argsData,argsSize,remoteEndpoint);
}

void DominoSensorMaster::ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    printf( "DominoFX: Received configuration confirm from sound master ...\n" );
    if( !m_soundConfigured )
    {
        m_soundConfigured = true;
    }
}

void DominoSensorMaster::ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoSensorMaster* parent = (DominoSensorMaster*)param;
    parent->ProcessConfirmSound(argsData,argsSize,remoteEndpoint);
}



// 
// Class DominoSensorWorker - Worker agent
// Public methods
//

DominoSensorWorker::DominoSensorWorker( DominoController& context ):
  DominoSensorAgent(context)
, m_initialized(false)
, m_soundConfigured(0)
, m_multiplexerAvailable1(0)
, m_multiplexerAvailable2(0)
, m_heartbeatCountdown(0)
, m_heartbeatTotal(0)
, m_activity(1.0f)
{
}

DominoSensorWorker::~DominoSensorWorker()
{
    for( int sensorIndex=0; sensorIndex<m_sensor.size(); sensorIndex++ )
    {
        SAFE_DELETE( m_sensor[sensorIndex] );
        SAFE_DELETE( m_state[sensorIndex] );
    }
    m_sensor.resize(0);
    m_state.resize(0);
}


bool DominoSensorWorker::Init()
{
    DominoSensorAgent::Init(); // base class method

    if( (m_params.workerToSoundAddress.length()!=0) && (m_params.workerToSoundPort!=0) )
    {
        printf( "DominoFX: Connecting to OSC at %s : %i ...\n",
            m_params.workerToSoundAddress.data(), m_params.workerToSoundPort );
        m_soundServer.Init( m_params.workerToSoundAddress, m_params.workerToSoundPort );
    }

    // TODO: Implement startup handshake with Master, determine master address and port
    std::string workerToMasterAddress = m_params.workerToMasterAddress;
    if( workerToMasterAddress.length()!=0 )
    {
        printf( "DominoFX: Connecting to master at %s : %i ...\n",
            workerToMasterAddress.data(), m_params.workerToMasterPort );
        m_heartbeatServer.Init( workerToMasterAddress, m_params.workerToMasterPort );
    }

    // Transmit config data to OSC
    if( m_soundServer.IsInitialized() )
    {
        m_context.RegisterSoundToWorkerCallback( m_params.confirmTag.data(), ProcessConfirmSound, this );
    }

    m_state.resize( m_params.dominoCount );    
    for( int sensorIndex=0; sensorIndex<m_params.dominoCount; sensorIndex++ )
    {
        m_state[sensorIndex] = new DominoState(m_params);
        m_state[sensorIndex]->Init( sensorIndex );
    }

    // Launch sensor input thread
    StartSensorThread();
    
    m_initialized = true;

    return m_initialized;
}

void DominoSensorWorker::Update( DmxFrame& dmxout )
{
    int active = 0;
    int bit = 1;
    for (int sensorIndex = 0; sensorIndex < m_params.dominoCount; ++sensorIndex)
    {
        Update(sensorIndex);
        
        // fade out when activity is low
        dmxout[sensorIndex] = LERP( 10, m_state[sensorIndex]->ValDMX(), m_activity );
        
        //if( sensorIndex!=0 ) // TODO: Remove this check, temporary because sensor zero is glitchy
        {
            if( m_state[sensorIndex]->Active() )
                active = (active | bit);
        }
        bit = (bit<<1);
    }
    
    // TODO: Implement improved blending between interactive and idle mode
    m_activity += (active? 0.05 : -0.0003);
    m_activity = CLAMP( m_activity, 0,1 );
    
    // Send heartbeat
    m_heartbeatCountdown--;
    if( (m_heartbeatCountdown<=0) && active )
    {
        printf("heartbeat worker->master %i\n", m_heartbeatTotal);
        m_heartbeatServer.Send( m_params.heartbeatTag, 1, (float*)(&active) );
        m_heartbeatCountdown = (int)(0.2f * m_params.continuousFPS); // One-fifth of a second
        m_heartbeatTotal++;
    }
}

DominoState* DominoSensorWorker::GetState( int sensorIndex )
{
    if( (sensorIndex<0) || (sensorIndex>=m_state.size()) )
        return nullptr;
    return m_state[sensorIndex];
}

ISensor* DominoSensorWorker::GetSensor( int sensorIndex )
{
    if( (sensorIndex<0) || (sensorIndex>=m_sensor.size()) )
        return nullptr;
    return m_sensor[sensorIndex];
}

int DominoSensorWorker::SendConfig()
{
    // Send config to sound worker via OSC, until it has acknoledged with a confirm
    if( (!m_soundConfigured) && m_soundServer.IsInitialized() )
    {
        printf( "DominoFX: Sending configuration init to sound worker ...\n" );
        
        // TODO: Implement startup handshake, don't use this temp solution
        
        static std::string configTag("/configWorker");
        int param0 = m_params.debugOscConfig0;
        int param1 = m_params.debugOscConfig1;
        int paramIPCount = 0; // hard coded because worker does not send IP addresses
        const char** paramIP = nullptr; // hard coded because worker does not send IP addresses
        
        m_soundServer.Send( configTag, param0, param1, paramIPCount, paramIP );
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// 
// Class DominoSensorWorker - Worker agent
// Private methods
//

bool DominoSensorWorker::Update(uint8_t sensorIndex)
{
    if( (sensorIndex<0) || (sensorIndex >= m_sensor.size()) ||
        (m_sensor[sensorIndex]==nullptr) )
    {
        if( m_context.m_debugOutput )
        {
            printf( "ERROR: DominoControllerWorker::Send() no sensor at index %i\n", sensorIndex );
        }
        return 0;
    }

    if( m_context.m_debugOutput )
    {
        printf( "---------- ----------\n" );
        printf("Sensor %i\n", sensorIndex);
        m_sensor[sensorIndex]->DebugPrint();
    }

    const SensorData* sensorData = m_sensor[sensorIndex]->GetData();

    if( m_context.m_debugOutput )
    {
        const FVec3_t& v = sensorData->acceleration;
        printf( "  Accel normalized:\t (%.3f,%.3f,%.3f)\n",
            (float)(v.x), (float)(v.y), (float)(v.z) );
    }

    DominoState* state = m_state[sensorIndex];
    state->Update( sensorData, (Axis)m_params.sensorAxis );


    // Send tap signal (only if tap was detected)
    bool tap = state->Tap();
    if( tap )
    {
        //if( sensorIndex!=0 ) // TODO: Remove this check, temporary because sensor zero is glitchy
        {
            m_soundServer.Send(m_params.interactTag, sensorIndex, 0, (float*)nullptr);
        }
    }

    // Send speed and angle signal
    if( state->Active() && state->AccelSend() )
    {
        //float veloc = state->Veloc() * m_params.sensorVelocityMultiplier;
        //float angle = state->Angle();
        float velocAvg = state->VelocAvg() * m_params.sensorVelocityMultiplier;
        float angleAvg = state->AngleAvg();
        float oscParams[2] = { velocAvg, angleAvg };
        m_soundServer.Send(m_params.interactTag, sensorIndex, 2, oscParams);
        state->AccelSent();
    }
    
    return true;
}


// 
// Sensor Thread
// 

int DominoSensorWorker::StartSensorThread()
{
    int result = 0;

    printf( "DominoFX: Initializing I2C bus... \n" );
    m_bus.Init( m_params.i2cDevice.data() );

    // Toggle the mux off (all of them)
    int multiplexerErr1 = 0, multiplexerErr2 = 0;
    multiplexerErr1 = m_bus.WriteGlobal( m_params.multiplexerAddress1, 0x00 );
    multiplexerErr2 = m_bus.WriteGlobal( m_params.multiplexerAddress2, 0x00 );
    
    m_multiplexerAvailable1 = (multiplexerErr1? 0:1);
    if( !m_multiplexerAvailable1 )
    {
        m_multiplexerAvailable1 = 0;
        printf( "DominoFX: Missing multiplexer 1 \n" );
    }

    m_multiplexerAvailable2 = (multiplexerErr2? 0:1);
    if( !m_multiplexerAvailable2 )
    {
        m_multiplexerAvailable2 = 0;
        printf( "DominoFX: Missing multiplexer 2 \n" );
    }

    m_sensor.resize( m_params.dominoCount );

    SensorParams sensorParams;
    sensorParams.tapThresh       = m_params.sensorTapThresh;
    sensorParams.tapTimeLimit    = m_params.sensorTapTimeLimit;
    sensorParams.tapTimeLatency  = m_params.sensorTapTimeLatency;
    sensorParams.tapTimeWindow   = m_params.sensorTapTimeWindow;

    // TODO: Move this to DominoSensorWorker::Init()
    
    printf( "DominoFX: Scanning for %i sensors... \n", (int)(m_params.dominoCount) );
    for( int sensorIndex=0; sensorIndex<m_sensor.size(); sensorIndex++ )
    {
        sensorParams.index = sensorIndex;
        
        if( (sensorIndex<m_params.multiplexerLanes) && m_multiplexerAvailable1 )
        {
            sensorParams.muxAddress = m_params.multiplexerAddress1;
            int muxIndex = sensorIndex;
            sensorParams.muxField = (1<<(muxFieldOrder[muxIndex]));
        }
        else if( m_multiplexerAvailable1 )
        {
            sensorParams.muxAddress = m_params.multiplexerAddress2;
            int muxIndex = (sensorIndex-m_params.multiplexerLanes);
            sensorParams.muxField = (1<<(muxFieldOrder[muxIndex]));
        }
        printf( "DominoFX: Sensor at index %i, mux address %X, field %X ... \n", (int)sensorIndex, sensorParams.muxAddress, sensorParams.muxField );

        // auto-detect LIS3DH or BMA220 motion sensor
        if( LIS3DH::IsAvailable( &m_bus, &sensorParams ) )
        {
            m_sensor[sensorIndex] = new LIS3DH();
            m_sensor[sensorIndex]->Init( &m_bus, &sensorParams );
        }
        else if( BMA220::IsAvailable( &m_bus, &sensorParams ) )
        {
            m_sensor[sensorIndex] = new BMA220();
            m_sensor[sensorIndex]->Init( &m_bus, &sensorParams );
        }
        else
        {
            printf( "DominoFX: No motion sensor at index %i... \n", (int)sensorIndex );
            m_sensor[sensorIndex] = nullptr;
        }
    }

    printf( "DominoFX: Launching DominoSensor thread...\n" );
    m_sensorThread = std::thread(SensorThread,this);
    return result;
}

void DominoSensorWorker::SensorThread( DominoSensorWorker* parent )
{
    printf( "DominoFX: Sensor thread started\n" );
    while( !signal_shutdown )
    {
        I2CBus& bus = parent->m_bus;

        int sensorCount = parent->m_sensor.size();
        for (int sensorIndex = 0; sensorIndex < sensorCount; ++sensorIndex)
        {
            ISensor* sensor = parent->m_sensor[sensorIndex];
            if( sensor != nullptr )
            {
                sensor->Sample( &bus );
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    printf( "DominoFX: Sensor thread stopped\n" );
}


void DominoSensorWorker::ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    printf( "DominoFX: Received configuration confirm from sound worker ...\n" );
    if( !m_soundConfigured )
    {
        m_soundConfigured = true;
    }
}

void DominoSensorWorker::ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoSensorWorker* parent = (DominoSensorWorker*)param;
    parent->ProcessConfirmSound(argsData,argsSize,remoteEndpoint);
}
