#include "DominoState.h"
#include <math.h>

// 
// Globals
// 

int signal_shutdown = 0;


//
// Class DominoParams
// Helper: Parameters specified in config.json
//

DominoParams::DominoParams() :
  masterMode(false)
, workerMode(false)
, dominoCount(0)
, continuousFPS(30)
, i2cDevice( "/dev/i2c-1" )
, dmxDevice( "/dev/ttyUSB0")
, workerToMasterPort(8880)
, masterToWorkerPort(8881)
, workerToSoundPort(8000)
, workerToSoundAddress( "127.0.0.1" )
, soundToWorkerPort(8001)
, masterToSoundPort(8002)
, masterToSoundAddress( "127.0.0.1" )
, soundToMasterPort(8003)
, workerToDiagPort(12345)
, workerToDiagAddress("127.0.0.1")
, sensorAxis(Axis::Z)
, sensorSmoothing(1)
, sensorVelocityMultiplier(1.0f)
, sensorTapThresh(80)
, sensorTapTimeLimit(10)
, sensorTapTimeLatency(20)
, sensorTapTimeWindow(255)
, multiplexerAddress1(0x70) //112
, multiplexerAddress2(0x72) //114
, multiplexerLanes(6)
, dmxEnable(true)
, dmxInterfaceID(0)
, dmxInterface( "" )
, interactTag( "/domino" )
, heartbeatTag( "/moving" )
, configTag( "/config" )
, confirmTag( "/confirm" )
, idleTag( "/idle" )
, diagTag( "/diag" )
, debugOscConfig0(0)
, debugOscConfig1(0)
, debugOutput(false)
{
}


//
// Class DominoState
// Helper: Sensor, sound and lighting data
//

DominoState::DominoState( DominoParams& params ) :
  m_params(params)
, m_index(-1)
, m_historyCount(1)
, m_historyIndex(0)
, m_activated(0)
, m_collided(0)
, m_accelSend(0)
, m_accelFreq(0)
, m_tap(0)
, m_angleAvgSlow(0)
, m_angleAvg(0)
, m_velocAvgSlow(0)
, m_velocAvg(0)
, m_unitVal(0)
, m_dmxVal(100)
{
}

int DominoState::Init( int index )
{
    m_index = index;
    m_historyCount = m_params.sensorSmoothing;
    m_angleHistory.resize(m_historyCount);
    m_velocHistory.resize(m_historyCount);
    for (int i = 0; i < m_historyCount; ++i)
    {
        m_angleHistory[i] = 0;
        m_velocHistory[i] = 0;
    }
    float accelFPS = 10; // rate at which accel OSC message sent to sound mgr
    m_accelFreq = m_params.continuousFPS / accelFPS; // number of frames per accel message sent
}

void DominoState::Update( const SensorData* sensorData, Axis axis )
{
    float angle;
    
    // TODO: Add params to control which axis and positive or negative is up, versus which axis is the roation pivot
    if( axis==Axis::Z )
        angle = atan2( sensorData->acceleration.x, sensorData->acceleration.y );
	
    float veloc = angle - m_angleHistory[m_historyIndex];
	
    // rotating list
    m_historyIndex = (m_historyIndex+1) % m_historyCount;
    m_angleHistory[m_historyIndex] = angle;
    m_velocHistory[m_historyIndex] = veloc;

    double angleTotal = 0.0f;
    double velocTotal = 0.0f;

    for (int i = 0; i < m_historyCount; ++i)
    {
        angleTotal += m_angleHistory[i];
        velocTotal += m_velocHistory[i];
    }

    m_angleAvg = angleTotal / (double)m_historyCount;
    m_velocAvg = velocTotal / (double)m_historyCount;
    float u = m_params.sensorActivityBlend;
    m_angleAvgSlow = ((1.0-u)*m_angleAvgSlow) + (u*m_angleAvg);
    m_velocAvgSlow = ((1.0-u)*m_velocAvgSlow) + (u*m_velocAvg);

    float dmxNorm = std::max(std::min(m_angleAvg, 1.0), -1.0); // Range [-1,1]
    m_unitVal = (dmxNorm + 1.0f) / 2.0f; // Range [0,1]
    
    // TODO: Find better adjustment function
    m_unitVal = 1.0f - pow( (m_unitVal - 0.5f) * 4.0f, 2.0f );
    float dmxByte = m_unitVal * 100.0f; // Maximum range [0,255], but brightness less than maximum
    
    m_dmxVal = std::max(std::min((int)dmxByte, 100), 10);
    
    int tap = 0;
    if( axis==Axis::X ) tap = sensorData->tap.y + sensorData->tap.z;
    if( axis==Axis::Y ) tap = sensorData->tap.x + sensorData->tap.z;
    if( axis==Axis::Z ) tap = sensorData->tap.x + sensorData->tap.y;
    
    int oldTapped = (m_activated!=0) && (m_collided!=0);

    if( m_activated>0 )
        m_activated--;

    // Activate and deactivate on crossing edges
    float speedAvg = abs(m_velocAvgSlow);
    if( speedAvg>0.001 )
    {
        m_activated = 25; // set activated flag when moving, stay for quarter of a second
    }

    if( m_accelSend>0 )
        m_accelSend--;
        
    if( m_collided>0 )
        m_collided--;

    // Set collided flag
    if( tap>0 )
    {
        m_collided = 25; // set collided flag on hardware tap, stay for quarter of a second
    }

    int newTapped = (m_activated!=0) && (m_collided!=0);
    
    // tap is nonzero only at moment of activation
    m_tap = ( newTapped && !oldTapped?  1 : 0 );
    
    m_err = sensorData->err;
}

void DominoState::AccelSent()
{
    m_accelSend = m_accelFreq;
}
