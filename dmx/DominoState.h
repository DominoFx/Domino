#ifndef DOMINOSTATE_H
#define DOMINOSTATE_H

#include "common.h"
#include "vec3.h"
#include <math.h>
#include <vector>

// 
// Classes
// 

class DominoController;
class DominoParams;
class SensorData;

//
// Class DominoState
// Helper: Sensor, sound and lighting data
// 

class DominoState
{
public:
    DominoState( DominoController& context );

    int Init( int index );
    void Reload();
    void Update( const SensorData* sensorData, Axis axis );
    
    int   Index()           { return m_index; }
    int   Posture()         { return m_posture; }
    float Angle()           { return m_angleHistory[m_historyIndex];}
    float AngleFiltered()   { return m_angleFiltered;}
    float Veloc()           { return m_velocHistory[m_historyIndex];}
    float VelocFiltered()   { return m_velocFiltered;}
    int   ValDMX()          { return m_dmxVal;}
    bool  Active()          { return (m_activated!=0); }
    bool  Tap()             { return (m_tap!=0); }
    float TapMagnitude()    { return m_tapMagnitude; }
    int   Err()             { return (m_err); }
    FVec3_t Accel()         { return m_accelHistory[m_historyIndex]; }
    float AngleUp()         { return m_angleUp;}
    
private:
    // Helpers
    inline void Update_DMX( int posture, float angle, float veloc );
    
    DominoController& m_context;
    DominoParams& m_params;
    int m_index;
    std::vector<float> m_angleHistory;
    std::vector<float> m_velocHistory;
    std::vector<float> m_angleFilteredHistory;
    std::vector<float> m_velocFilteredHistory;
    std::vector<FVec3_t> m_accelHistory;
    int m_historyCount, m_historyIndex;
    int m_err;        // set for single frame if error reading from sensor
    int m_posture;    // one if upright, negative one if fallen, zero in between
    int m_activated;  // set nonzero when speed exceeds a threshold
    int m_accelSend;  // set nonzero for short period after accel OSC message sent to sound mgr
    int m_accelFreq;  // how many should elapse between accel OSC messages sent to sound mgr
    int m_tap;        // set for single frame when activated and collided become true 
    int m_tapRampout; // set nonzero for short period after tap message sent, for cooldown
    int m_tapRampin;  // set nonzero for short period before tap message sent, to measure magnitude
    float  m_tapMagnitude;
    float  m_angleUp;
    float  m_angleFiltered;
    double m_angleFilteredL1, m_angleFilteredL2;
    float  m_velocFiltered;
    double m_velocFilteredL1, m_velocFilteredL2;
    float  m_idleVelocThresh;
    float  m_winkVelocThresh;
    float  m_wink;
    float  m_winkFade;
    int    m_tapCooldown;
    float  m_tapMagnitudePow;
    float  m_tapMagnitudeMul;
    int    m_tapMagnitudeDelay;
    int    m_tapLookbackDelay;
    float  m_tapLookbackThresh;
    int    m_fallRampout;
    int    m_fallRampoutDelay;
    int    m_flashRampin;
    int    m_flashRampinDelay;
    int    m_flashRampout;
    int    m_flashRampoutDelay;
    int    m_flashCooldown; 
    int    m_flashCooldownDelay; 
    float  m_dmxPlayModeUpThresh;
    float  m_dmxPlayModeDownThresh;
    float  m_dmxPlayModeSpan;
    int    m_dmxVal;
        
    int m_instanceIndex;
    static int s_instanceCount;
};

#endif /* DOMINOSTATE_H */

