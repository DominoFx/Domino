#ifndef DOMINOSTATE_H
#define DOMINOSTATE_H

#include "common.h"
#include <vector>
#include <string>

//
// Class DominoParams
// Helper: Parameters specified in config.json
// 

struct DominoParams
{
    DominoParams();

    std::string i2cDevice;
    std::string dmxDevice;

    bool dmxEnable;
    std::string dmxInterface;    

    std::string oscAddress;
    int oscPort;
    std::string oscTag;

    std::string diagAddress;    
    int diagPort;

    int multiplexerAddress1;
    int multiplexerAddress2;
    int multiplexerLanes;    
     
    Axis sensorAxis;// 1 = x, 2 = y, 4 = z, 7 = xyz
    int sensorCount;
    int sensorSmoothing;
    int sensorVelocityMultiplier;
    int sensorTapThresh;
    int sensorTapTimeLimit;
    int sensorTapTimeLatency;
    int sensorTapTimeWindow;

    float continuousFPS;

    bool debugOutput;
};


//
// Class DominoState
// Helper: Sensor, sound and lighting data
// 

class DominoState
{
public:
    DominoState();

    int Init( int historyCount );
    void Update(float angle, float speed );
    
    double Angle() { return m_angleAvg;}
    double Speed() { return m_speedAvg;}
    int DMXVal() { return m_dmxVal;}
    
private:

    std::vector<float> m_angleHistory;
    std::vector<float> m_speedHistory;
    int m_historyCount, m_historyIndex;
    double m_angleAvg;
    double m_speedAvg;
    int m_dmxVal;
};


#endif /* DOMINOSTATE_H */