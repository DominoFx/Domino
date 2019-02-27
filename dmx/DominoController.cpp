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



// 
// Class DominoController
// Main class - Container for Master and Worker agents
//

DominoController::DominoController():
  m_initialized(false)
, m_sensorMaster(nullptr)
, m_playerMaster(nullptr)
, m_sensorWorker(nullptr)
, m_playerWorker(nullptr)
, m_masterToWorkerListener("Master to Worker")
, m_workerToMasterListener("Worker to Master")
, m_soundToMasterListener("Sound to Master")
, m_soundToWorkerListener("Sound to Worker")
, m_diagToWorkerListener("Params Control")
, m_workerCount(0)
, m_updateCount(0)
, m_continuousDelay(1.0f / 30.0f) //Default to 30fps
, m_sleep(0)
, m_debugCmd(0)
, m_debugFlags(0)
, m_watchdogServer(nullptr)
{
}


DominoController::~DominoController()
{
    SAFE_DELETE( m_sensorMaster );
    SAFE_DELETE( m_playerMaster );
    SAFE_DELETE( m_sensorWorker );
    SAFE_DELETE( m_playerWorker );
    
    signal_shutdown = 1;
}

bool DominoController::Init( int argc, char** argv )
{
    //
    // Read parameters from disk
    //

    bool success = true;
    
    printf( "DominoFX: Reading config file %s... \n", CONFIG_SETUP_FILENAME );
    std::ifstream configFile( CONFIG_SETUP_FILENAME );

    Json::Value jsonRoot;
    configFile >> jsonRoot;

    if(jsonRoot.empty())
    {
        printf( "DominoFX: Unable to read config file \n" );
        success = false;
    }

    //
    // Process parameters
    //

    if( success )
    {
        success = Load( jsonRoot );
    }

    //
    // Master & Worker command-line switch
    //

    if( (argc>1) && success )
    {
        m_params.masterMode = false,  m_params.workerMode = false;
        for( int i=1; i<argc; i++ )
        {
            // TODO: Add more robust handling for command-line parameters
            if( (strcmp(argv[i],"-m")==0) || (strcmp(argv[i],"-M")==0) ) // Master mode only
            {
                printf( "DominoFX: Command-line param found, master mode...\n" );
                m_params.masterMode = true;
            }
            if( (strcmp(argv[i],"-w")==0) || (strcmp(argv[i],"-W")==0) ) // Worker mode only
            {
                printf( "DominoFX: Command-line param found, worker mode...\n" );
                m_params.workerMode = true;
            }
            if( (strcmp(argv[i],"-h")==0) || (strcmp(argv[i],"-h")==0) ) // Both
            {
                printf( "DominoFX: Command-line param found, master and worker hybrid mode...\n" );
                m_params.masterMode = true,  m_params.workerMode = true;
            }
        }
    }
    else if( success )
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
    // Master & Worker initialization
    //

    if( success )
    {
        m_workerCount = m_params.dominoModuleCount;
        
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
            m_watchdogServer = new OscController();
            m_watchdogServer->Open( m_params.workerToWatchdogAddress, m_params.workerToWatchdogPort );
            StartInputThread();
        }
        m_initialized = success;
    }
     
    //
    // Initialize sensor and player agents
    //
    
    m_lastUpdate = std::chrono::system_clock::now();
    m_lastConfig = std::chrono::system_clock::now();
     
    printf( "DominoFX: Initialization %s \n", (success?"done":"failed") );
    
    // Initialized
    return success;    
}

int DominoController::Load( Json::Value& jsonRoot )
{
    printf( "\nDominoFX: Loading all parameters... \n" );
    
    m_params.Init( jsonRoot );
        
    //
    // Basics
    //
    
    this->m_continuousDelay = 1.0f / m_params.continuousFPS;

    //
    // Params listener
    //

    // TODO: Maybe have separate ports for master and worker params listener,
    // or guarantee hybrid is used instead of running both as separate processes,
    // or better yet, prevent the port conflict from causing program to freeze
    if( m_params.workerMode )
    {
// TODO: Restore this, why is crashing on startup?
//        if( !m_diagToWorkerListener.Opened() || (m_diagToWorkerListener.Port()!=m_params.diagToWorkerPort) )
//            m_diagToWorkerListener.Open( m_params.diagToWorkerPort );
//        m_diagToWorkerListener.Register( m_params.paramTag.c_str(), ProcessParamChange, this );
    }
    else 
        printf( "\nDominoFX: NOTE, Params Control listener disabled in master mode... \n" );
    
    //
    // Master & Worker reload
    //
    
    if( m_sensorMaster!=nullptr ) m_sensorMaster->Reload();
    if( m_sensorWorker!=nullptr ) m_sensorWorker->Reload();
    if( m_playerMaster!=nullptr ) m_playerMaster->Reload();
    if( m_playerWorker!=nullptr ) m_playerWorker->Reload();
       
    return 1;
}

void DominoController::Update()
{
    TimePoint now = std::chrono::system_clock::now();
    Duration elapsedUpdate = now - m_lastUpdate;
    Duration elapsedConfig = now - m_lastConfig;
    
    bool sendUpdate = (elapsedUpdate.count() >= m_continuousDelay);
    if( sendUpdate )
    {
        // Send configuration messages occasionally
        bool sendConfig = (elapsedConfig.count() >= 2.0f); // two seconds
        if( sendConfig )
        {
            m_lastConfig = now;
        }

        // MASTER AGENT UPDATE
        bool isMaster = (m_sensorMaster!=nullptr);
        if( isMaster )
        {
            if( sendConfig )
                m_sensorMaster->SendConfig();
                
            m_sensorMaster->Update();
            m_playerMaster->Update();
        }
        
        // WORKER AGENT UPDATE
        bool isWorker = (m_sensorWorker!=nullptr);
        if( isWorker )
        {
            DmxFrame dmxout;
            ZERO_ARRAY( dmxout, GetDominoLocalCount(), 0 );
            
            if( sendConfig )
                m_sensorWorker->SendConfig();
                
            m_sensorWorker->Update( dmxout);
            m_playerWorker->Update( dmxout );
        }
        
        // WATCHDOG UPDATE
        bool isRemoteWorker = (isWorker && m_sensorWorker->IsRemote());
        if( sendConfig && (isMaster || isRemoteWorker) )
        {
            if( m_params.sleepEnable!=0 )
            {       
                m_sleep = (isMaster? m_sensorMaster->GetSleep() : m_sensorWorker->GetSleep());
                const char* params[] = {m_sleep? "sleep":"awake"};
                m_watchdogServer->Send( m_params.configTag, m_sleep, 1, params );
            }
        }

        // Print current FPS
        if( GetDebugVerbose() )
        {
            printf( "---------- ----------\n" );
            printf("FPS core: %f\n", (float)(1.0f/elapsedUpdate.count()) );
        }

        // End debug verbose status output
        SET_FLAG( m_debugFlags, debug_verbose, false ); // disable until next request
        
        m_updateCount += 1;
        m_lastUpdate = now;
    }
    else
    {
    }
}

DominoParams& DominoController::GetParams()
{
    return this->m_params;
}

int DominoController::GetDominoTotalCount()
{
    return m_params.dominoTotalCount;
}

int DominoController::GetDominoLocalCount()
{
    return (m_params.dominoTotalCount / m_params.dominoModuleCount);
}

int DominoController::GetUpdateCount()
{
    return m_updateCount;
}

int DominoController::GetWorkerCount()
{
    return m_workerCount;
}

OscController* DominoController::GetWorkerSock( int index )
{
    return (m_sensorMaster==nullptr? nullptr : m_sensorMaster->GetWorkerSock(index) );
}

int DominoController::GetSleep()
{
    return m_sleep;
}

int DominoController::GetDebugFlags()
{
    return m_debugFlags;
}

void DominoController::SetDebugFlags( int flags )
{
    m_debugFlags = flags;
}

bool DominoController::GetDebugVerbose()
{
    return ((m_debugFlags & debug_verbose)>0);
}

bool DominoController::GetDebugDiagServer()
{
    return ((m_debugFlags & debug_diag_server)>0);
}

bool DominoController::GetDebugDiagStream()
{
    return ((m_debugFlags & debug_diag_stream)>0);
}


int DominoController::RegisterMasterToWorkerCallback( const char* tag, OscCallback func, void* param )
{
    int result = 0;
    if( !m_masterToWorkerListener.Opened() || (m_masterToWorkerListener.Port()!=m_params.masterToWorkerPort) )
        m_masterToWorkerListener.Open( m_params.masterToWorkerPort );
    m_masterToWorkerListener.Unregister( func );
    m_masterToWorkerListener.Register( tag, func, param );
    return result;
}

int DominoController::RegisterWorkerToMasterCallback( const char* tag, OscCallback func, void* param )
{
    int result = 0;
    if( !m_workerToMasterListener.Opened() || (m_workerToMasterListener.Port()!=m_params.workerToMasterPort) )
        m_workerToMasterListener.Open( m_params.workerToMasterPort );
    m_workerToMasterListener.Unregister( func );
    m_workerToMasterListener.Register( tag, func, param );
    return result;
}

int DominoController::RegisterSoundToMasterCallback( const char* tag, OscCallback func, void* param )
{
    int result = 0;
    if( !m_soundToMasterListener.Opened() || (m_soundToMasterListener.Port()!=m_params.soundToMasterPort) )
        m_soundToMasterListener.Open( m_params.soundToMasterPort );
    m_soundToMasterListener.Unregister( func );
    m_soundToMasterListener.Register( tag, func, param );
    return result;
}

int DominoController::RegisterSoundToWorkerCallback( const char* tag, OscCallback func, void* param )
{
    int result = 0;
    if( !m_soundToWorkerListener.Opened() || (m_soundToWorkerListener.Port()!=m_params.soundToWorkerPort) )
        m_soundToWorkerListener.Open( m_params.soundToWorkerPort );
    m_soundToWorkerListener.Unregister( func );
    m_soundToWorkerListener.Register( tag, func, param );
    return result;
}

void DominoController::ProcessParamChange( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    Json::Value jsonRoot;
    
    Json::Reader reader;
    bool parseResult = reader.parse( argsData, jsonRoot );

    if( (!parseResult) || jsonRoot.isNull() )
        printf( "DominoFX: Error receiving params \"%s\" \n", argsData );
    else 
    {
        printf( "DominoFX: Received params [" );
        if( jsonRoot.isArray() )
            printf( "MALFORMATTED AS ARRAY" );
        if( !jsonRoot.isObject() )
            printf( "MALFORMATTED AS SINGLE \"%s\"", jsonRoot.asString().c_str() );
        else
        {
            std::vector<std::string> memberNames = jsonRoot.getMemberNames();
            if( memberNames.size()>0 )
                printf( "%s", memberNames[0].c_str() );
            for( int i=1; i<memberNames.size(); i++ )
                printf( ", %s", memberNames[i].c_str() );
        }
        printf("]\n");
        Load( jsonRoot );
    }
}

void DominoController::ProcessParamChange( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoController* parent = (DominoController*)param;
    parent->ProcessParamChange(argsData,argsSize,remoteEndpoint);
}

// 
// User input thread
// 

int DominoController::StartInputThread()
{
    int result = 0;
    
    // launch thread if not yet created
    if( m_inputThread.get_id() == std::thread::id() )
    {
        printf( "DominoFX: Launching Input thread...\n" );
        m_inputThread = std::thread(InputThreadBootstrap,this);
    }
    
    return result;
}

void DominoController::InputThread()
{
    printf( "DominoFX: Started Input thread...\n" );
    while( (!feof(stdin)) && (!signal_shutdown) )
    {
        int c = getc(stdin);
        printf( "DominoController::InputThread() received %i \n", (int)c );
        if( c=='\t' )
            TOGGLE_FLAG( m_debugFlags, debug_diag_stream );
        else
            SET_FLAG( m_debugFlags, debug_verbose, true );
        printf( "Debug verbose: %s, %i \n", (GetDebugVerbose()? "true" : "false"), (int)m_debugFlags );
    }
    printf( "DominoFX: Stopped Input thread\n" );
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
, m_workerAddress(nullptr)
, m_workerCount(0)
, m_workersConfigured(0)
, m_activity(0.0f)
, m_sleep(-1)
, m_heartbeatRampout(0)
, m_heartbeatCount(0)
, m_clock{0,0,0,0}
, m_soundServer(nullptr)
, m_soundConfigured(0)
{
}
DominoSensorMaster::~DominoSensorMaster()
{
    for( int i=0; i<m_workerCount; i++ )
    {
        SAFE_DELETE_ARRAY( m_workerAddress[i] );
    }
    
    for( int i=0; i<m_workerSock.size(); i++ )
    {
        SAFE_DELETE( m_workerSock[i] );
    }
    m_workerSock.clear();
}

bool DominoSensorMaster::Init()
{
    DominoSensorAgent::Init(); // base class method
    
    Reload();
    
    // Transmit config data to OSC
    m_context.RegisterSoundToMasterCallback( m_params.confirmTag.data(), ProcessConfirmSound, this );
    m_context.RegisterWorkerToMasterCallback( m_params.heartbeatTag.data(), ProcessHeartbeat, this );
    m_context.RegisterWorkerToMasterCallback( m_params.configTag.data(), ProcessConfigDomino, this );

    m_initialized = true;
    return m_initialized;
}

void DominoSensorMaster::Reload()
{
    if( (m_params.masterToSoundAddress.length()!=0) && (m_params.masterToSoundPort!=0) )
    {
        if( (m_soundServer==nullptr) || !m_soundServer->Opened() )
        {
            printf( "DominoFX: Connecting to sound server at %s : %i ...\n",
                m_params.masterToSoundAddress.data(), m_params.masterToSoundPort );
        }
        SAFE_DELETE(m_soundServer);
        m_soundServer = new OscController();
        m_soundServer->Open( m_params.masterToSoundAddress, m_params.masterToSoundPort );
    }
    
    {
        // Reallocate workers array
        for( int i=0; i<m_workerCount; i++ )
        {
            SAFE_DELETE_ARRAY( m_workerAddress[i] );
        }
        SAFE_DELETE_ARRAY( m_workerAddress );
        
        m_workerCount = m_context.GetWorkerCount();
        if( m_workerCount>0 )
        {
            m_workerAddress = new char*[m_workerCount];
        }
        ZERO_ARRAY( m_workerAddress, m_workerCount, nullptr );

        // Register workers
        //if( (i<m_workerAddress.size()) && (m_workerAddress[i]!=nullptr) )
        //    RegisterWorker( m_workerAddress[i], m_params.masterToWorkerPort, i );
    }    
}

void DominoSensorMaster::Update()
{
    for( int i=0; i<m_workerCount; i++ )
    {
        if( (m_workerSock.size()>i) && (m_workerSock[i]!=nullptr) ) // should always be true
        {
            float param = m_activity;
            m_workerSock[i]->Send( m_params.heartbeatTag, 1, (float*)(&param) );
        }
    }

    if( (m_heartbeatRampout<=0) )
    {
        m_activity -= 0.0003;
        m_activity = CLAMP( m_activity, 0,1 );
    }
    else
    {
        m_heartbeatRampout--;
    }       
}

OscController* DominoSensorMaster::GetWorkerSock( int index )
{
    if( (index<0) || (index>=m_workerSock.size()) )
        return nullptr;
    return m_workerSock[index];
}

bool DominoSensorMaster::RegisterWorker( const char* workerAddress, int workerPort, int workerIndex )
{
    //printf( "Master::RegisterWorker() %i at %s:%.4i ...\n", workerIndex, workerAddress, workerPort );

    if( workerIndex<0 )
        return false; // should not happen
            
    if( (workerAddress==nullptr) || (workerIndex>=m_workerCount) )
        return false; // can't register
        
    char* address = m_workerAddress[workerIndex];
    OscController* sock = (m_workerSock.size()<=workerIndex?  nullptr : m_workerSock[workerIndex]);
    if( (address!=nullptr) && (strcmp(address,workerAddress)==0) && (sock!=nullptr) && (sock->Opened()) )
        return true; // already registered
        
    // Print messages first time transmit socket is opened, or has closed for some reason
    bool readout = ( (sock==nullptr) || !(sock->Opened()) );
    if( readout )
        printf( "DominoFX: Registering worker %i at %s:%.4i ...\n", workerIndex, workerAddress, workerPort );

    if( address==nullptr )
    {
        m_workerAddress[workerIndex] = new char[IpEndpointName::ADDRESS_STRING_LENGTH];
        address = m_workerAddress[workerIndex];
    }
    
    if( sock==nullptr )
    {        
        while( m_workerSock.size()<=m_workerCount )
            m_workerSock.push_back(nullptr);            
        m_workerSock[workerIndex] = new OscController();
        sock = m_workerSock[workerIndex];
    }
    
    if( sock!=nullptr )
    {
        // Open the socket
        int port = m_params.masterToWorkerPort;        
        strcpy( address, workerAddress );
        m_workerSock[workerIndex]->Open( address, port );
        if( readout )
            printf( "DominoFX: Registered worker %i at %s:%.4i  ...\n", workerIndex, address, port );
    }
    else
    {
        if( readout )
            printf( "DominoFX: Failure registering worker %i at %s:%.4i ...\n", workerIndex, workerAddress, workerPort );
    }

    // Check whether all workers are now configured
    int i=0;
    for( ; (i<m_workerCount) && (m_workerSock[i]!=nullptr); i++) ; // empty statement
    m_workersConfigured = ((i>0) && (i==m_workerCount)? 1:0); // if loop broke early, or zero workers
}

int DominoSensorMaster::SendConfig()
{
    // Update clock and sleep even if no workers registered yet
    UpdateClock();
    UpdateSleep();

    // Send config to sound master
    // Send redundantly, to support reconnect if either program is restarted
    if( (m_workersConfigured>0) && (m_soundServer!=nullptr) )
    {
        if( (!m_soundConfigured) && m_soundServer->Opened() )
            printf( "DominoFX: Sending configuration init to sound master ...\n" );
        
        static std::string configTag("/configMaster");
        int param0 = m_params.soundIntrument;
        int param1 = -1; // hard coded because mother is always workerIndex -1
        
        if( m_params.dominoModuleCount>1 )
        {
            // Multiple modules; workers are running on other machines, send their addresses
            int paramIPCount = m_workerCount;
            const char** paramIP = (const char**)(m_workerAddress);        
            m_soundServer->Send( configTag, param0, param1, paramIPCount, paramIP );
        }
        else
        {
            // One module in hybrid mode; worker is running locally, send localhost
            const char* paramIP = "127.0.0.1";
            m_soundServer->Send( configTag, param0, param1, 1, &paramIP );
        }
    }

    // Send time and sleep values to domino workers
    if( m_workersConfigured>0 )
    {
        int params[] = {m_sleep, m_clock.weekday, m_clock.hour, m_clock.minute, m_clock.second};
        for( int i=0; i<m_workerCount; i++ )
        {
            if( (m_workerSock.size()>i) && (m_workerSock[i]!=nullptr) ) // should always be true
            {
                m_workerSock[i]->Send( m_params.clockTag, 5, params );
            }
        }
    }
}

int DominoSensorMaster::GetSleep()
{
    return m_sleep;
}

void DominoSensorMaster::UpdateClock()
{
    time_t generalTime = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    const tm localTime = *localtime( &generalTime );
    m_clock.weekday = localTime.tm_wday;
    m_clock.hour    = localTime.tm_hour;
    m_clock.minute  = localTime.tm_min;
    m_clock.second  = localTime.tm_sec;
    const char* weekday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    //printf( "UpdateCLock() m_clock = %s %i:%i:%i ...\n",
    //    weekday[m_clock.weekday], m_clock.hour, m_clock.minute, m_clock.second );
}

void DominoSensorMaster::UpdateSleep()
{
    int sleep = 0;
    // TODO: Convert to config parameters without making the config file too heavy, hmm
    switch( m_clock.weekday )
    {
    case 0: // sunday, 10am to 10pm
        sleep = ((m_clock.hour>=10) && (m_clock.hour<22)? 0:1); break;
    case 1: // monday-wednesday, 12pm to 10pm
    case 2:
    case 3:
        sleep = ((m_clock.hour>=12) && (m_clock.hour<22)? 0:1); break;
    case 4:
    case 5: // thursday-friday, 12pm to 11pm
        sleep = ((m_clock.hour>=12) && (m_clock.hour<23)? 0:1); break;
    case 6: // saturday, 10am-11pm
        sleep = ((m_clock.hour>=10) && (m_clock.hour<23)? 0:1); break;
    }
    
    if( m_sleep!=sleep )
    {
        const char* weekday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        printf( "DominoFX: Time is %s %i:%i:%i ...\n",
            weekday[m_clock.weekday], m_clock.hour, m_clock.minute, m_clock.second );
        if( (m_params.sleepEnable==0) && (sleep!=0) )
        {
            printf( "DominoFX: Ignoring SLEEP mode, disabled ...\n" );
        }
        printf( "DominoFX: Entering %s mode ...\n", ( (sleep && (m_params.sleepEnable!=0))? "SLEEP" : "AWAKE") );
        m_sleep = sleep;
    }    

    //printf("UpdateSleep() m_sleep = %i \n",(int)sleep);
}

void DominoSensorMaster::ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    m_activity += 0.05;
    m_activity = CLAMP( m_activity, 0,1 );
    
    if( m_heartbeatRampout<=0 )
    {    
        if( m_context.GetDebugDiagStream() )
        {
            //printf("heartbeat worker->master\n");
            printf("heartbeat master->sound\n");
        }
        
        if( m_soundServer!=nullptr )
        {
            float param = 1.0f;
            m_soundServer->Send(m_params.heartbeatTag, 1, (float*)(&param));
        }

        m_heartbeatRampout = (int)(0.2f * m_params.continuousFPS); // One-fifth of a second
        m_heartbeatCount++;
    }
}

void DominoSensorMaster::ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoSensorMaster* parent = (DominoSensorMaster*)param;
    parent->ProcessHeartbeat(argsData,argsSize,remoteEndpoint);
}

void DominoSensorMaster::ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    if( !m_soundConfigured )
    {
        printf( "DominoFX: Received configuration confirm from sound master ...\n" );
        m_soundConfigured = true;
    }
}

void DominoSensorMaster::ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoSensorMaster* parent = (DominoSensorMaster*)param;
    parent->ProcessConfirmSound(argsData,argsSize,remoteEndpoint);
}

void DominoSensorMaster::ProcessConfigDomino( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    int* args = (int*)argsData;
    int index = little_endian( args[0] );
    int port = remoteEndpoint.port;
    char address[IpEndpointName::ADDRESS_STRING_LENGTH];
    remoteEndpoint.AddressAsString( address );
    
    if( m_workersConfigured==0 )
    {
        printf( "DominoFX: Received configuration from worker %i, at %s:%i ...\n",
            (int)index,address,port );
    }
    RegisterWorker( address, port, index );
}

void DominoSensorMaster::ProcessConfigDomino( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoSensorMaster* parent = (DominoSensorMaster*)param;
    parent->ProcessConfigDomino(argsData,argsSize,remoteEndpoint);
}



// 
// Class DominoSensorWorker - Worker agent
// Public methods
//

DominoSensorWorker::DominoSensorWorker( DominoController& context ):
  DominoSensorAgent(context)
, m_initialized(false)
, m_diagServer(nullptr)
, m_masterBroadcast(nullptr)
, m_masterServer(nullptr)
, m_masterRemote(0)
, m_activity(1.0f)
, m_sleep(0)
, m_clock{0,0,0,0}
, m_soundServer(nullptr)
, m_soundConfigured(0)
, m_multiplexerAvailable{0,0}
, m_updateCount(0)
, m_dominoCount(0)
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
    m_diagData.resize(0);
}


bool DominoSensorWorker::Init()
{
    DominoSensorAgent::Init(); // base class method

    //
    // Sensor setup
    // Params related to sensor count and bus are not reloadable
    //

    m_dominoCount = m_context.GetDominoLocalCount();
    m_sensor.resize( m_dominoCount );
    m_state.resize( m_dominoCount );    

    printf( "DominoFX: Initializing I2C bus %s... \n", m_params.i2cDevice.c_str() );
    m_bus.Init( m_params.i2cDevice.c_str() );

    // Check if mux chips are available
    printf( "DominoFX: Scanning for multiplexers... \n" );
    int multiplexerErr[2] = {0,0};
    multiplexerErr[0] = m_bus.WriteGlobal( m_params.multiplexerAddress0, 0x00 );
    multiplexerErr[1] = m_bus.WriteGlobal( m_params.multiplexerAddress1, 0x00 );
    
    m_multiplexerAvailable[0] = (multiplexerErr[0]? 0:1);
    if( !m_multiplexerAvailable[0] )
    {
        printf( "DominoFX: Missing multiplexer 1 \n" );
    }

    m_multiplexerAvailable[1] = (multiplexerErr[1]? 0:1);
    if( !m_multiplexerAvailable[1] )
    {
        printf( "DominoFX: Missing multiplexer 2 \n" );
    }


    // Create sensor objects
    printf( "DominoFX: Scanning for %i sensors... \n", (int)(m_dominoCount) );
    for( int sensorIndex=0; sensorIndex<m_sensor.size(); sensorIndex++ )
    {
        SensorAddress sensorAddress;
        m_params.InitSensorAddress( &sensorAddress, sensorIndex );
        
        if( m_multiplexerAvailable[ sensorAddress.muxWhich ] )
        {
            printf( "DominoFX: Checking sensor %i, mux address %X, field %X ... \n",
                (int)sensorIndex, sensorAddress.muxAddress, sensorAddress.muxField );

            // auto-detect LIS3DH or BMA220 motion sensor
            if( LIS3DH::IsAvailable( &m_bus, &sensorAddress ) )
            {
                m_sensor[sensorIndex] = new LIS3DH(m_context);
            }
            else if( BMA220::IsAvailable( &m_bus, &sensorAddress ) )
            {
                m_sensor[sensorIndex] = new BMA220(m_params);
            }
            else
            {
                printf( "DominoFX: No motion sensor %i... \n", (int)sensorIndex );
                m_sensor[sensorIndex] = nullptr;
            }
        }
        else
        {
            printf( "DominoFX: Skipping motion sensor %i, missing multiplexer... \n", (int)sensorIndex );
            m_sensor[sensorIndex] = nullptr;
        }
    }
    
    // Create state objects
    for( int sensorIndex=0; sensorIndex<m_dominoCount; sensorIndex++ )
    {
        // Allow DominoState object even if no sensor available, will return default values
        //if( m_sensor[sensorIndex]!=nullptr )
        //{
            m_state[sensorIndex] = new DominoState(m_context);
            m_state[sensorIndex]->Init( sensorIndex );
        //}
        //else m_state[sensorIndex] = nullptr;
    }

    // Make OSC connections and reload other params
    Reload();    

    // Launch sensor thread
    StartSensorThread();
    
    m_initialized = true;

    return m_initialized;
}

void DominoSensorWorker::Reload()
{
    // Lock, only one thread may have access to m_sensor[]
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reload sensor hardware params
    for( int sensorIndex=0; sensorIndex<m_sensor.size(); sensorIndex++ )
    {
        if( m_sensor[sensorIndex]!=nullptr )
        {
            SensorAddress sensorAddress;
            m_params.InitSensorAddress( &sensorAddress, sensorIndex );
            m_sensor[sensorIndex]->Init( &m_bus, &sensorAddress );
        }
    }
     
    // Reload domino state params
    for( int sensorIndex=0; (sensorIndex<m_dominoCount) && (sensorIndex<m_state.size()); sensorIndex++ )
    {
        if( m_state[sensorIndex]!=nullptr )
        {
            m_state[sensorIndex]->Reload();
        }
    }
        
    // Restart transmitter connection to sound worker, sends sensor data
    if( (m_params.workerToSoundAddress.length()!=0) && (m_params.workerToSoundPort!=0) )
    {
        if( m_soundServer!=nullptr  )
        {
            printf( "DominoFX: Connecting to OSC at %s : %i ...\n",
                m_params.workerToSoundAddress.data(), m_params.workerToSoundPort );
        }
        SAFE_DELETE( m_soundServer );
        m_soundServer = new OscController();
        m_soundServer->Open( m_params.workerToSoundAddress, m_params.workerToSoundPort );
    }

    SAFE_DELETE_ARRAY( m_masterServer );
    SAFE_DELETE_ARRAY( m_masterBroadcast );
    if( m_params.dominoModuleCount>1 )
    {
        // Multiple modules; master is running on another machine, broadcast to it
        m_masterBroadcast = new OscBroadcaster();
        m_masterBroadcast->Open( m_params.workerToMasterPort );
        m_masterRemote = 0;
    }
    else
    {
        // One module in hybrid mode; master is running locally
        m_masterServer = new OscController();
        m_masterServer->Open( "127.0.0.1", m_params.workerToMasterPort );
        m_masterRemote = 0;
    }

    // Restart receiver sockets from domino master
    m_context.RegisterMasterToWorkerCallback( m_params.heartbeatTag.data(), ProcessHeartbeat, this );
    m_context.RegisterMasterToWorkerCallback( m_params.clockTag.data(), ProcessClock, this );

    // Restart receiver socket from sound worker
    // Received config confirm to OSC
    m_context.RegisterSoundToWorkerCallback( m_params.confirmTag.data(), ProcessConfirmSound, this );

    // Restart transmistter socket to diagnostic server
    printf( "DominoFX: DIAGNOSTICS SERVER SUPPORT DISABLED \n" );
//    printf( "!!!!!!!!!!********** DISABLE THIS **********!!!!!!!!!! \n" );
//    m_context.SetDebugFlags( DominoController::debug_diag_server );
//    if( m_context.GetDebugDiagServer() && (m_params.workerToDiagAddress.length()!=0) && (m_params.workerToDiagPort!=0) )
//    {
//        if( m_diagServer!=nullptr  )
//        {
//            printf( "DominoFX: Connecting to diagnostics at %s : %i \n",
//                m_params.workerToDiagAddress.data(), m_params.workerToDiagPort );
//        }
//        SAFE_DELETE( m_diagServer );
//        m_diagServer = new OscController();
//        m_diagServer->Open( m_params.workerToDiagAddress, m_params.workerToDiagPort );
//    }
}

void DominoSensorWorker::Update( DmxFrame& dmxout )
{
    int active = 0;
    int bit = 1;
    int oscParamIndex = 0;
    int oscParamCount = (1*m_context.GetDominoLocalCount());
    //int oscParamCount = (3*m_context.GetDominoLocalCount());
    float* oscParams = new float[oscParamCount];
    //FVec3_t accel;
    float angle;
    
    for (int sensorIndex=0; sensorIndex<m_dominoCount; sensorIndex++)
    {
        Update(sensorIndex);
     
        if( m_state[sensorIndex]!=nullptr )
        {
            // fade out when activity is low
            dmxout[sensorIndex] = LERP( m_params.dmxBaseline, m_state[sensorIndex]->ValDMX(), m_activity );
            
            if( m_state[sensorIndex]->Active() )
                active = (active | bit);
            //accel = m_state[sensorIndex]->Accel(); // does not work for virtual sensors, only real sensors
            angle = m_state[sensorIndex]->Angle();
        }
        else // no sensor, set acceleration to zero
        {
            angle = 0;
            //accel.x = accel.y = accel.z = 0;
        }
        
        //oscParams[oscParamIndex++] = m_state[sensorIndex]->VelocFiltered();
        //oscParams[oscParamIndex++] = m_state[sensorIndex]->AngleFiltered();
        
        oscParams[oscParamIndex++] = angle;
        //oscParams[oscParamIndex++] = accel.x;
        //oscParams[oscParamIndex++] = accel.y;
        //oscParams[oscParamIndex++] = accel.z;
        
        bit = (bit<<1);
    }

    // Send sound signal - every update, not only when the domino is active
    m_soundServer->Send(m_params.interactTag, oscParamCount, oscParams);
    delete[] oscParams;

    // Send diagnostic signal
    if( m_context.GetDebugDiagServer() )
    {
        int sensorCount = m_context.GetDominoLocalCount();
        int elementCount = 7;
        int preambleCount = 1;
        int diagDataCount = preambleCount + (elementCount * sensorCount);
        m_diagData.resize( diagDataCount );
        m_diagData[0] = (float)m_updateCount;
        for (int sensorIndex = 0; sensorIndex<sensorCount; sensorIndex++)
        {
            DominoState* state = GetState(sensorIndex);
            ISensor* sensor = GetSensor(sensorIndex);
            int a = preambleCount + (sensorIndex*elementCount);
            m_diagData[a+0] = (state==nullptr?  0 : state->AngleFiltered());
            m_diagData[a+1] = (sensor==nullptr? 0 : sensor->GetData()->acceleration.x);
            m_diagData[a+2] = (sensor==nullptr? 0 : sensor->GetData()->acceleration.y);
            m_diagData[a+3] = (state==nullptr?  0 : (state->Tap()? state->TapMagnitude() : (state->Active()? 1.0f : 0.0f)));
            m_diagData[a+4] = (float)(dmxout[sensorIndex]/255.0f);
            m_diagData[a+5] = (state==nullptr?  0 : (float)(state->Err()));
            m_diagData[a+6] = (state==nullptr?  0 : state->VelocFiltered() );
        }
        m_diagServer->Send( m_params.diagTag, diagDataCount, m_diagData.data() );
    }
    if( m_context.GetDebugDiagStream() )
    {
        bool anyActive=false;
        int sensorCount = m_context.GetDominoLocalCount();
        for (int sensorIndex = 0; sensorIndex<sensorCount; sensorIndex++)
        {   // first pass, check if any domino is active
            DominoState* state = GetState(sensorIndex);
            if( state->Active() )
                anyActive=true;
        }
        if( anyActive )
        {   // second pass, print out values if any domino active
            for (int sensorIndex = 0; sensorIndex<sensorCount; sensorIndex++)
            {
                DominoState* state = GetState(sensorIndex);
                float angle = state->Angle();
                if( state->Active() )
                    printf( (angle<0? " %.2f " : "  %.2f "), (int)sensorIndex, (float)angle );
                else
                    printf( " ----- " );
            }
            printf( "\n" );
        }
    }
    
    //
    // Heartbeat from domino worker to master
    // Sent only if a domino is actively moving
    //
    if( active )
    {
        if( m_masterServer!=nullptr )
        {
            //printf("heartbeat worker->master %i\n", m_heartbeatCount);
            m_masterServer->Send( m_params.heartbeatTag, 1, (float*)(&active) );
        }
    }
 
    m_updateCount += 1;
    // magic number 9240 is kinda random but has lots of factors, divides nicely by many other numbers
    //m_updateCount = (m_updateCount%9240); //keep updateCount from running away
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

bool DominoSensorWorker::IsRemote()
{
    return (m_masterRemote!=0);
}

int DominoSensorWorker::SendConfig()
{
    // Send config to sound worker
    // Send redundantly, to support reconnect if either program is restarted
    if( m_soundServer!=nullptr )
    {
        //if( (!m_soundConfigured) && m_soundServer->Opened() )
        //    printf( "DominoFX: Sending configuration init to sound worker ...\n" );
        
        static std::string configTag("/configWorker");
        int param0 = m_params.soundIntrument;
        int param1 = m_params.soundModuleIndex;
        int paramIPCount = 0; // hard coded because worker does not send IP addresses
        const char** paramIP = nullptr; // hard coded because worker does not send IP addresses
        
        m_soundServer->Send( configTag, param0, param1, paramIPCount, paramIP );
    }
    
    // Send config to domino master
    // Send redundantly, to support reconnect if either program is restarted
    if( m_masterBroadcast!=nullptr )
    {
        int params[] = {m_params.dominoModuleIndex};
        if( m_masterServer==nullptr )
            printf( "DominoFX: Sending configuration init to master (broadcast), index %i ...\n", params[0] );
        m_masterBroadcast->Send( m_params.configTag, 1, params );
    }
    else if( m_masterServer!=nullptr )
    {
        int params[] = {m_params.dominoModuleIndex};
        if( m_masterServer==nullptr )
            printf( "DominoFX: Sending configuration init to master (local), index %i ...\n", params[0] );
        m_masterServer->Send( m_params.configTag, 1, params );
    }
}

int DominoSensorWorker::GetSleep()
{
    return m_sleep;
}

// 
// Class DominoSensorWorker - Worker agent
// Private methods
//

bool DominoSensorWorker::Update(uint8_t sensorIndex)
{
    const SensorData* sensorData = nullptr;


    if( (sensorIndex>=0) && (sensorIndex < m_sensor.size()) &&
        (m_sensor[sensorIndex]!=nullptr) )
    {
        sensorData = m_sensor[sensorIndex]->GetData();        
        if( m_context.GetDebugVerbose() )
        {
            printf( "---------- ----------\n" );
            printf("Sensor %i\n", sensorIndex);
            m_sensor[sensorIndex]->DebugPrint();
        }
    }

    DominoState* state = m_state[sensorIndex];
    int posturePrev = state->Posture(); // posture including up, left, right, mid
    int postureUpPrev = state->PostureUp(); // posture up or down only
    
    // Update state
    // if sensor data is null, operation uses a virtual sensor
    state->Update( sensorData, (Axis)m_params.sensorAxis );
    
    int posture = state->Posture();
    int postureUp = state->PostureUp();
    // Notify nearby dominos when posture changes, for virtual sensor handling
    if( postureUp!=postureUpPrev ) // only consider major changes between up and down
    {
        float veloc = state->VelocPosture();
        if( (sensorIndex>0) && (m_state[sensorIndex-1]!=nullptr) )
            m_state[sensorIndex-1]->NotifyPostureRight( posture, posturePrev, veloc );
        if( (sensorIndex<(m_sensor.size()-1)) && (m_state[sensorIndex+1]!=nullptr) )
            m_state[sensorIndex+1]->NotifyPostureLeft( posture, posturePrev, veloc );
    }

    if( sensorData==nullptr )
    {
        if( m_context.GetDebugVerbose() )
        {
            printf( "ERROR: DominoControllerWorker::Send() no sensor at index %i\n", sensorIndex );
        }
        return 0;
    }

    if( m_context.GetDebugVerbose() )
    {
        printf( "---------- ----------\n" );
        printf( "  Angle:\t %.3f\n", (float)(state->Angle()) );
        printf( "  Angle Up:\t %.3f\n", (float)(state->AngleUp()) );
    }


    // Send tap signal (only if tap was detected)
    bool tap = state->Tap();
    if( tap )
    {
        float tapMagnitude = state->TapMagnitude();
        float oscParams[1] = { tapMagnitude };
        m_soundServer->Send(m_params.interactTag, sensorIndex, 1, oscParams);
    }

    // Send every update, not only when the domino is active
//    {
//        //float veloc = state->Veloc() * m_params.sensorVelocityMultiplier;
//        //float angle = state->Angle();
//        float velocFiltered = state->VelocFiltered();
//        float angleFiltered = state->AngleFiltered();
//        float oscParams[2] = { velocFiltered, angleFiltered };
//        m_soundServer->Send(m_params.interactTag, sensorIndex, 2, oscParams);
//    }
    
    return true;
}



// 
// Sensor Thread
// 

int DominoSensorWorker::StartSensorThread()
{
    int result = 0;

    if( m_sensorThread.get_id()==std::thread::id() )
    {
        printf( "DominoFX: Launching DominoSensor thread...\n" );
        m_sensorThread = std::thread(SensorThreadBootstrap,this);
    }
    return result;
}

void DominoSensorWorker::SensorThread()
{
    printf( "DominoFX: Started sensor thread...\n" );
    while( !signal_shutdown )
    {
        // Lock, only one thread may have access to m_sensor[]
        std::lock_guard<std::mutex> lock(m_mutex);
        
        int sensorCount = m_sensor.size();
        for( int sensorIndex = 0; sensorIndex < sensorCount; sensorIndex++ )
        {
            ISensor* sensor = m_sensor[sensorIndex];
            if( sensor != nullptr )
            {
                sensor->Sample( &m_bus );
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    printf( "DominoFX: Stopped sensor thread\n" );
}

void DominoSensorWorker::ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    float* args = (float*)argsData;
    m_activity = little_endian( args[0] );
}

void DominoSensorWorker::ProcessHeartbeat( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoSensorWorker* parent = (DominoSensorWorker*)param;
    parent->ProcessHeartbeat(argsData,argsSize,remoteEndpoint);
}

void DominoSensorWorker::ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    if( !m_soundConfigured )
    {
        printf( "DominoFX: Received configuration confirm from sound worker ...\n" );
        m_soundConfigured = true;
    }
}

void DominoSensorWorker::ProcessConfirmSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoSensorWorker* parent = (DominoSensorWorker*)param;
    parent->ProcessConfirmSound(argsData,argsSize,remoteEndpoint);
}

void DominoSensorWorker::ProcessClock( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    int* args = (int*)argsData;
    m_sleep = little_endian( args[0] );
    m_clock.weekday = little_endian( args[1] );
    m_clock.hour    = little_endian( args[2] );
    m_clock.minute  = little_endian( args[3] );
    m_clock.second  = little_endian( args[4] );
    
    // Clock message serves a secondary purpose, as a config confirmation...
    // Open a socket to transmit to master when it confirms with address and port number
    if( m_masterServer==nullptr  )
    {
        int port = remoteEndpoint.port;
        char address[IpEndpointName::ADDRESS_STRING_LENGTH];
        remoteEndpoint.AddressAsString(address);
        
        m_masterServer = new OscController();
        m_masterRemote = (strcmp(address,"127.0.0.1")==0? 0:1); // is the master localhost?
        
        printf( "DominoFX: Received configuration confirm from master %s:%i ...\n",
            address, port );
        const char* weekday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        printf( "DominoFX: Time from domino master is %s %i:%i:%i, %s ...\n",
            weekday[m_clock.weekday], m_clock.hour, m_clock.minute, m_clock.second, (m_sleep?"SLEEP":"AWAKE") );
            
        printf( "DominoFX: Connecting to master at %s : %i ...\n", address, m_params.workerToMasterPort );
        m_masterServer->Open( address, m_params.workerToMasterPort );
    }        
}

void DominoSensorWorker::ProcessClock( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoSensorWorker* parent = (DominoSensorWorker*)param;
    parent->ProcessClock(argsData,argsSize,remoteEndpoint);
}
