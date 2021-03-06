/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LIS3DH.h
 * Author: simon
 *
 * Created on April 25, 2018, 12:56 AM
 */

#ifndef LIS3DH_H
#define LIS3DH_H

#include "ISensor.h"
#include <stdint.h>
#include <mutex>
#include <chrono>

// 
// Classes
// 

class DominoController;
class DominoParams;


// 
// Class LIS3DH
// Motion sensor
// 

class LIS3DH: public ISensor {
public:
    typedef std::chrono::time_point<std::chrono::system_clock> TimePoint;
    typedef std::chrono::duration<double> Duration;
    
    LIS3DH( DominoController& context );
    virtual ~LIS3DH();
    
    // local methods
    static int IsAvailable( I2CBus* bus, SensorAddress* address );

    // from ISensor
    int Init( I2CBus* bus, SensorAddress* params );
    int Sample( I2CBus* bus );

    const SensorData* GetData();
    const SensorAddress* GetAddress();

    void DebugPrint();

private:
    DominoController& m_context;
    DominoParams& m_params;
    TimePoint m_lastPrint;
    int m_lastReadCount;
    int m_lastSampleCount;

    std::mutex m_mutex;
    SensorAddress m_address;
    SensorData m_data;
    int m_readCount;
    int m_sampleCount;
    int m_sampleQueue;

    struct SensorSample
    {
        SensorSample() : err(0), tap(0), count(0) {}
        //uint8_t pad[1]; // TODO: annoying this structure doesn't align to 4-byte boundary, hence padding
        int err; //uint8_t err;
        int tap; //uint8_t tap;
        int count; //uint8_t count;
        int avail;
        WVec3_t accel[32]; // LIS3DH hardware FIFO buffer has 32 entries
    };
    static const int SAMPLE_COUNT = 32;
    SensorSample m_samples[SAMPLE_COUNT];
    int m_sampleIndex;
};

#endif /* LIS3DH_H */

