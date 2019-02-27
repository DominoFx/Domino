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

enum DominoPosture
{
    posture_up = 0,
    posture_left_mid = 1,
    posture_left_down = 2,
    posture_right_mid = 3,
    posture_right_down = 4,
};

class DominoState
{
public:
    DominoState( DominoController& context );

    int Init( int index );
    void Reload();
    void Update( const SensorData* sensorData, Axis axis );
    
    int   Index()           { return m_index; }
    bool  RealSensor()      { return (m_realSensor!=0); }
    int   Posture()         { return m_posture; }
    int   PostureUp()       { return m_postureUp; }
    float Angle()           { return m_angleHistory[m_historyIndex]; }
    float AngleFiltered()   { return m_angleFiltered; }
    float Veloc()           { return m_velocHistory[m_historyIndex]; }
    float VelocFiltered()   { return m_velocFiltered; }
    float VelocPosture()    { return m_velocPosture; }
    int   ValDMX()          { return m_dmxVal; }
    bool  Active()          { return (m_activated!=0); }
    bool  Tap()             { return (m_tap!=0); }
    float TapMagnitude()    { return m_tapMagnitude; }
    int   Err()             { return (m_err); }
    FVec3_t Accel()         { return m_accelHistory[m_historyIndex]; }
    float AngleUp()         { return m_angleUp;}
    
    // Notifications when posture of nearby domino changes, for virtual sensor handling
    void NotifyPostureLeft(  int posture, int posturePrev, float veloc );
    void NotifyPostureRight( int posture, int posturePrev, float veloc );
    
private:
    // Helpers
    void SetPostureGoal( int posture, float veloc );
    inline void Update_DMX( int posture, float angle, float veloc );
    
    DominoController& m_context;
    DominoParams& m_params;
    int m_index;
    int m_realSensor; // if physical sensor present, otherwise position value guessed via notify
    std::vector<float> m_angleHistory;
    std::vector<float> m_velocHistory;
    std::vector<float> m_angleFilteredHistory;
    std::vector<float> m_velocFilteredHistory;
    std::vector<FVec3_t> m_accelHistory;
    int m_historyCount, m_historyIndex;
    int m_err;        // set for single frame if error reading from sensor
    int m_posture;    // posture enum, specific value including mid postures
    int m_postureUp;  // set nonzero when posture crosses fully up, zero when posture crosses fully down
    int m_activated;  // set nonzero when speed exceeds a threshold
    int m_accelSend;  // set nonzero for short period after accel OSC message sent to sound mgr
    int m_accelFreq;  // how many should elapse between accel OSC messages sent to sound mgr
    int m_tap;        // set for single frame when activated and collided become true 
    int m_tapRampout; // set nonzero for short period after tap message sent, for cooldown
    int m_tapRampin;  // set nonzero for short period before tap message sent, to measure magnitude
    float  m_tapMagnitude;
    int    m_markTime;      // current time, measured in ticks (number of update calls)
    int    m_markLastDown;  // last time posture was down left or right, measured in ticks
    int    m_markLastUp;    // last time posture was up, measured in ticks
    float  m_angleLastDown; // last angle when posture was down left or right
    float  m_angleLastUp;   // last angle when posture was up
    float  m_velocPosture;  // velocity over last major posture change between up and down
    float  m_angleUp;
    float  m_angleGoal;
    float  m_angleFiltered;
    double m_angleFilteredL1, m_angleFilteredL2;
    float  m_velocGoal;
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

