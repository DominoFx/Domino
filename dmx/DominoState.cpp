#include "DominoState.h"
#include "DominoParams.h"
#include "DominoController.h"
#include "ISensor.h"
#include <math.h>
#include <algorithm>
//
// Class DominoState
// Helper: Sensor, sound and lighting data
//

// PHYSICAL MEASUREMENT:
const float dominoFallAngle = 1.00f; // angle value of fallen domino should be + or - this amount
//const float dominoFallAngle = 1.31f; // angle value of fallen domino should be + or - this amount
// measured value 1.31 for middle dominos 1.00 for end dominos

// PHYSICAL GUESSES:
const float dominoLiftTime = 1.50f; // average number of seconds for domino to be lifted up
// TODO: hack, artificially fast because the angle filtering makes effective speed lower
const float dominoFallTime = 0.05f; // maximim number of seconds for domino to fall

int DominoState::s_instanceCount;


DominoState::DominoState( DominoController& context ) :
  m_context(context)
, m_params(context.GetParams())
, m_index(-1)
, m_historyCount(1)
, m_historyIndex(0)
, m_err(0)
, m_activated(0)
, m_tap(0)
, m_tapRampout(-1)
, m_tapRampin(-1)
, m_tapMagnitude(0)
, m_markTime(0)
, m_markLastDown(0)
, m_markLastUp(0)
, m_angleLastDown(0)
, m_angleLastUp(0)
, m_velocPosture(0)
, m_angleUp(0)
, m_angleGoal(0)
, m_angleFiltered(0)
, m_angleFilteredL1(0)
, m_angleFilteredL2(0)
, m_velocGoal(0)
, m_velocFiltered(0)
, m_velocFilteredL1(0)
, m_velocFilteredL2(0)
, m_idleVelocThresh(0)
, m_winkVelocThresh(0)
, m_tapCooldown(0)
, m_tapLookbackDelay(0)
, m_tapLookbackThresh(0)
, m_fallRampout(-1)
, m_fallRampoutDelay(0)
, m_flashRampin(0)
, m_flashRampout(0)
, m_flashCooldown(0)
, m_dmxVal(100)
{
    m_instanceIndex = s_instanceCount++;
}

int DominoState::Init( int index )
{
    m_index = index;
    m_angleHistory.reserve(100);
    m_velocHistory.reserve(100);
    m_angleFilteredHistory.reserve(100);
    m_velocFilteredHistory.reserve(100);
    m_accelHistory.reserve(100);
    
    Reload();
    return 1;
}

void DominoState::Reload()
{
    m_idleVelocThresh   = m_params.sensorIdleVelocityThresh;
    m_winkVelocThresh   = m_params.sensorWinkVelocityThresh;
    m_tapLookbackThresh = m_params.sensorTapLookbackThresh;
    m_tapCooldown       = (int)ceilf((m_params.sensorTapCooldown/1000.0f)*m_params.continuousFPS);
    m_tapMagnitudePow  = m_params.sensorTapMagnitudePow;
    m_tapMagnitudeMul  = m_params.sensorTapMagnitudeMul;
    m_tapMagnitudeDelay  = (int)ceilf((m_params.sensorTapMagnitudeDelay/1000.0f)*m_params.continuousFPS);
    m_tapLookbackDelay  = (int)ceilf((m_params.sensorTapLookbackDelay/1000.0f)*m_params.continuousFPS);
    m_fallRampoutDelay = m_params.dmxPlayModeFallDelay;
    m_flashRampinDelay = m_params.dmxPlayModeFlashIn;
    m_flashRampoutDelay = m_params.dmxPlayModeFlashOut;
    m_flashCooldownDelay = m_params.dmxPlayModeFlashDelay;
    m_dmxPlayModeUpThresh = (m_params.dmxPlayModeUpRange - 0.01);
    m_dmxPlayModeDownThresh = dominoFallAngle - 0.15f; // TODO: fix hard-coded constant fudge factor
    m_dmxPlayModeSpan = m_dmxPlayModeDownThresh - m_dmxPlayModeUpThresh;
    m_historyCount = std::max( 2*(m_tapLookbackDelay+m_tapMagnitudeDelay), 10 );
    if( (m_angleHistory.size()!=m_historyCount) || (m_velocHistory.size()!=m_historyCount) ||
        (m_accelHistory.size()!=m_historyCount) )
    {
        m_historyIndex=0;
        m_angleHistory.resize(m_historyCount);
        m_velocHistory.resize(m_historyCount);
        m_angleFilteredHistory.resize(m_historyCount);
        m_velocFilteredHistory.resize(m_historyCount);
        m_accelHistory.resize(m_historyCount);
        for( int i=0; i<m_historyCount; i++ )
        {
            m_angleHistory[i] = 0;
            m_velocHistory[i] = 0;
            m_angleFilteredHistory[i] = 0;
            m_velocFilteredHistory[i] = 0;
            m_accelHistory[i].x = m_accelHistory[i].y = m_accelHistory[i].z = 0;
        }
    }
    if( m_instanceIndex==0 )
    {
        printf( "DominoFX: sensor idle veloc thresh = %.4f\n", (float)m_idleVelocThresh );
        printf( "DominoFX: sensor tap lookback thresh = %f\n", (float)m_tapLookbackThresh );
        printf( "DominoFX: sensor tap cooldown = %i\n", (int)m_tapCooldown );
        printf( "DominoFX: sensor tap magnitude pow = %.4f\n", (float)m_tapMagnitudePow );
        printf( "DominoFX: sensor tap magnitude mul = %.4f\n", (float)m_tapMagnitudeMul );
        printf( "DominoFX: sensor tap magnitude delay = %i\n", (int)m_tapMagnitudeDelay );
        printf( "DominoFX: sensor tap lookback delay = %i\n", (int)m_tapLookbackDelay );
        printf( "DominoFX: sensor history count = %i\n", (int)(m_velocHistory.size()) );
    }
}

inline int RotatingListIndex( int i, std::vector<float>& f )
{
    // TODO: if this is unsigned, adding negative number gives incorrect result (increses value)
    int range = f.size(); // Compiler bug?
    
    i = (i%range);
    return (i<0? (range+i) : i);
}
inline void RotatingListMinMax( float& min, float& max, int i, int span, std::vector<float>& f )
{
    i = RotatingListIndex( i, f );
    min = max = f[i];
    unsigned int range = f.size();
    for( ; span>0; span--, i=((i+1)%range) )
    {
        if( f[i]<min ) min = f[i];
        if( f[i]>max ) max = f[i];
    }
}
inline float RotatingListDerivative( int i, std::vector<float>& f )
{
    int iU2 = RotatingListIndex( (i+2), f );
    int iU1 = RotatingListIndex( (i+1), f );
    int iD1 = RotatingListIndex( (i-1), f );
    int iD2 = RotatingListIndex( (i-2), f );
    float q = (-f[iU2]) + (8.0f*f[iU1]) + (-8.0f*f[iD1]) + (f[iD2]);
    float retval = (q / 12.0f);
    return retval;
}

void DominoState::Update( const SensorData* sensorData, Axis axis )
{
	float angle;
    FVec3_t accel;
    
    if( sensorData==nullptr )
    {
        m_realSensor = 0;
        
        // Virtual sensor
        // Angle is calculated by moving toward "angle goal" at rate equal to "velocity goal"
        // Both goals determined via notifications from nearby dominos
        angle = m_angleHistory[m_historyIndex] + m_velocGoal;
        
        float anglePrev = m_angleHistory[m_historyIndex];
                
        bool passedGoalDown = (angle < m_angleGoal) && (m_velocGoal < 0);
        bool passedGoalUp   = (angle > m_angleGoal) && (m_velocGoal > 0);        
        if( passedGoalDown || passedGoalUp )
        {   // when crossing the goal, backtrack to hit it exactly, but slowing down each crossing
            m_velocGoal = -(m_velocGoal/2.0f);
        }
        accel.x = accel.y = accel.z = 0;
        m_err = 0;
    }
    else
    {
        m_realSensor = 1;
        // TODO: Add params to control which axis and positive or negative is up, versus which axis is the roation pivot
	    if( axis==Axis::Z )
		    angle = atan2( sensorData->acceleration.x, sensorData->acceleration.y );
	    if( axis==Axis::X )
		    angle = atan2( sensorData->acceleration.z, sensorData->acceleration.y );
    
        angle -= m_angleUp;
        accel = sensorData->acceleration;
        // Hardware tap detection
        //if( axis==Axis::X ) tap = sensorData->tap.y + sensorData->tap.z;
        //if( axis==Axis::Y ) tap = sensorData->tap.x + sensorData->tap.z;
        //if( axis==Axis::Z ) tap = sensorData->tap.x + sensorData->tap.y;
        m_err = sensorData->err;    
    }
    
	
	//float veloc = angle - m_angleHistory[m_historyIndex];
	
    float uL1, uL2;
    
    uL1 = m_params.sensorFilterAmountL1;
    uL2 = m_params.sensorFilterAmountL2;
    
    float anglePrev = m_angleFilteredL1;
    m_angleFilteredL1 = ((1.0-uL1)*m_angleFilteredL1) + (uL1*angle);
    m_angleFilteredL2 = ((1.0-uL2)*m_angleFilteredL2) + (uL2*m_angleFilteredL1);
    
    float veloc = anglePrev - m_angleFilteredL1;
    m_velocFilteredL1 = ((1.0-uL1)*m_velocFilteredL1) + (uL1*veloc);
    m_velocFilteredL2 = ((1.0-uL2)*m_velocFilteredL2) + (uL2*m_velocFilteredL1);


    // Choose which filtered values will be transmitted to sound and diag servers
    m_angleFiltered = m_angleFilteredL2;
    m_velocFiltered = m_velocFilteredL2 * m_params.sensorVelocityMultiplier;
    
    
    // old velocity entry overwritten below is our lookback velocity
    int indexOld = RotatingListIndex(m_historyIndex-m_tapLookbackDelay,m_velocFilteredHistory);
    float velocOld = m_velocFilteredHistory[indexOld];
    float velocNew = m_velocFiltered;
    float angleOld = m_angleFilteredHistory[indexOld];
    
    // rotating list
    m_historyIndex = (m_historyIndex+1) % m_historyCount;
    m_angleHistory[m_historyIndex] = angle;
    m_velocHistory[m_historyIndex] = veloc;
    m_angleFilteredHistory[m_historyIndex] = m_angleFiltered;
    m_velocFilteredHistory[m_historyIndex] = m_velocFiltered;
    m_accelHistory[m_historyIndex] = accel;

    // check if current state counts as a tap, not necessarily valid to send yet
    int currentTapped = 0;
    float velocDiff = fabs(velocNew-velocOld);
    if( velocDiff > m_tapLookbackThresh )
    {
        currentTapped = 1;
    }

    // check if current state should activate from idle mode    
    float velocAbs = abs(m_velocFiltered);
    m_activated = (velocAbs>m_idleVelocThresh? 1:0);

    // posture handling    
    float angleAbs = fabs( m_angleFiltered );
    if( angleAbs < m_dmxPlayModeUpThresh )
    {
        m_posture = posture_up;
    }
    else if( m_angleFiltered>0 ) // falling left
    {
        if(angleAbs >= m_dmxPlayModeDownThresh)
             m_posture = posture_left_down;
        else m_posture = posture_left_mid;
    }
    else if( m_angleFiltered<0 ) // falling right
    {
        if(angleAbs >= m_dmxPlayModeDownThresh)
             m_posture = posture_right_down;
        else m_posture = posture_right_mid;
    }
    // handle posture crossing from up to down, measure velocPosture    
    m_markTime++;
    int postureUpPrev = m_postureUp;
    if( angleAbs < m_dmxPlayModeUpThresh )
    {
        m_markLastUp = m_markTime;
        m_angleLastUp = m_angleFiltered;
        m_postureUp = 1;
    }
    else if( angleAbs >= m_dmxPlayModeDownThresh )
    {
        m_markLastDown = m_markTime;
        m_angleLastDown = m_angleFiltered;
        m_postureUp = 0;
    }
    // calculate velocPosture on posture crossing
    if( m_postureUp!=postureUpPrev )
        // handles sign correctly, it's magic
        m_velocPosture = ( (m_angleLastUp-m_angleLastDown) / (m_markLastUp-m_markLastDown) );
    

    // check calibration; if domino is inactive and fallen in its "flattest" direction,
    // then drift the true up direction, comparing reported angle to known physical value
    if( (m_index<5) && (angle<-1) && (m_velocFiltered<0.001) )
    {
        float uprightGuess = angle + dominoFallAngle;
        m_angleUp += ( uprightGuess<m_angleUp?  -0.0001f : 0.0001f );
    }
    if( (m_index>=1) && (angle>1) && (m_velocFiltered<0.001) )
    {
        float uprightGuess = angle - dominoFallAngle;
        m_angleUp += ( uprightGuess<m_angleUp?  -0.0001f : 0.0001f );
    }
    
//    if( upright && !m_activated )
//    {
//        if( (angle<0) && (m_angleUp>-m_dmxPlayModeUpThresh) )
//            m_angleUp -= 0.0001f;
//        else if( (angle>0) && (m_angleUp<m_dmxPlayModeUpThresh) )
//            m_angleUp += 0.0001f;
//    }
    
    // countdown if tap was sent recently, to allow another tap
    if( m_tapRampout>=0 )
    {
        m_tapRampout--;
    }

    // countdown to send tap soon
    // goes to 0 at moment countdown finishes
    // goes to -1 after countdown is finished
    if( m_tapRampin>=0 )
    {
        m_tapRampin--;
    }
    
    // if tap becomes official, will be sent after ramp in period
    if( (currentTapped!=0) && (m_tapRampin<0) && (m_tapRampout<0) )
    {
        m_tapRampin = m_tapMagnitudeDelay;
    }

    // if ramp in reaches zero, we can finally send the tap signal now
    if( m_tapRampin==0 )
    {
        float min, max;
        int span = 1+(m_tapLookbackDelay + m_tapMagnitudeDelay);
        RotatingListMinMax( min, max, m_historyIndex-span, span, m_velocFilteredHistory );
        m_tapRampout = m_tapCooldown; // tap is sent, and will be allowed again after ramp out period
        float diff = (max-min)-m_tapLookbackThresh;
        if( diff<0 ) //shouldn't happen, threshold should be the minimum possible value for (max-min)
            diff=0;
        m_tapMagnitude = (pow(diff,m_tapMagnitudePow)*m_tapMagnitudeMul)+1.0f;
        m_tap = 1;
    }
    else m_tap = 0;
    
    
    if( m_tap ) printf("tap magnitude %.4f \n",m_tapMagnitude);

    
    // Update the DMX value
    Update_DMX( m_posture, m_angleFiltered, m_velocFiltered ); // sets m_dmxVal
}

inline bool PostureLeft(  int posture ) { return (posture==posture_left_mid)  || (posture==posture_left_down);  }
inline bool PostureRight( int posture ) { return (posture==posture_right_mid) || (posture==posture_right_down); }

void DominoState::NotifyPostureLeft(  int posture, int posturePrev, float veloc )
{
    bool thatRightPrev = PostureRight( posturePrev );
    bool thisLeftCur   = PostureLeft(  m_posture );

    // Nearby domino is going up, and had an insersection with us earlier; assume we'll travel up    
    if( (posture==posture_up) && (thatRightPrev || thisLeftCur) )
        SetPostureGoal( posture_up, veloc );
    // Nearby domino is going down, while intersecting with us; assume we'll travel down
    // Special case if we're already leaned left, assume the two dominos touch and balance
    if( (posture==posture_right_down) && !thisLeftCur )
        SetPostureGoal( posture_right_down, veloc );
}

void DominoState::NotifyPostureRight( int posture, int posturePrev, float veloc )
{
    bool thatLeftPrev = PostureLeft(  posturePrev );
    bool thisRightCur = PostureRight( m_posture );

    // Nearby domino is going up, and had an insersection with us earlier; assume we'll travel up    
    if( (posture==posture_up) && (thatLeftPrev || thisRightCur) )
        SetPostureGoal( posture_up, veloc );
    // Nearby domino is going down, while intersecting with us; assume we'll travel down
    if( (posture==posture_left_down) && !thisRightCur )
        SetPostureGoal( posture_left_down, veloc );
}

void DominoState::SetPostureGoal( int posture, float veloc )
{
    if( posture==posture_up )
        m_angleGoal = 0;
    else if( posture==posture_left_down )
        m_angleGoal =  dominoFallAngle;
    else if( posture==posture_right_down )
        m_angleGoal = -dominoFallAngle;

    // lifting speed is constnt, relatively slow
    float speedUpConst = (1.0f/(dominoLiftTime*m_params.continuousFPS));
    // lifting speed minimum, relatively false
    float speedDownMin = (1.0f/(dominoFallTime*m_params.continuousFPS));
        
    float speed;
    if( posture==posture_up )
         speed = speedUpConst;
    else speed = MAX( fabs(veloc), speedDownMin );
    
    if( m_angleFiltered > m_angleGoal )
        m_velocGoal = -speed;
    else
        m_velocGoal = speed;
}

// Helper for Update()
void DominoState::Update_DMX( int posture, float angle, float veloc )
{
    float angleAbs = fabs( angle );
    
    float winkSpeed = 0.15;
    
    // Calculate the base lighting value to be modulated...
    float pulseCenter = m_params.dmxBaseline;
    if( angleAbs < m_dmxPlayModeUpThresh )
    {
        pulseCenter = m_params.dmxPlayModeUp;
    }
    else if( angleAbs < 0.7f )
    {
        float w = (angleAbs - m_dmxPlayModeUpThresh) / (0.70f - m_dmxPlayModeUpThresh); // <- 0 to 1
        w = CLAMP( w, 0.0f, 1.0f );
        pulseCenter = LERP( m_params.dmxPlayModeUp, m_params.dmxPlayModeMax, w );
        winkSpeed = LERP( 0.15f, 0.20f, w );
    }
    else // angle abs >= 0.7f
    {
        float w = (angleAbs - 0.70f) / (dominoFallAngle - 0.80f); // <- 0 to 1 ish
        w = CLAMP( w, 0.0f, 1.0f );
        pulseCenter = LERP( m_params.dmxPlayModeMax, m_params.dmxPlayModeMin, w );        
        winkSpeed = 0.20f;
    }

    // Calculate the amount of modulation (wink) on the lighting...
    float velocAbs = abs(veloc);
    if( velocAbs<m_winkVelocThresh )
         m_winkFade -= 0.005f;
    else m_winkFade += 0.02;
    m_winkFade = CLAMP( m_winkFade, 0, 1 );

    if( m_winkFade==0 )
         m_wink = 0;
    else m_wink += winkSpeed;
    
    float pulseSin = m_winkFade * sin(m_wink); // <- from -1 to +1
    //float pulseAmount = 30.0f * pulseSin; // <- from -15 to +15
    float pulseMul = 0.6f + ((1.0f+pulseSin) / 2.5f); // <- from 0.6 to 1.4    
    
    // Add base lighting amount to modulation (wink) amount...

//
// *** DISABLED LAST MINUTE AFTER HARDWARE PROBLEMS ***
//     Some sensors give erratic readings, causing glitchy winking, so
//     all winking disabled to avoid drawing attention to the problems

//    float pulse = pulseMul * pulseCenter; //pulseCenter + pulseAmount; // <- slightly above or below pulseCenter
//    pulse = CLAMP( pulse, 0, m_params.dmxPlayModeMax );
//    m_dmxVal = pulse;

    m_dmxVal = pulseCenter;
    
    // Lighting wave back up to baseline if below...    
    
    if( posture==posture_up ) 
    {
        m_fallRampout = -1;
    }
    if( m_dmxVal < m_params.dmxBaseline )
    {
        if( m_fallRampout==0 )
        {
            m_dmxVal = m_params.dmxBaseline; // fell too long ago, clip to baseline
        }
        else if( m_fallRampout<0 )
        {
            m_fallRampout = m_fallRampoutDelay; // fell at this moment, begin rampout
        }
        else
        {
            if( m_fallRampout<(m_fallRampoutDelay/4) )
            {   // fell recently, slowly ramp upwards, decrementing rampout
                float w = m_fallRampout / (float)(m_fallRampoutDelay/4);
                m_dmxVal = (int)((w*m_dmxVal) + ((1.0f-w)*m_params.dmxBaseline));
            }
            m_fallRampout--;
        }
    }    
}



/*  CODE JUNKYARD
 
    // Lighting wave behavior
     
    // After a domino falls, the dmx value goes down to m_params.dmxPlayModeMin
    // Ramp this slowly back up to m_params.dmxBaseline, until the next fall    
    float angleAbs = 1.0f - fabs( angle );
    bool upright = (posture==1); // upright posture
    bool fallen  = (posture==-1); // fallen posture
    if( upright ) 
    {
        m_fallRampout = -1;
        m_dmxVal = m_params.dmxPlayModeUp;
    }
    else
    {
        if( fallen )
            m_dmxVal = m_params.dmxPlayModeMin;
        else // partially fallen posture
        {
            float w = (angleAbs - m_dmxPlayModeDownThresh) / m_dmxPlayModeSpan;
            m_dmxVal = (int)((w*m_params.dmxPlayModeUp) + ((1.0f-w)*m_params.dmxPlayModeMin));
        }  
        
        if( m_dmxVal < m_params.dmxBaseline )
        {
            if( m_fallRampout==0 )
                m_dmxVal = m_params.dmxBaseline; // fell too long ago, clip to baseline
            else if( m_fallRampout<0 )
                m_fallRampout = m_fallRampoutDelay; // fell at this moment, begin rampout
            else
            {
                if( m_fallRampout<(m_fallRampoutDelay/4) )
                {   // fell recently, slowly ramp upwards, decrementing rampout
                    float w = m_fallRampout / (float)(m_fallRampoutDelay/4);
                    m_dmxVal = (int)((w*m_dmxVal) + ((1.0f-w)*m_params.dmxBaseline));
                }
                m_fallRampout--;
            }
        }    
    }
    
 
    // Flashing behavior
 
    // After a domino crosses below the activation point, dmx value flashes briefly
    // Ramp the flash up and down, and decrement cooldown to allow next flash
    bool flashing = ((m_flashRampin!=0) || (m_flashRampout!=0)); 
    if( upright && !flashing ) // upright posture, not flashing
    {
        m_flashCooldown = MAX( m_flashCooldown-1, 0);
    }
    else // partially fallen posture, or flashing
    {        
        if( (m_flashCooldown<=0) && !flashing )
        {
            m_flashCooldown = m_flashCooldownDelay;
            m_flashRampin = m_flashRampinDelay;
        }
        if( m_flashRampin==1 )
        {
            m_flashRampin=0;
            m_flashRampout = m_flashRampoutDelay;
            m_dmxVal = m_params.dmxPlayModeMax;
        }
        else if( m_flashRampin>0 )
        {
            float w = (m_flashRampin/(float)m_flashRampinDelay);
            m_dmxVal = LERP( m_dmxVal, m_params.dmxPlayModeMax, 1.0f-w ); // up to 255, ramp counting down
            m_flashRampin--;
        }
        else if( m_flashRampout>0 )
        {
            float w = (m_flashRampout/(float)m_flashRampoutDelay);
            m_dmxVal = LERP( m_dmxVal, m_params.dmxPlayModeMax, w ); // down from 255, ramp counting down
            m_flashRampout--;
        }
    }


    // Pfff
    
//    //float velocAbs = abs(veloc) * fabs( angle ) / angle;
//    float velocRel = velocAbs / m_idleVelocThresh;
//    m_dmxVal += (int)( velocRel * 60 );
//    m_dmxVal = CLAMP( m_dmxVal, 0, 255 );

//    float uprightness = pow( MAX( angleAbs - m_dmxPlayModeDownThresh, 0.0f ),0.3f );
//    float velocAbs = abs(veloc);
//    float velocRel = MIN( velocAbs / m_idleVelocThresh, 1.0f );    


    // Tbthhhh
    
    //float pulseMax = 130;
    //float pulseMinW = MIN( (angleAbs<0.25f? 0.0f : (1.333f*(angleAbs-0.25f))), 1.0f );
    //float pulseMin = LERP( m_params.dmxBaseline, m_params.dmxPlayModeMax, pulseMinW );
    
    //float pulseW = (pulseSin+1.0f)*0.5f;
    //float pulse = LERP( pulseMin, pulseMax, pulseW );


    // Old adjustment function
    
    // TODO: Find better adjustment function
    //float dmxNorm = std::max( std::min( angle, 1.0f ), -1.0f ); // Range [-1,1]
    //float dmxUnit = (dmxNorm + 1.0f) / 2.0f; // Range [0,1]
    //unitVal = 1.0f - pow( (dmxUnit - 0.5f) * 4.0f, 2.0f );
    //float dmxByte = dmxUnit * (float)dmxMaximum; 
    //m_dmxVal = std::max(std::min((int)dmxByte, dmxMaximum), 10);    


//TAP MAGNITUDE VERSION 1   
    //int historyIndexOld = m_historyIndex;
    //int historyIndexNew = (m_historyIndex+1) % m_historyCount; // rotating list
	//float angle;
    
    //m_historyIndex = historyIndexNew;
    
    //// TODO: Add params to control which axis and positive or negative is up, versus which axis is the roation pivot
	//if( axis==Axis::Z )
		//angle = atan2( sensorData->acceleration.x, sensorData->acceleration.y );
	
    //float uL1 = m_params.sensorFilterAmountL1;
    //float uL2 = m_params.sensorFilterAmountL2;
    
    //m_angleFilteredL1 = ((1.0-uL1)*m_angleFilteredL1) + (uL1*angle);
    //m_angleFilteredL2 = ((1.0-uL2)*m_angleFilteredL2) + (uL2*m_angleFilteredL1);
    
    ////  >>>  Choose which filtered value to publish (L1 or L2) ...
    //m_angleFiltered = m_angleFilteredL1;
    
    //m_angleHistory[historyIndexNew] = angle;
    //m_angleFilteredHistory[historyIndexNew] = m_angleFiltered;
    
    ////float veloc = angleFilteredPrev - m_angleFiltered;
    //float veloc = RotatingListDerivative( (historyIndexNew-2), m_angleFilteredHistory );    
    //m_velocFilteredL1 = ((1.0-uL1)*m_velocFilteredL1) + (uL1*veloc);
    //m_velocFilteredL2 = ((1.0-uL2)*m_velocFilteredL2) + (uL2*m_velocFilteredL1);

    ////  >>>  Choose which filtered value to publish (L1 or L2) ...
    //m_velocFiltered = m_velocFilteredL2 * m_params.sensorVelocityMultiplier;

    //m_velocHistory[historyIndexNew] = veloc;
    //m_velocFilteredHistory[historyIndexNew] = m_velocFiltered;


    //// Hardware tap detection
    ////if( axis==Axis::X ) tap = sensorData->tap.y + sensorData->tap.z;
    ////if( axis==Axis::Y ) tap = sensorData->tap.x + sensorData->tap.z;
    ////if( axis==Axis::Z ) tap = sensorData->tap.x + sensorData->tap.y;

    
    //// Set DMX values
    //float dmxNorm = CLAMP( m_angleFiltered, -1.0f, 1.0f ); // Range [-1,1]
    
    //// TODO: Find better adjustment function
    //float dmxUnit = 1.0f - pow( dmxNorm * 2.0f, 2.0f ); // 1 when dmxNorm is zero, 0 when dmxNorm is +-0.5
    
    //float dmxByteScratch = dmxUnit * 100.0f; // Maximum range [0,255], but brightness less than maximum
    //float dmxByteTap = 255.0 * (m_tapMagnitude/10.0);
    //float dmxByte = dmxByteScratch + dmxByteTap;
    
    //m_dmxVal = CLAMP( (int)dmxByte, 10, 255 );

    
    //// old velocity entry overwritten below is our lookback velocity
    //int tapLookbackIndex = historyIndexOld + (-2*m_tapLookbackDelay);
    //float velocMin, velocMax;
    //RotatingListMinMax( velocMin, velocMax, tapLookbackIndex, m_tapLookbackDelay, m_velocHistory );
    

    //// velocity lookback - compare oldest velocity value in our history with current
    //int tappedNew = 0;
    //int tappedOld = (m_tapped!=0);
    
    //float velocDiff = fabs(velocMax-velocMin); //fabs(velocNew-velocOld);
    //if( velocDiff > m_tapLookbackThresh )
    //{
        //tappedNew = 1;
        //m_tapped = m_tapCooldown; // set collided flag on tap, stay for short duration
    //}
    //else if( m_tapped>0 )
    //{
        //m_tapped--;
    //}

    
    //// Activate and deactivate on crossing edges
    //m_activated = (velocDiff>m_idleVelocThresh? 1:0);
    ////float velocAbs = abs(velocNew);
    ////m_activated = (velocAbs>m_idleVelocThresh? 1:0);


    //// tap is nonzero only at moment of activation
    //m_tap = ( (tappedNew!=0) && (tappedOld==0)?  1 : 0 );
    
    //if( m_tap )
    //{
        //RotatingListMinMax( velocMin, velocMax, tapLookbackIndex, 2*m_tapLookbackDelay, m_velocHistory );
        //float tapMagnitude = (fabs(velocMax-velocMin) * 10.0f) + 1.0f; // min 1, max ~10
        //m_tapMagnitude += tapMagnitude; 
        ////m_tapMagnitude += ((velocDiff - m_tapLookbackThresh) * 100.0f) + 1.0f; // range 1-10
    //}
    //else
        //m_tapMagnitude -= 0.5f; // TODO: Define fade param
        
    //m_tapMagnitude = CLAMP( m_tapMagnitude, 0.0f, 10.0f );
    //if( m_tap ) printf("tap %.4f\n", m_tapMagnitude );
    
    //m_err = sensorData->err;

*/
