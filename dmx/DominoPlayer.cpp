#include "DominoPlayer.h"
#include "DominoController.h"
#include "enttecdmxusb.h"
#include "OscController.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <json/json.h>
#include <arpa/inet.h>
// STL headers
#include <map>
#include <algorithm>


//
// Class DominoPlayerFrame
//

DominoPlayerSequence::Frame::Frame():
  m_dmxTime(0)
, m_dmxChannelShift(0)
, m_dmxValBase(0)
{
}

//
// Class DominoPlayerSequence
//

DominoPlayerSequence::DominoPlayerSequence( DominoController& context ):
  m_context(context)
{
}

int DominoPlayerSequence::Load( Json::Value& jsonRoot )
{
    int result = 0;
    
    int frameCount = 0;
    double dmxTime = 0;
    int dmxChannelMin=0, dmxChannelMax=-1;        
    int readSec=0, readMillis=0, readChannel=0, readValue=0;

    // Erase old frames
    m_dmxFrames.clear();

    // Read basic params
    if(jsonRoot.isMember("name") )
    {
        m_name = jsonRoot["name"].asString();
    }
    if(jsonRoot.isMember("id") )
    {
        m_id = jsonRoot["id"].asInt();
    }        
    int dmxValBase=0;
    if(jsonRoot.isMember("baselineValueDMX") )
    {
        dmxValBase = jsonRoot["baselineValueDMX"].asInt();
    }
    
    // Create first blank frame
    m_dmxFrames.resize(++frameCount);
    Frame* frame = &(m_dmxFrames[frameCount-1]);
    frame->m_dmxTime = 0;
    frame->m_dmxChannelShift = 0;
    frame->m_dmxValBase = dmxValBase;

    // Read frames
    // First pass: Create map of channel->value pairs for each frame
    if( jsonRoot.isMember("sequence") )
    {
        Json::Value& jsonSequence = jsonRoot["sequence"];

        for( int i=0; i<jsonSequence.size(); i++ )
        {
            int indexA, indexB;
            
            Json::Value& jsonStep = jsonSequence[i];
            if(jsonStep.isMember("fade") )
            {
                sscanf( jsonStep["fade"].asString().data(), "%2is %3i", &readSec, &readMillis );
                dmxTime += (readSec + (readMillis/1000.0));
            }
            indexA = frameCount;
            m_dmxFrames.resize(++frameCount);
            m_dmxFrames[indexA].m_dmxTime = dmxTime;
            m_dmxFrames[indexA].m_dmxValBase = dmxValBase;

            if(jsonStep.isMember("sustain") )
            {
                sscanf( jsonStep["sustain"].asString().data(), "%2is %3i", &readSec, &readMillis );
                dmxTime += (readSec + (readMillis/1000.0));
            }
            indexB = frameCount;
            m_dmxFrames.resize(++frameCount);
            m_dmxFrames[indexB].m_dmxTime = dmxTime;
            m_dmxFrames[indexB].m_dmxValBase = dmxValBase;
                        
            if(jsonStep.isMember("dmx") )
            {
                Json::Value& jsonArray = jsonStep["dmx"];
                
                for( int j=0; j<jsonArray.size(); j++ )
                {
                    Json::Value& jsonItem = jsonArray[j];
                    Json::Value::Members jsonMembers = jsonItem.getMemberNames();
                    if( jsonMembers.size()>0 )
                    {
                        std::string& jsonName = jsonMembers[0];
                        sscanf( jsonName.data(), "%i", &readChannel );
                        dmxChannelMin = MIN(dmxChannelMin,readChannel);
                        dmxChannelMax = MAX(dmxChannelMax,readChannel);
                        readValue = jsonItem[jsonName].asInt();
                            
                        m_dmxFrames[indexA].m_dmxValMap[ readChannel ] = readValue;                        
                        m_dmxFrames[indexB].m_dmxValMap[ readChannel ] = readValue;
                    }
                }
            }
        } // for( int i=0; i<jsonRoot.size(); i++ )
    }

    m_duration = dmxTime;
    
    int dmxChannelShift = -dmxChannelMin;
    int dmxChannelRange = (dmxChannelMax-dmxChannelMin) + 1;
    int dmxChannelCount = (frameCount<=1? m_context.GetDominoTotalCount() : dmxChannelRange);
    
    // Second pass: Create array of channel->values for each frame, from the map values
    printf( "DominoFX: Loaded DMX pattern \"%s\" \n", m_name.data() );
    for( int i=0; i<frameCount; i++ )
    {
        printf( "  [" );
        Frame* frame = &(m_dmxFrames[i]);
        // Update properties
        frame->m_dmxChannelShift = dmxChannelShift;
        // Create blank array values, equal to the specified baseline
        frame->m_dmxValBuf.insert( frame->m_dmxValBuf.begin(), dmxChannelCount, dmxValBase );
        // Set array values, from the map values
        Frame::ValMap::iterator it = frame->m_dmxValMap.begin();
        for( ; it != frame->m_dmxValMap.end(); it++ )
        {
            int dmxChannel = it->first;
            int dmxVal = it->second;
            frame->m_dmxValBuf[dmxChannel+dmxChannelShift] = dmxVal;
        }
        for( int j=0; j<dmxChannelCount; j++ ) printf( " %3i", (int)(frame->m_dmxValBuf[j]) );
        printf( " ] frame %i, shift %i, time %.2f \n",
            (int)i, (int)dmxChannelShift, (float)(frame->m_dmxTime) );
    }
}

int DominoPlayerSequence::NumFrames()
{
    return m_dmxFrames.size();
}

int DominoPlayerSequence::FindFrame( float dmxTime )
{
    int result = 0;
    for( int i=1; (i<m_dmxFrames.size()) && (m_dmxFrames[i].m_dmxTime < dmxTime); i++ )
    {
        result = i;
    }
    return result;
}

DominoPlayerSequence::Frame* DominoPlayerSequence::GetFrame( int index )
{
    if( (index<0) || (index>=m_dmxFrames.size()) )
        return nullptr;
    return &(m_dmxFrames[index]);
}


//
// Class DominoPlayerAgent 
// Agent - Base class of Master and Worker
// 

DominoPlayerAgent::DominoPlayerAgent( DominoController& context ):
  m_context(context)
, m_params(context.GetParams())
{
}
DominoPlayerAgent::~DominoPlayerAgent()
{
}


//
// Class DominoPlayerMaster::Task
//

void DominoPlayerMaster::Task::Run( DominoPlayerSequence* sequence, int dmxCenterpoint )
{
    m_sequence = sequence;
    m_dmxCenterpoint = dmxCenterpoint;
    m_start = std::chrono::system_clock::now();
}
bool DominoPlayerMaster::Task::Running()
{
    TimePoint now = std::chrono::system_clock::now();
    Duration elapsed = now - m_start;
    return (elapsed.count() < m_sequence->GetDuration());
}
void DominoPlayerMaster::Task::Update( DmxFrame& dmxout )
{
    // Find the playback frame index for current time point
    TimePoint now = std::chrono::system_clock::now();
    Duration elapsed = now - m_start;
    double dmxTime = elapsed.count();
    int frameIndex = m_sequence->FindFrame(dmxTime);
    bool frameLast = ((frameIndex+1) >= m_sequence->NumFrames());
    // Fetch playback frames before and after current time point
    Frame* frameA = m_sequence->GetFrame(frameIndex);
    Frame* frameB = (frameLast?  nullptr : m_sequence->GetFrame(frameIndex+1));
    // Special case: only one frame available for current time point
    if( frameB==nullptr )
    {
        for( int i=0; i<NB_CANAUX_MAX; i++ )
        {
            float v = frameA->GetDmxVal(i,m_dmxCenterpoint);
            dmxout[i] = MAX( dmxout[i], v );
        }
    }
    else
    {   // Lerp frames before and after current time point
        float timeA = frameA->GetTime(), timeB = frameB->GetTime();
        float span = timeB - timeA;
        float u = (span<=0? 0 : ((dmxTime-timeA) / span)); // lerp value
        for( int i=0; i<NB_CANAUX_MAX; i++ )
        {
            int valA = frameA->GetDmxVal(i,m_dmxCenterpoint);
            int valB = frameB->GetDmxVal(i,m_dmxCenterpoint);
            float v = ((1.0f-u)*valA) + (u*valB);
            dmxout[i] = MAX( dmxout[i], v );
        }
    }
}

//
// Class DominoPlayerMaster
// Master - Runs idle mode, transmits DMX frames to workers
//

DominoPlayerMaster::DominoPlayerMaster( DominoController& context ):
  DominoPlayerAgent(context)
, m_initialized(false)
//, m_oscListener("Player master DMX")
{
    memset( m_dmxout, 0, sizeof(DmxFrame) );
}
DominoPlayerMaster::~DominoPlayerMaster()
{
    for( int workerIndex=0; workerIndex<m_workers.size(); workerIndex++ )
    {
        SAFE_DELETE( m_workers[workerIndex] );
    }
    m_workers.clear();
    
    SequencesMap::iterator it = m_sequencesMap.begin();
    for( ; it!=m_sequencesMap.end(); it++ )
    {
        SAFE_DELETE( it->second );
    }
    m_sequencesMap.clear();
}
 
bool DominoPlayerMaster::Init()
{
    DominoPlayerAgent::Init(); // base class method
    
    this->m_dmxDelay = (1.0f / m_params.dmxFPS);
    this->m_dmxDelay = (0.5f * this->m_dmxDelay); // update at double rate for higher assurance
    
    // Load DMX sequences from disk
    LoadSequences(CONFIG_SEQUENCES_FILENAME);
    
    {   // Register workers
        // TODO: Implement startup sequence, don't use this temp solution
        for( int i=0; i<m_params.masterToWorkerAddress.size(); i++ )
            RegisterWorker( m_params.masterToWorkerAddress[i], m_params.masterToWorkerPort, i ); // TODO: DELETEME, Testing only
    }
    
    m_context.RegisterSoundToMasterCallback( m_params.idleTag.data(), ProcessIdleSound, this );
    
    //m_oscPort = m_params.oscOutPort;
    //StartOscThread();
    StartDmxThread();
    
    m_initialized = true;
    return m_initialized;
}

void DominoPlayerMaster::Update()
{
    // Play SuperCollider commands
    // - Each command contains a NoteID
    // - Each NoteID corresponds to a recorded file
    // - Each SuperCollider command has a NodeID, position, duration and attenuation

    // Grab buffered commands from OscController / OscPacketListener
    // Iterate through them one by one
    std::lock_guard<std::mutex> lock(m_mutex);
    memset( m_dmxout, 0, sizeof(DmxFrame) );
    
    for( int i=(m_queue.size()-1); i>=0; i-- )
    {
        Task* task = &(m_queue[i]);
        if( task->Running() )
        {
            task->Update(m_dmxout);
        }
        else
        {   // sequence is finished, erase it
            m_queue.erase( m_queue.begin() + i );
        }
    }
}

bool DominoPlayerMaster::RegisterWorker( const std::string& workerAddress, int workerPort, int workerIndex )
{
    printf( "DominoFX: Registering worker #%i at %s:%.4i ...\n", workerIndex, workerAddress.data(), workerPort );
    
    OscController* worker = new OscController();

    if( worker!=nullptr )
    {        
        while( m_workers.size()<=workerIndex )
            m_workers.push_back(nullptr);
        SAFE_DELETE( m_workers[workerIndex] );
        m_workers[workerIndex] = worker;
        // Open the socket
        worker->Init( workerAddress, workerPort );
        printf( "DominoFX: Registered worker #%i at %s:%.4i  ...\n", workerIndex, workerAddress.data(), workerPort );
    }
    else
    {
        printf( "DominoFX: Failure registering worker #%i at %s:%.4i ...\n", workerIndex, workerAddress.data(), workerPort );
    }
}


bool DominoPlayerMaster::LoadSequences( const char* filenameConfig )
{
    printf( "DominoFX: Loading DMX pattern config %s ...\n", filenameConfig );
    
    Json::Value jsonRoot;
    std::ifstream configfile(filenameConfig);
    configfile >> jsonRoot;
    
    std::string folder;
    if(jsonRoot.isMember("folder"))
    {
        folder = jsonRoot["folder"].asString();
    }

    if(jsonRoot.isMember("files"))
    {
        int count = jsonRoot["files"].size();
        for( int i=0; i<count; i++ )
        {
            std::string file = jsonRoot["files"][i].asString();
            std::string path = folder + file;
            LoadSequence( path.data() );
        }
    }
    
    return true;
}

bool DominoPlayerMaster::LoadSequence( const char* filename )
{
    Json::Value* sequenceJson = new Json::Value();
    m_sequencesJson.push_back( sequenceJson );
    
    Json::Value& jsonRoot = *sequenceJson;
    std::ifstream file(filename);
    file >> jsonRoot;

    std::string name;
    if(jsonRoot.isMember("name"))
    {
        name = jsonRoot["name"].asString();
    }
    
    printf( "DominoFX: Loading DMX pattern \"%s\" ...\n", name.data() );
    DominoPlayerSequence* sequence = new DominoPlayerSequence(m_context);
    
    sequence->Load(jsonRoot);
    m_sequencesMap[sequence->GetId()] = sequence;
}

bool DominoPlayerMaster::PlaySequence( int id, int dmxCenterpoint )
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Sequence* sequence = FindSequence(id);
    if( sequence!=nullptr )
    {
        int index = m_queue.size();
        m_queue.resize( index+1 );
        Task* task = &(m_queue[index]);
        task->Run( sequence, dmxCenterpoint );
    }
    else printf( "DominoPlayerMaster::PlaySequence() no sequence found %i \n", id );
}

DominoPlayerMaster::Sequence* DominoPlayerMaster::FindSequence( int id )
{
    SequencesMap::iterator it = m_sequencesMap.find( id );
    if( it!=m_sequencesMap.end() )
    {
        return it->second;
    }
    return nullptr;
}


void DominoPlayerMaster::ProcessIdleSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    int numItems = 2;
    int numItemsAvail = (argsSize/sizeof(int));
    if( numItemsAvail>=numItems )
    {
        const int* item = (const int*)argsData;
        int id = little_endian( item[1] );
        int dmxCenterpoint = little_endian( item[0] );
        printf( "DominoFX: Playing pattern : id %i at domino %i...\n", id, dmxCenterpoint );
        PlaySequence( id, dmxCenterpoint );
    }
    else
    {
        // TODO: Re-enable this.  Master currently gets a lot of #bundle messages, not sure why
        //printf( "DominoPlayerMaster::ProcessOscPacket() unrecognized packet %s \n", data ); 
    }
}

void DominoPlayerMaster::ProcessIdleSound( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoPlayerMaster* parent = (DominoPlayerMaster*)param;
    parent->ProcessIdleSound(argsData,argsSize,remoteEndpoint);
}


//
// Class DominoPlayerMaster::DmxThread
// Plays DMX sequences, transmits DMX frames to workers

int DominoPlayerMaster::StartDmxThread()
{
    int result = 0;
    
    printf( "DominoFX: Launching player master DMX sender thread...\n" );
    m_dmxThread = std::thread(DmxThreadBootstrap,this);
    
    return result;    
}

void DominoPlayerMaster::DmxThread()
{
    printf( "DominoFX: Player master DMX sender thread started ...\n" );
    while( !signal_shutdown )
    {
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = now - m_lastUpdate;
        
        if( elapsed.count() >= m_dmxDelay )
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for( int i=0; i<m_workers.size(); i++ )
            {
                OscController* worker = m_workers[i];
                if( worker!=nullptr )
                {
                    int dmxOffset = (i * m_params.dominoCount);
                    int* buf = &(m_dmxout[dmxOffset]);
                    worker->Send( m_params.idleTag, m_params.dominoCount, buf );
                }
            }

            m_lastUpdate = now;
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }
    }
    printf( "DominoFX: Player master DMX sender thread stopped ...\n" );    
}


//
// Class DominoPlayerWorker
// Worker - Runs interactive mode, receives DMX frames from Master
//

DominoPlayerWorker::DominoPlayerWorker( DominoController& context ):
  DominoPlayerAgent(context)
, m_initialized(false)
, m_dmxInterface(nullptr)
, m_dmxFps(0)
{
    memset( m_dmxout, 0, sizeof(DmxFrame) );
}
DominoPlayerWorker::~DominoPlayerWorker()
{
    SAFE_DELETE( m_dmxInterface );
}

bool DominoPlayerWorker::Init()
{
    printf( "DominoFX: Initializing worker ... \n" );
        
    DominoPlayerAgent::Init(); // base class method
    
    m_dmxInterface = new EnttecDMXUSB( (EnttecInterfaces)m_params.dmxInterfaceID, m_params.dmxDevice );

    m_context.RegisterMasterToWorkerCallback( m_params.idleTag.data(), ProcessIdleDmx, this );
    
    if((m_dmxInterface && m_dmxInterface->IsAvailable()))
    {
        printf( "DominoFX: Found USB->DMX controller... \n" );

        StartOutputDmxThread();
        m_initialized = true;
    }
    else
    {
         printf( "DominoFX: No USB->DMX controller found ...\n" );
         m_initialized = true;
    }
        
    return m_initialized;
}

void DominoPlayerWorker::Update( DmxFrame& dmxout )
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for( int i=0; i<m_params.dominoCount; i++ )
    {
        m_dmxoutSensor[i] = dmxout[i];
        m_dmxout[i] = MAX( m_dmxoutSensor[i], m_dmxoutPlayer[i] );
        dmxout[i] = m_dmxout[i];
    }
     // Print current FPS
    if( m_context.m_debugOutput )
    {
        printf( "---------- ----------\n" );
        printf( "FPS DMX: %f\n", (float)(GetFPS()) );
    }   
}

void DominoPlayerWorker::ProcessIdleDmx( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint )
{
    int numItems = m_params.dominoCount;
    int numItemsAvail = (argsSize/sizeof(int));
    
    if( numItemsAvail>=numItems )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const int* item = (const int*)argsData;
        for( int i=0; i<m_params.dominoCount; i++ )
        {
            int val = little_endian( item[i] );
            m_dmxoutPlayer[i] = val;
        }
    }
    else
    {
        printf( "DominoFX: idle mode dmx values missing in packet %s : %i \n", argsData, numItems );
    }
}

void DominoPlayerWorker::ProcessIdleDmx( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param )
{
    DominoPlayerWorker* parent = (DominoPlayerWorker*)param;
    parent->ProcessIdleDmx(argsData,argsSize,remoteEndpoint);
}


//
// Class DominoPlayerMaster::DmxThread
// Sens DMX updates to hardware

int DominoPlayerWorker::StartOutputDmxThread()
{
    int result = 0;
    
    if( m_dmxInterface!=nullptr )
    {
        printf( "DominoFX: Launching player worker DMX thread ...\n" );
        m_dmxThread = std::thread(OutputDmxThreadBootstrap,this);
    }
    else
    {
        printf( "DominoFX: Cannot launch player worker DMX thread ...\n" );
        result = 1;
    }
    return result;    
}

void DominoPlayerWorker::OutputDmxThread()
{    
    printf( "DominoFX: Player worker DMX thread started ...\n" );

    #ifdef DEBUG_DMX_USB
    m_dmxInterface->GetConfiguration();
    m_dmxInterface->DisplayConfig();
    #endif
    m_dmxInterface->ResetCanauxDMX();
    m_dmxInterface->SendDMX();

    while( !signal_shutdown )
    {
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedUpdate = now - m_dmxLastUpdate;
        m_dmxFps = 1.0f/elapsedUpdate.count();
        
        {
            // Grab lock on the channel data, set channel data in DMX
            // Use separate scope, release mutex before sending data
            std::lock_guard<std::mutex> lock(m_mutex);
            for( int i=0; i<NB_CANAUX_MAX; i++ )
                m_dmxInterface->SetCanalDMX( i+1, m_dmxout[i] );
        }
        m_dmxInterface->SendDMX();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        m_dmxLastUpdate = now;
    }
}
