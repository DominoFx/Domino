/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BMA220.h
 * Author: simon
 *
 * Created on April 25, 2018, 12:19 AM
 */

#ifndef BMA220_H
#define BMA220_H

#include "ISensor.h"
#include <stdint.h>
#include <mutex>

class BMA220: public ISensor {
public:
    BMA220();
    virtual ~BMA220();

    // local methods
    static int IsAvailable( I2CBus* bus, SensorParams* params );
    
    // from ISensor
    int Init( I2CBus* bus, SensorParams* params ); 
    int Sample( I2CBus* bus );
    const SensorData* GetData();
    const SensorParams* GetParams();
    void DebugPrint();

private:
    double Realdata (int data);

    std::mutex m_mutex;
    SensorParams m_params;
    SensorData m_data;

    struct SensorSample
    {
        SensorSample() : tap(0), count(0) {}
        uint8_t tap;
        WVec3_t accelSum;
        int count;
    };
    SensorSample m_sample;
};

#endif /* BMA220_H */

