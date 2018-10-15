#include "DominoState.h"

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

