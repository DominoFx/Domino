/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ISensor.h
 * Author: simon
 *
 * Created on April 25, 2018, 12:35 AM
 */

#ifndef ISENSOR_H
#define ISENSOR_H

#include "enums.h"
#include "I2CBus.h"
#include "vec3.h"

struct SensorParams
{
    // when using TCA9548A mux chip
    int muxAddress;
    int muxField; // equal to (1<<index)

    // calibration for tap impact detection
    int tapThresh;
    int tapTimeLimit;
    int tapTimeLatency;
    int tapTimeWindow;
};

class SensorData
{
public:
    SensorData()
    {}

    FVec3_t acceleration;
    FVec3_t velocity;
    FVec3_t position;
    WVec3_t tap;  // X, Y, Z ... nonzero if tap detected
};

class ISensor
{
public:
    ISensor(){};
    virtual ~ISensor(){};
    
    virtual int Init( I2CBus* bus, SensorParams* params ) = 0;
    virtual int Sample( I2CBus* bus ) = 0;

    virtual const SensorData* GetData() = 0;
    virtual const SensorParams* GetParams() = 0;

    virtual void DebugPrint() {}
};

#endif /* ISENSOR_H */

