#include "DominoController.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <algorithm>
#include <thread>
#include "BMA220.h"
#include "LISD3H.h"


// 
// Defines
// 

#define CONFIG_FILENAME "config.json"


// 
// Globals
// 

int signal_shutdown = 0;


//
// Class DominoParams
// Helper: Parameters specified in config.json
//

DominoParams::DominoParams() :
  i2cDevice( "/dev/i2c-1" )
, dmxDevice( "/dev/ttyUSB0")
, dmxEnable(true)
, oscAddress("127.0.0.1")
, oscPort(8888)
, oscTag("/domino")
, diagAddress("")
, diagPort(12345)
, multiplexerAddress1(0x70) //112
, multiplexerAddress2(0x72) //114
, multiplexerLanes(6)
, sensorAxis(Axis::Z)
, sensorSmoothing(1)
, sensorVelocityMultiplier(1.0f)
, sensorTapThresh(80)
, sensorTapTimeLimit(10)
, sensorTapTimeLatency(20)
, sensorTapTimeWindow(255)
, debugOutput(false)
{
}


//
// Class DominoState
// Helper: Sensor, sound and lighting data
//

DominoState::DominoState() :
  m_historyCount(1)
, m_historyIndex(0)
, m_angleAvg(0)
, m_speedAvg(0)
, m_dmxVal(0)
{
}

int DominoState::Init( int historyCount )
{
    m_historyCount = historyCount;
    m_angleHistory.resize(m_historyCount);
    m_speedHistory.resize(m_historyCount);
    for (int i = 0; i < m_historyCount; ++i)
    {
        m_angleHistory[i] = 0;
        m_speedHistory[i] = 0;
    }
}

void DominoState::Update( float angle, float speed )
{
    // rotating list
    m_historyIndex = (m_historyIndex+1) % m_historyCount;
    m_angleHistory[m_historyIndex] = angle;
    m_speedHistory[m_historyIndex] = speed;

    double angleTotal = 0.0f;
    double speedTotal = 0.0f;

    for (int i = 0; i < m_historyCount; ++i)
    {
        angleTotal += m_angleHistory[i];
        speedTotal += m_speedHistory[i];
    }

    m_angleAvg = angleTotal / (double)m_historyCount;
    m_speedAvg = speedTotal / (double)m_historyCount;

    double dmxNormalized = std::max(std::min(m_angleAvg, 1.0), -1.0);

    m_dmxVal = (int)(((dmxNormalized + 1.0f) / 2.0f) * 255.0f);
    m_dmxVal = std::max(std::min(m_dmxVal, 255), 0);
}


// 
// Class DominoController
// Main class
//

DominoController::DominoController():
  m_diagSock(-1)
, m_continuousDelay(1.0f / 30.0f) //Default to 30fps
, m_debugOutput(false)
, m_debugCmd(0)
, m_dmxInterface(NULL)
, m_initialized(false)
{
}


DominoController::~DominoController()
{
    signal_shutdown = 1;
}

bool DominoController::Init()
{
    printf( "DominoFX: Reading config file... \n" );
    Json::Value jsonRoot;
    std::ifstream configfile(CONFIG_FILENAME);
    configfile >> jsonRoot;
    
    EnttecInterfaces dmxInterfaceID = DMX_USB_PRO;
    
    if(jsonRoot.empty())
    {
        return false;
    }
   
    // device name depends on Raspberry Pi model
    // to deterime the correct device, run the following at the command line:
    //   sudo i2cdetect -y 0
    //   sudo i2cdetect -y 1
    // if the first one works, use "/dev/i2c-0"
    // if the second one works, use "/dev/i2c-1"

    if(jsonRoot.isMember("i2cDevice"))
    {
        m_params.i2cDevice = jsonRoot["i2cDevice"].asString();
    }

    if(jsonRoot.isMember("dmxDevice"))
    {
        m_params.dmxDevice = jsonRoot["dmxDevice"].asString();
    }

    if(jsonRoot.isMember("debugOutput"))
    {
        m_params.debugOutput = jsonRoot["debugOutput"].asBool();
    }

    if(jsonRoot.isMember("dmxInterface"))
    {
        m_params.dmxInterface = jsonRoot["dmxInterface"].asString();

        if(m_params.dmxInterface == "open")
        {
            dmxInterfaceID = OPEN_DMX_USB;
        }
    }

    if (jsonRoot.isMember("sensorCount"))
    {
        m_params.sensorCount = jsonRoot["sensorCount"].asInt();
    }

    if(jsonRoot.isMember("continuousFPS"))
    {
        m_params.continuousFPS = jsonRoot["continuousFPS"].asFloat();
        {
          this->m_continuousDelay = 1.0f / m_params.continuousFPS;
        }

    }
    
    if(jsonRoot.isMember("sensorAxis"))
    {
        m_params.sensorAxis = (Axis)jsonRoot["sensorAxis"].asInt();
    }

    if (jsonRoot.isMember("dmxEnable"))
    {
        m_params.dmxEnable = jsonRoot["dmxEnable"].asBool();
    }

    if (jsonRoot.isMember("oscAddress") && jsonRoot.isMember("oscPort") && jsonRoot.isMember("oscTag"))
    {
        m_params.oscAddress = jsonRoot["oscAddress"].asString();
        m_params.oscPort = jsonRoot["oscPort"].asInt();

        if( jsonRoot.isMember("oscTag") )
        {
            m_params.oscTag = jsonRoot["oscTag"].asString();
        }
        
        printf( "DominoFX: Initializing sound controller...\n" );
        m_oscController.Init( m_params.oscAddress, m_params.oscPort );
    }

    if(jsonRoot.isMember("oscDMXSendThreshold"))
    {
        // no longer used
    }
    
    if(jsonRoot.isMember("multiplexerAddress1"))
    {
        m_params.multiplexerAddress1 = jsonRoot["multiplexerAddress1"].asInt();
    }

    if(jsonRoot.isMember("multiplexerAddress2"))
    {
        m_params.multiplexerAddress2 = jsonRoot["multiplexerAddress2"].asInt();
    }

    if(jsonRoot.isMember("multiplexerLanes"))
    {
        m_params.multiplexerLanes = jsonRoot["multiplexerLanes"].asInt();
    }

    if(jsonRoot.isMember("sensor"))
    {
        // no longer used
    }
    
    if(jsonRoot.isMember("sensorSmoothing"))
    {
        m_params.sensorSmoothing = jsonRoot["sensorSmoothing"].asInt();
    }

    if(jsonRoot.isMember("sensorVelocityMultiplier"))
    {
        m_params.sensorVelocityMultiplier = jsonRoot["sensorVelocityMultiplier"].asFloat();
    }

    if(jsonRoot.isMember("sensorTapThresh"))
    {
        m_params.sensorTapThresh = jsonRoot["sensorTapThresh"].asInt();
    }
    if(jsonRoot.isMember("sensorTapTimeLimit"))
    {
        m_params.sensorTapTimeLimit = jsonRoot["sensorTapTimeLimit"].asInt();
    }
    if(jsonRoot.isMember("sensorTapTimeLatency"))
    {
        m_params.sensorTapTimeLatency = jsonRoot["sensorTapTimeLatency"].asInt();
    }
    if(jsonRoot.isMember("sensorTapTimeWindow"))
    {
        m_params.sensorTapTimeWindow = jsonRoot["sensorTapTimeWindow"].asInt();
    }

    if (jsonRoot.isMember("diagAddress") && jsonRoot.isMember("diagPort") )
    {
        m_params.diagAddress = jsonRoot["diagAddress"].asString();
        m_params.diagPort = jsonRoot["diagPort"].asInt();
        
        if( m_params.diagAddress.length() )
        {
            m_diagSock = socket( AF_INET, SOCK_STREAM, 0 );
            if( m_diagSock==-1 )
            {
                printf( "DominoFX: Unable to create diagnostic socket \n" );
            }
            else
            {
                m_diagAddr.sin_family = AF_INET;
                m_diagAddr.sin_port = htons( m_params.diagPort );
                inet_aton( m_params.diagAddress.c_str(), &m_diagAddr.sin_addr );
                //printf( "DominoFX: Diagnostic server address %s:%i \n",
                //    inet_ntoa(m_diagAddr.sin_addr), m_params.diagPort );
            }
        }
    }

    if (m_params.dmxEnable)
    {
        printf( "DominoFX: Initializing USB->DMX controller... \n" );
        m_dmxInterface = new EnttecDMXUSB( dmxInterfaceID, m_params.dmxDevice );
        
        if((m_dmxInterface && m_dmxInterface->IsAvailable()))
        {
            #ifdef DEBUG_DMX_USB
            m_dmxInterface->GetConfiguration();
            m_dmxInterface->DisplayConfig();
            #endif
            m_dmxInterface->ResetCanauxDMX();
            m_dmxInterface->SendDMX();
            m_initialized = true;
        }
        else
        {
             printf( "DominoFX: No USB->DMX controller found ... aborting\n" );
        }
    }
    else
    {
        m_initialized = true;
    }
    
    m_lastContinuousSend = std::chrono::system_clock::now();

    m_state.resize( m_params.sensorCount );    
    for( int i=0; i<m_params.sensorCount; i++ )
    {
        m_state[i].Init( m_params.sensorSmoothing );
    }

    if( m_diagSock != -1 )
    {
        printf( "DominoFX: Connecting to diagnostic server %s : %i \n",
            inet_ntoa(m_diagAddr.sin_addr), m_params.diagPort );
        int res = connect( m_diagSock, (struct sockaddr*)&m_diagAddr, sizeof(m_diagAddr) );
        if( res < 0 )
        {
            printf( "ERROR: couldn't connect to diagnostic server \n" );
            close( m_diagSock );
            m_diagSock = -1;
        }
    }


    StartInputThread();

    StartSensorThread();

    return m_initialized;
}


void DominoController::Update()
{
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedSend = now - m_lastContinuousSend;

    if( elapsedSend.count() >= m_continuousDelay )
    {
        if( m_debugCmd>0 ) // user input debugging command
        {
            m_debugOutput = true;
            m_debugCmd--;
        }

        for (int sensorIndex = 0; sensorIndex < m_params.sensorCount; ++sensorIndex)
        {
            Send(sensorIndex);
        }
        if (m_params.dmxEnable && (m_dmxInterface!=nullptr) )
        {
            // Send DMX signal for all channels in one call
            m_dmxInterface->SendDMX();
        }
        
        if( m_debugOutput )
        {
            printf( "---------- ----------\n" );
            printf("FPS: %f\n", (float)(1.0f/elapsedSend.count()) );
        }

      
        m_debugOutput = false; // disable debugging until next update
        m_lastContinuousSend = now;
    }
    else
    {
    }
}


const DominoParams& DominoController::GetDominoParams()
{
    return this->m_params;
}


bool DominoController::Send(uint8_t sensorIndex)
{
    if( (sensorIndex<0) || (sensorIndex >= m_sensor.size()) ||
        (m_sensor[sensorIndex]==nullptr) )
    {
        printf( "ERROR: DominoController::Send() no sensor at index %i\n", sensorIndex );
        return 0;
    }

    if( m_debugOutput )
    {
        printf( "---------- ----------\n" );
        printf("Sensor %i\n", sensorIndex);
        m_sensor[sensorIndex]->DebugPrint();
    }

    const SensorData* sensorData = m_sensor[sensorIndex]->GetData();

    if( m_debugOutput )
    {
        const FVec3_t& v = sensorData->acceleration;
        printf( "  Accel normalized:\t (%.3f,%.3f,%.3f)\n",
            (float)(v.x), (float)(v.y), (float)(v.z) );
    }

    int d; // data index
    if( m_params.sensorAxis==Axis::X ) d = 0;
    if( m_params.sensorAxis==Axis::Y ) d = 1;
    if( m_params.sensorAxis==Axis::Z ) d = 2;

    float angle = sensorData->position[d];
    float speed = sensorData->velocity[d];
    int tap = sensorData->tap[d];
    
    DominoState& state = m_state[sensorIndex];
    state.Update( angle, speed );

    int dmxVal = state.DMXVal();

    if (m_debugOutput)
    {
         printf("  Angle:\t\t %f\n  Speed:\t\t %f\n  DMX:\t\t\t  %i\n  Angle smoothed:\t %f\n  Speed smoothed:\t %f\n",
                 (int)sensorIndex, (float)angle, (float)speed, (int)dmxVal,
                 (float)(state.Angle()), (float)(state.Speed()));
    }


    if (m_params.dmxEnable && (m_dmxInterface!=nullptr) )
    {
        // Set DMX data for this channel
        // Data will be transmitted later, via SendDMX(), when all channels are colleced
        m_dmxInterface->SetCanalDMX(sensorIndex + 1, dmxVal);
    }


    // Send tap signal (only if tap was detected)
    if( tap )
    {
        m_oscController.Send(m_params.oscTag, sensorIndex + 1);
    }

    // Send speed and angle signal
    speed = speed * m_params.sensorVelocityMultiplier;
    float oscParams[7] = {
        speed, angle,
        (float)(state.Speed()), (float)(state.Angle()),
        sensorData->acceleration[0], sensorData->acceleration[0], sensorData->acceleration[2] };
    m_oscController.Send(m_params.oscTag, sensorIndex + 1, 7, oscParams);

    // Send diagnostic signal
    if( m_diagSock>=0 )
    {
    
        char buf[1024];
        const FVec3_t& v = sensorData->acceleration;
        sprintf( buf, "%i %.4f %.4f %.4f\n", sensorIndex, (float)(v.x), (float)(v.y), (float)(v.z) );        
        send( m_diagSock, buf, strlen(buf), 0 );
    }

    return true;
}


// 
// Sensor Thread
// 

int DominoController::StartSensorThread()
{
    int result = 0;

    printf( "DominoFX: Initializing I2C bus... \n" );
    m_bus.Init( m_params.i2cDevice.data() );

    // Toggle the mux off (all of them)
    m_bus.WriteGlobal( m_params.multiplexerAddress1, 0 );
    m_bus.WriteGlobal( m_params.multiplexerAddress2, 0 );

    m_sensor.resize( m_params.sensorCount );

    SensorParams sensorParams;
    sensorParams.tapThresh       = m_params.sensorTapThresh;
    sensorParams.tapTimeLimit    = m_params.sensorTapTimeLimit;
    sensorParams.tapTimeLatency  = m_params.sensorTapTimeLatency;
    sensorParams.tapTimeWindow   = m_params.sensorTapTimeWindow;

    printf( "DominoFX: Scanning for %i sensors... \n", (int)(m_params.sensorCount) );
    for( int sensorIndex=0; sensorIndex<m_sensor.size(); sensorIndex++ )
    {
        if( sensorIndex<m_params.multiplexerLanes )
        {
            sensorParams.muxAddress = m_params.multiplexerAddress1;
            sensorParams.muxField = (1<<sensorIndex);
        }
        else
        {
            sensorParams.muxAddress = m_params.multiplexerAddress2;
            sensorParams.muxField = (1<<(sensorIndex-m_params.multiplexerLanes));
        }

        // auto-detect LIS3DH or BMA220 motion sensor
        if( LIS3DH::IsAvailable( &m_bus, &sensorParams ) )
        {
            printf( "DominoFX: Initializing LIS3DH motion sensor at index %i... \n", (int)sensorIndex );
            m_sensor[sensorIndex] = new LIS3DH();
            m_sensor[sensorIndex]->Init( &m_bus, &sensorParams );
        }
        else if( BMA220::IsAvailable( &m_bus, &sensorParams ) )
        {
            printf( "DominoFX: Initializing BMA220 motion sensor at index %i... \n", (int)sensorIndex );
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

void DominoController::SensorThread( DominoController* parent )
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
    }
    printf( "DominoFX: Sensor thread stopped\n" );
}


// 
// User input thread
// 

int DominoController::StartInputThread()
{
    int result = 0;
    
    printf( "DominoFX: Launching DominoInput thread...\n" );
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
    printf( "DominoFX: InputThread stopped\n" );
}
