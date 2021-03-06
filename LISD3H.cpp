/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LIS3DH.cpp
 * Author: simon
 * 
 * Created on April 25, 2018, 12:56 AM
 */

#include "LISD3H.h"
#include "dmx/DominoController.h"
#include "dmx/DominoParams.h"
#include <stdio.h>
#include <thread>
#include <chrono>
#include <math.h>


#define LIS3DH_ADDRESS_1_I2C     0x18 // if the SDO pin is low
#define LIS3DH_ADDRESS_2_I2C     0x19 // if the SDO pin is high
#define LIS3DH_REG_STATUS_AUX    0x07 // accelerometer data status
#define LIS3DH_REG_WHO_AM_I      0x0F // always returns 0x33
#define LIS3DH_REG_TEMP_CFG_REG  0x1F // temperature and ADC enable
#define LIS3DH_REG_CTRL_REG1     0x20 // accelerometer data rate, low-power mode, and axis enables
#define LIS3DH_REG_CTRL_REG2     0x21 // accelerometer high-pass filter
#define LIS3DH_REG_CTRL_REG3     0x22 // accelerometer interrupt control
#define LIS3DH_REG_CTRL_REG4     0x23 // accelerometer scale selection 2G,4G,8G,16G
#define LIS3DH_REG_CTRL_REG5     0x24 // accelerometer latch and 4D enable
#define LIS3DH_REG_STATUS_REG    0x27 // accelerometer data status
#define LIS3DH_REG_OUT_X_L       0x28 // accelerometer X low byte
#define LIS3DH_REG_OUT_X_H       0x29 // accelerometer X high byte
#define LIS3DH_REG_OUT_Y_L       0x2A // accelerometer Y low byte
#define LIS3DH_REG_OUT_Y_H       0x2B // accelerometer Y high byte
#define LIS3DH_REG_OUT_Z_L       0x2C // accelerometer Z low byte
#define LIS3DH_REG_OUT_Z_H       0x2D // accelerometer Z high byte
#define LIS3DH_REG_FIFO_CTRL_REG 0x2E // first-in-first-out buffer control
#define LIS3DH_REG_FIFO_SRC_REG  0x2F // first-in-first-out buffer status
#define LIS3DH_REG_CLICK_CFG     0x38 // tap and double-tap config
#define LIS3DH_REG_CLICK_SRC     0x39 // tap and double-tap read
#define LIS3DH_REG_CLICK_THS     0x3A // tap and double-tap config
#define LIS3DH_REG_TIME_LIMIT    0x3B // tap and double-tap time limit (meaning?)
#define LIS3DH_REG_TIME_LATENCY  0x3C // tap and double-tap time latency (meaning?)
#define LIS3DH_REG_TIME_WINDOW   0x3D // tap and double-tap time window (meaning?)

#define LIS3DH_DEFAULT_TAPTHRESH       80
#define LIS3DH_DEFAULT_TAPTIMELIMIT    10
#define LIS3DH_DEFAULT_TAPTIMELATENCY  20
#define LIS3DH_DEFAULT_TAPTIMEWINDOW   255

// Control register 1 - Axis selection
#define LIS3DH_AXIS_BITMASK           0x07 // bitmask for this value
#define LIS3DH_AXIS_X                 0x01
#define LIS3DH_AXIS_Y                 0x02
#define LIS3DH_AXIS_Z                 0x04
#define LIS3DH_AXIS_XYZ               0x07

// Control register 1 - Data Rate
#define LIS3DH_DATARATE_BITMASK       0xF0 // bitmask for this value
#define LIS3DH_DATARATE_1344HZ        0x90
#define LIS3DH_DATARATE_400HZ         0x70
#define LIS3DH_DATARATE_200HZ         0x60
#define LIS3DH_DATARATE_100HZ         0x50
#define LIS3DH_DATARATE_50HZ          0x40
#define LIS3DH_DATARATE_25HZ          0x30
#define LIS3DH_DATARATE_10HZ          0x20
#define LIS3DH_DATARATE_1HZ           0x10

// Control Register 3 - Data Ready 1 signal (DRDY1)
#define LIS3DH_DRDY1_BITMASK          0x10 // bitmask for this value
#define LIS3DH_DRDY1_ENABLE           0x10

// Control Register 3 - Data Ready 2 signal (DRDY2)
#define LIS3DH_DRDY2_BITMASK          0x08 // bitmask for this value
#define LIS3DH_DRDY2_ENABLE           0x08

// Control Register 3 - Int1 Tap Interrupt
#define LIS3DH_INT1TAP_BITMASK        0x80 // bitmask for this value
#define LIS3DH_INT1TAP_ENABLE         0x80

// Control Register 4 - Block Data Update
#define LIS3DH_BDU_BITMASK            0x80 // bitmask for this value
#define LIS3DH_BDU_ENABLE             0x80

// Control Register 4 - Block Data Update
#define LIS3DH_HIGHRES_BITMASK        0x08 // bitmask for this value
#define LIS3DH_HIGHRES_ENABLE         0x08

// Control register 4 - Acceleration Range (sensitivity)
#define LIS3DH_ACCELRANGE_BITMASK     0x30 // bitmask for this value
#define LIS3DH_ACCELRANGE_2G          0x00
#define LIS3DH_ACCELRANGE_4G          0x10
#define LIS3DH_ACCELRANGE_8G          0x20
#define LIS3DH_ACCELRANGE_16G         0x30

// Control Register 5 - Int1 Interrupt Latch
#define LIS3DH_INT1LATCH_BITMASK      0x08 // bitmask for this value
#define LIS3DH_INT1LATCH_ENABLE       0x08

// Control Register 5 - First-In-First-Out buffer enable
#define LIS3DH_FIFO_BITMASK           0x40
#define LIS3DH_FIFO_ENABLE            0x40

// Temperature and ADC register - Analog-to-Digital (ADC) enable
#define LIS3DH_ADC_BITMASK            0x80 // bitmask for this value
#define LIS3DH_ADC_ENABLE             0x80

// Status register (read-only)
#define LIS3DH_STATUS_NEWDATAX        0x01
#define LIS3DH_STATUS_NEWDATAY        0x02
#define LIS3DH_STATUS_NEWDATAZ        0x04
#define LIS3DH_STATUS_NEWDATAXYZ      0x07

// First-In-First-Out (FIFO) control - Mode select
#define LIS3DH_FIFO_MODE_BITMASK      0xC0
#define LIS3DH_FIFO_MODE_BYPASS       0x00
#define LIS3DH_FIFO_MODE_FIFO         0x40
#define LIS3DH_FIFO_MODE_STREAM       0x80
#define LIS3DH_FIFO_MODE_STREAMFIFO   0xC0

// First-In-First-Out (FIFO) status - Sample count
#define LIS3DH_FIFO_SAMPLES_BITMASK   0x1F

// Tap config register - Single tap enable
#define LIS3DH_SINGLETAP_BITMASK      0x15
#define LIS3DH_SINGLETAP_X            0x01
#define LIS3DH_SINGLETAP_Y            0x04
#define LIS3DH_SINGLETAP_Z            0x10
#define LIS3DH_SINGLETAP_XYZ          0x15

// Tap source register (read-only)
#define LIS3DH_TAP_DETECTINT          0x40
#define LIS3DH_TAP_DETECTX            0x01
#define LIS3DH_TAP_DETECTY            0x02
#define LIS3DH_TAP_DETECTZ            0x04
#define LIS3DH_TAP_DETECTXYZ          0x07


LIS3DH::LIS3DH( DominoController& context ):
  m_context(context)
, m_params(context.GetParams())
, m_readCount(0)
, m_sampleCount(0)
, m_sampleIndex(0)
{
    // HACK: Ensure some initial data appears populated during first GetData() call
    m_samples[0].count = 10;
}


LIS3DH::~LIS3DH() {
}

int LIS3DH::IsAvailable( I2CBus* bus, SensorAddress* address )
{
    //printf("LIS3DH::IsAvailable() \n" );
    int result = 0;

    uint8_t addressList[] = {LIS3DH_ADDRESS_1_I2C,LIS3DH_ADDRESS_2_I2C};

    // Toggle the mux to the correct device
    bus->WriteGlobal( address->muxAddress, address->muxField );
    //printf("LIS3DH::IsAvailable() WriteGlobal() 0x%X : 0x%X, result=%i... \n", muxAddress, muxField, result);

    // Check the mux toggled correctly
    uint8_t muxField_check;
    bus->ReadGlobal( address->muxAddress, &muxField_check );
    if( muxField_check != address->muxField )
    {
        printf( "ERROR: LIS3DH::IsPresent() mux check got %i, expected %i\n",
            (int)(muxField_check), (int)(address->muxField) );
        result = 1; // error flag
    }

    bus->SetDebugOutput( false );
    int resultPrev = result;
    for( int i=0; (i<2) && (address->address==0); i++ )
    {
        result = resultPrev;
        // Doesn't work as expected, seems to always returns 1
        int isPresent = bus->IsPresent( addressList[i] );
        if( isPresent==0 )
        {
            result = 2; // error flag
        }
        else
        {
            // Read the useless WhoAmI register for confirmation
            int whoAmI=0; //uint8_t whoAmI;
            bus->ReadSingle( addressList[i], LIS3DH_REG_WHO_AM_I, (uint8_t*)&whoAmI );
            //printf("LIS3DH::IsAvailable() ReadSingle() addr=0x%x, whoAmI=0x%X... \n", (int)(addrList[i]), (int)whoAmI);
            
            if( whoAmI != 0x33 )
            {
                result = 3; // error flag
            }
        }
        
        if( result==0 )
            address->address = addressList[i];
    }
    bus->SetDebugOutput( true );

    // Toggle the mux off
    bus->WriteGlobal( address->muxAddress, 0 );

    return (result==0? 1:0);
}

int LIS3DH::Init( I2CBus* bus, SensorAddress* address )
{
    //printf("LIS3DH::Init() \n");
    int result = 0;

    m_address = (*address);
    
    m_lastPrint = std::chrono::system_clock::now();
    m_lastReadCount = 0;
    m_lastSampleCount = 0;
    m_readCount = 0;
    m_sampleCount = 0;
    m_sampleQueue = m_params.sensorHWQueue;

    // Toggle unused mux chips off
    bus->WriteGlobal( 0x70, 0x0 ); // TODO: HACK add better handling for dual mux
    bus->WriteGlobal( 0x72, 0x0 ); // TODO: HACK add better handling for dual mux
    
    // Toggle the mux to the correct device 
    bus->WriteGlobal( m_address.muxAddress, m_address.muxField );

    // Check the mux toggled correctly
    uint8_t muxField_check;
    bus->ReadGlobal( m_address.muxAddress, &muxField_check );
    if( m_address.muxField != muxField_check )
    {
        printf( "ERROR: LIS3DH::Init() mux check got %i, expected %i\n",
            (int)(muxField_check), (int)(m_address.muxField) );
        result = 1; // error flag
    }

    uint8_t addressList[] = {LIS3DH_ADDRESS_1_I2C,LIS3DH_ADDRESS_2_I2C};
    int whoAmI=0; //uint8_t whoAmI;
    bus->SetDebugOutput( false );
    int resultPrev = result;
    for( int i=0; (i<2) && (m_address.address==0); i++ )
    {
        result = resultPrev;
        // Read the useless WhoAmI register to make sure we're synchronized
        bus->ReadSingle( addressList[i], LIS3DH_REG_WHO_AM_I, (uint8_t*)&whoAmI );
        if( whoAmI != 0x33 )
        {
            result = 2; // error flag
        }
        else m_address.address = addressList[i];
    }
    bus->SetDebugOutput( true );
    if( result==2 )
        printf( "ERROR: 0x%X <- LIS3DH::Init() WhoAmI, expected 0x33\n", (int)whoAmI );
    
    // Set Axis
    bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG1, LIS3DH_AXIS_XYZ, LIS3DH_AXIS_BITMASK );

    // Set Data Rate
    if( m_params.sensorHWSamplesPerSec == 100 ) // Set corresponding LIS3DH_DATARATE_100HZ ...
        bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG1, LIS3DH_DATARATE_100HZ, LIS3DH_DATARATE_BITMASK );
    else if( m_params.sensorHWSamplesPerSec == 200 ) // Set corresponding LIS3DH_DATARATE_200HZ ...
        bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG1, LIS3DH_DATARATE_200HZ, LIS3DH_DATARATE_BITMASK );
    else if( m_params.sensorHWSamplesPerSec == 400 ) // Set corresponding LIS3DH_DATARATE_400HZ ...
        bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG1, LIS3DH_DATARATE_400HZ, LIS3DH_DATARATE_BITMASK );
    else if( m_params.sensorHWSamplesPerSec == 1344 ) // Set corresponding LIS3DH_DATARATE_1344HZ ...
        bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG1, LIS3DH_DATARATE_1344HZ, LIS3DH_DATARATE_BITMASK );

    // Enable Data Ready 1 signal
    bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG3, LIS3DH_DRDY1_ENABLE, LIS3DH_DRDY1_BITMASK );

    // Enable Int1 Tap interrupt, we won't use interrupt functionality though
    bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG3, LIS3DH_INT1TAP_ENABLE, LIS3DH_INT1TAP_BITMASK );

    // Enable High Resolution
    bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG4, LIS3DH_HIGHRES_ENABLE, LIS3DH_HIGHRES_BITMASK );

    // Enable Block Data Update
    bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG4, LIS3DH_BDU_ENABLE, LIS3DH_BDU_BITMASK );

    // Set Accel Range (sensitivity)
    if( m_params.sensorHWAccelRange==2 )
        bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG4, LIS3DH_ACCELRANGE_2G, LIS3DH_ACCELRANGE_BITMASK );
    else if( m_params.sensorHWAccelRange==4 )
        bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG4, LIS3DH_ACCELRANGE_4G, LIS3DH_ACCELRANGE_BITMASK );
    else if( m_params.sensorHWAccelRange==8 )
        bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG4, LIS3DH_ACCELRANGE_8G, LIS3DH_ACCELRANGE_BITMASK );
    else if( m_params.sensorHWAccelRange==16 )
        bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG4, LIS3DH_ACCELRANGE_16G, LIS3DH_ACCELRANGE_BITMASK );

    // Enable Int1 Latching for interrupt, we won't use interrupt functionality though
    bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG5, LIS3DH_INT1LATCH_ENABLE, LIS3DH_INT1LATCH_BITMASK);

    // Enable Analog-to-Digital converter (ADC)
    bus->ToggleSingle( m_address.address, LIS3DH_REG_TEMP_CFG_REG, LIS3DH_ADC_ENABLE, LIS3DH_ADC_BITMASK);

    // Enable Single-Tap detection
    bus->ToggleSingle( m_address.address, LIS3DH_REG_CLICK_CFG, LIS3DH_SINGLETAP_XYZ, LIS3DH_SINGLETAP_BITMASK );

    // Set the Single-Tap timing and sensitivity params
    bus->WriteSingle( m_address.address, LIS3DH_REG_CLICK_THS,    m_params.sensorHWTapThresh );
    bus->WriteSingle( m_address.address, LIS3DH_REG_TIME_LIMIT,   m_params.sensorHWTapTimeLimit );
    bus->WriteSingle( m_address.address, LIS3DH_REG_TIME_LATENCY, m_params.sensorHWTapTimeLatency );
    bus->WriteSingle( m_address.address, LIS3DH_REG_TIME_WINDOW,  m_params.sensorHWTapTimeWindow );

    // Enable First-In-First-Out (FIFO) buffering
    bus->ToggleSingle( m_address.address, LIS3DH_REG_CTRL_REG5, LIS3DH_FIFO_ENABLE, LIS3DH_FIFO_BITMASK );
    bus->ToggleSingle( m_address.address, LIS3DH_REG_FIFO_CTRL_REG, LIS3DH_FIFO_MODE_STREAM, LIS3DH_FIFO_MODE_BITMASK );

    // Toggle the mux off
    bus->WriteGlobal( m_address.muxAddress, 0 );

    printf( "DominoFX: Initialized LIS3DH motion sensor at index %i addres %X, %s ... \n",
        m_address.index, m_address.address, (result==0?"ok":"failed") );

    return result;
}

int LIS3DH::Sample( I2CBus* bus )
{
    bus->SetDebugOutput( m_context.GetDebugDiagStream() );
    
    int result = 0, err = 0;

    // Toggle the mux to the correct device
    err = bus->WriteGlobal( m_address.muxAddress, m_address.muxField );
    if( err!=0 ) result |= 2; // bit 2 for mux error
    
    // Read number of data samples available
    uint8_t count = 0;
    //if( aux & 0x08 ) ... // would ensure new data is available before reading count
    
    // TODO: Why is this returning an I2C error very frequently?
    err = bus->ReadSingle( m_address.address, LIS3DH_REG_FIFO_SRC_REG, (uint8_t*)&count );
    if( err!=0 ) result |= 1; // bit 1 for sensor error
    count = count & LIS3DH_FIFO_SAMPLES_BITMASK;

    // Read samples if any are available
    if( count>0 )
    {
        // Fetch data sample from our bank
        m_sampleIndex = (m_sampleIndex+1) % SAMPLE_COUNT;

        // Lock, only one thread may have access to m_samples[]
        std::lock_guard<std::mutex> lock(m_mutex);

        SensorSample& sample = m_samples[m_sampleIndex];
        sample.avail = count;
        
        // Optimization; if too many samples waiting in the fifo buffer
        // don't try to fetch them all right now, otherwise chain reaction
        // putting us further and further behind schedule with other sensors.
        // Instead, only fetch enough for the main thread to use right now,
        // catch up on next iteration.
        if( count > m_sampleQueue )
            count = m_sampleQueue;

        sample.count = count;

        // Add 0x80 to register address to enable auto-increment during read
        uint8_t reg = LIS3DH_REG_OUT_X_L | 0x80;
        
        err = bus->ReadMulti( m_address.address, reg, 6*sample.count, (uint8_t*)(sample.accel) );
        if( err!=0 ) result |= 1; // bit 1 for sensor error

        // Read tap status bits
        err = bus->ReadSingle( m_address.address, LIS3DH_REG_CLICK_SRC, (uint8_t*)&sample.tap );
        if( err!=0 ) result |= 1; // bit 1 for sensor error

        m_readCount += 1;
        m_sampleCount += count;
    }

    // Toggle the mux off
    err = bus->WriteGlobal( m_address.muxAddress, 0x00 );
    if( err!=0 ) result |= 2; // bit 2 for mux error

    if( count>0 )
    {
        SensorSample& sample = m_samples[m_sampleIndex];
        sample.err |= result;
        //if( sample.count!=count )
        //    printf("wonky count value\n");
        if( (result!=0) && (m_context.GetDebugDiagStream()) )
            printf("^^^ LIS3DH::Sample() [%X : %X] \n", m_address.muxAddress, m_address.muxField );
    }
    else
    {
        if( (result!=0) && (m_context.GetDebugDiagStream()) )
            printf("i2c_err_mux %i, channel 0x%X:0x%X ", result, (int)(m_address.muxAddress), (int)(m_address.muxField) );
        //else printf("no_sensor_data %i, channel 0x%X:0x%X ", result, (int)(m_address.muxAddress), (int)(m_address.muxField) );
    }

    return result;
}

const SensorData* LIS3DH::GetData()
{
    //return &m_data;
    
    // Target latency window of 50ms
    // Determine number of samples in window and average them
//    int accumCount = 1;
    int accumCount = ceilf(m_params.sensorHWSamplesPerSec / (float)m_params.continuousFPS);
//printf("continuousFPS %f, accumCount %i\n",(float)(m_params.continuousFPS),(int)accumCount);

    DVec3_t accumAccel;
    //double  accumAngle = 0;
    double accumWeight = 0.0;
    double weight = 1.0;
    double weightScale = (1.0 - m_params.sensorFilterAmountL1);
    uint8_t tap = 0;
    uint8_t err = 0;

    // Critical section
    {
        // Lock, only one thread may have access to m_samples[]
        std::lock_guard<std::mutex> lock(m_mutex);

        // Find starting point for accumulation process
        int bufIndex=0, sampleIndex=m_sampleIndex;
        for( int accumRemain = accumCount; accumRemain>0; )
        {
            SensorSample& sample = m_samples[sampleIndex];
            if( sample.count>=accumRemain )
            {
                bufIndex = sample.count - accumRemain;
                accumRemain = 0;
            }
            else
            {
                sampleIndex = (sampleIndex==0? (SAMPLE_COUNT-1) : (sampleIndex-1)); // non-negative modulo
                accumRemain = accumRemain - sample.count;
            }
        }

        // Sum values within the time window        
        for( int accumRemain = accumCount; accumRemain>0; )
        {
            while( bufIndex >= m_samples[sampleIndex].count )
            {
                sampleIndex = (sampleIndex+1) % SAMPLE_COUNT;
                bufIndex = 0;
            }
            SensorSample& sample = m_samples[sampleIndex];
            DVec3_t accel( sample.accel[bufIndex] );
            accel *= weight;
            accumWeight += weight;
            weight *= weightScale;
            //if( axis==Axis::Z )
            //    accumAngle = atan2( accel.x, accel.y );
            accumAccel += accel;
            tap |= sample.tap;
            err |= sample.err;
            sample.tap = 0; // clear bits, avoid processing same tap twice
            sample.err = 0; // clear bits, avoid processing same err twice
            bufIndex++;
            accumRemain--;
        }
    } // Critical section end

    // Divide by total weight for weighted averaged value
    accumAccel /= accumWeight;
    // Divide by range for final averaged normalized value
    accumAccel /= 32768.0;

    // Set acceleration value
    m_data.acceleration = accumAccel;

    // Set tap flags
    m_data.tap.x = (tap & LIS3DH_TAP_DETECTX?  1 : 0);
    m_data.tap.y = (tap & LIS3DH_TAP_DETECTY?  1 : 0);
    m_data.tap.z = (tap & LIS3DH_TAP_DETECTZ?  1 : 0);

    m_data.err = err;

    return &m_data;
}

const SensorAddress* LIS3DH::GetAddress()
{
    return &m_address;
}

#define MINMAXSUM( val, min, max, sum ) \
    (sum)+=(val); if((val)<(min)) (min)=(val); if((val)>(max)) (max)=(val);
    
void LIS3DH::DebugPrint()
{
    // Lock, only one thread may have access
    std::lock_guard<std::mutex> lock(m_mutex);

    TimePoint now = std::chrono::system_clock::now();
    Duration elapsedPrint = now - m_lastPrint;        

    int minQueue=32767, maxQueue=-32768, sumQueue=0;
    int minAvail=32767, maxAvail=-32768, sumAvail=0;
    int minX=32767,maxX=-32768,sumX=0;
    int minY=32767,maxY=-32768,sumY=0;
    int minZ=32767,maxZ=-32768,sumZ=0;

    for( int sampleIndex=0; sampleIndex<SAMPLE_COUNT; sampleIndex++ )
    {
        SensorSample& sample = m_samples[sampleIndex];
        MINMAXSUM( sample.count, minQueue, maxQueue, sumQueue );
        MINMAXSUM( sample.avail, minAvail, maxAvail, sumAvail );
        for( int bufIndex=0; bufIndex<sample.count; bufIndex++ )
        {
            WVec3_t& v = sample.accel[bufIndex];
            //printf( "  LIS3DH - Sample %i.%i = [%i,%i,%i]\n",
            //    sampleIndex, bufIndex, v.x, v.y, v.z );
            MINMAXSUM( v.x, minX, maxX, sumX );
            MINMAXSUM( v.y, minY, maxY, sumY );
            MINMAXSUM( v.z, minZ, maxZ, sumZ );
        }
    }
    WVec3_t newest = m_samples[m_sampleIndex].accel[0];
    
    //printf( "  LIS3DH - Mux 0x%X, Slot 0x%X, this 0x%X\n", m_address.muxAddress, m_address.muxField, (int)this );
    //printf( "  Read count %i\n", (int)m_readCount );
    printf( "  Queue avg:\t\t %f min:max [%i:%i]\n", (float)(sumQueue/(float)SAMPLE_COUNT), (int)minQueue, (int)maxQueue );
    printf( "  Avail avg:\t\t %f min:max [%i:%i]\n", (float)(sumAvail/(float)SAMPLE_COUNT), (int)minAvail, (int)maxAvail );
    printf( "  Accel cur:\t\t (%i,%i,%i)\n", (int)newest.x, (int)newest.y, (int)newest.z );
    printf( "  Accel avg:\t\t (%.1f,%.1f,%.1f) delta (%i,%i,%i)\n",
        (float)(sumX/(float)sumQueue), (float)(sumY/(float)sumQueue), (float)(sumZ/(float)sumQueue) );
    
    //printf( "  Accel avg:\t\t (%.1f,%.1f,%.1f) delta (%i,%i,%i)\n",
    //    (float)(sumX/(float)sumQueue), (float)(sumY/(float)sumQueue), (float)(sumZ/(float)sumQueue),
    //    (int)(maxX-minX), (int)(maxY-minY), (int)(maxZ-minZ) );
    
    //printf( "---------- ----------\n" );
    int readCount = m_readCount - m_lastReadCount;
    printf("  Reads per second:\t %.3f\n", (float)(readCount/elapsedPrint.count()) );
    //printf( "---------- ----------\n" );
    int sampleCount = m_sampleCount - m_lastSampleCount;
    printf("  Samples per second:\t %.3f\n", (float)(sampleCount/elapsedPrint.count()) );

    m_lastPrint = std::chrono::system_clock::now();
    m_lastReadCount = m_readCount;
    m_lastSampleCount = m_sampleCount;
}

//[ /domino,
//  0.010683200322092, 0.123046875, -0.0011820531217381,
//  0.01171875, 0.12186482548714, -0.001953125,
//  0, 0.11874737590551 -0.001953125,
// -0.00390625, 0.1171875, -0.00390625
// 0.013671875, 0.119140625, 0.00390625, -0.00390625, 0.1217360496521, 0, 0.001953125, 0.11641642451286, -0.0015752997715026, 0.032663375139236, 0.111328125,
// -0.0024928753264248, 0, 0.11795857548714, -0.00077107187826186
// 1.1962749560598e+22, 5.7987830888065e+22, 6.7120162658453e+22/ /
