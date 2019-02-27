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

#include "common.h"
#include "I2CBus.h"
#include "vec3.h"

struct SensorAddress
{
    SensorAddress()
    { index=0, address=0, muxWhich=0, muxAddress=0, muxField=0; }
    
    int index;
    int address;
    
    // when using TCA9548A mux chip
    int muxWhich;
    int muxAddress;
    int muxField; // equal to (1<<index)
};

struct SensorData
{
    SensorData()
    {}

    uint8_t pad[1]; // TODO: annoying this structure doesn't align to 4-byte boundary, hence padding
    uint8_t err;
    //float   angle;
    FVec3_t acceleration;
    //FVec3_t velocity;
    //FVec3_t position;
    WVec3_t tap;  // X, Y, Z ... nonzero if tap detected
};

class ISensor
{
public:
    ISensor(){};
    virtual ~ISensor(){};
    
    virtual int Init( I2CBus* bus, SensorAddress* address ) = 0;
    virtual int Sample( I2CBus* bus ) = 0;

    virtual const SensorData* GetData() = 0;
    virtual const SensorAddress* GetAddress() = 0;

    virtual void DebugPrint() {}
};

#endif /* ISENSOR_H */

