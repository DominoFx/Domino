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



class LIS3DH: public ISensor {
public:
    LIS3DH();
    virtual ~LIS3DH();
    
    // local methods
    static int IsAvailable( I2CBus* bus, SensorParams* params );

    // from ISensor
    int Init( I2CBus* bus, SensorParams* params );
    int Sample( I2CBus* bus );

    const SensorData* GetData();
    const SensorParams* GetParams();

    void DebugPrint();

private:

    std::mutex m_mutex;
    SensorParams m_params;
    SensorData m_data;
    int m_addr;

    struct SensorSample
    {
        SensorSample() : tap(0), count(0) {}
        uint8_t pad[1]; // TODO: annoying this structure doesn't align to 4-byte boundary, hence padding
        uint8_t err;
        uint8_t tap;
        uint8_t count;
        WVec3_t accel[32]; // LIS3DH hardware FIFO buffer has 32 entries
    };
    static const int SAMPLE_COUNT = 32;
    SensorSample m_samples[SAMPLE_COUNT];
    int m_sampleIndex;
};

#endif /* LIS3DH_H */

