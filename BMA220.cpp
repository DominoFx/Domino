/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BMA220.cpp
 * Author: simon
 * 
 * Created on April 25, 2018, 12:19 AM
 */

#include "BMA220.h"
#include "dmx/DominoParams.h"
#include <stdio.h>
#include <sys/ioctl.h>

#define BMA220_ADDRESS_I2C     0xA

#define BMA220_REG_X           0x04
#define BMA220_REG_Y           0x06
#define BMA220_REG_Z           0x08

#define BMA220_REG_WHO_AM_I    0x00

BMA220::BMA220(DominoParams& params) :
  m_params(params)
{
}

BMA220::~BMA220() {
}

int BMA220::IsAvailable( I2CBus* bus, SensorAddress* address )
{
    int result = 0;

    address->address = BMA220_ADDRESS_I2C;

    // Toggle the mux to the correct device
    bus->WriteGlobal( address->muxAddress, address->muxField );

    // Check the mux toggled correctly
    uint8_t muxField_check;
    bus->ReadGlobal( address->muxAddress, &muxField_check );
    if( muxField_check != address->muxField )
    {
        printf( "ERROR: BMA220::IsPresent() mux check got %i, expected %i\n",
            (int)(muxField_check), (int)(address->muxField) );
        result = 1; // error flag
    }

    // Doesn't work as expected, seems to always returns 1
    int isPresent = bus->IsPresent( address->address );
    if( isPresent==0 )
    {
        result = 2; // error flag
    }

    // Read the useless WhoAmI register ... TWICE ... for confirmation
    uint8_t whoAmI;
    bus->ReadSingle( address->address, BMA220_REG_WHO_AM_I, &whoAmI ); // first read fails, BMA220 bug
    bus->ReadSingle( address->address, BMA220_REG_WHO_AM_I, &whoAmI );
    if( whoAmI != 0xDD )
    {
        result = 3; // error flag
    }

    // Toggle the mux off
    bus->WriteGlobal( address->muxField, 0 );

    return (result==0? 1:0);
}

int BMA220::Init( I2CBus* bus, SensorAddress* address )
{
    int result = 0;

    m_address = (*address);

    m_address.address = BMA220_ADDRESS_I2C;


    // Toggle the mux to the correct device
    bus->WriteGlobal( m_address.muxAddress, m_address.muxField );

    // Check the mux toggled correctly
    uint8_t muxField_check;
    bus->ReadGlobal( m_address.muxAddress, &muxField_check );
    if( m_address.muxField != muxField_check )
    {
        printf( "ERROR: BMA220::Init() mux check got %i, expected %i\n",
            (int)(muxField_check), (int)(m_address.muxField) );
        result = 1; // error flag
    }

    // Read the useless WhoAmI register just to make sure we're synchronized
    uint8_t whoAmI;
    bus->ReadSingle( m_address.address, BMA220_REG_WHO_AM_I, &whoAmI );
    if( whoAmI != 0xDD )
    {
        printf( "ERROR: 0x%X <- BMA220::Init() WhoAmI, expected 0xDD\n", (int)whoAmI );
        result = 2; // error flag
    }

    // Toggle the mux off
    bus->WriteGlobal( m_address.muxAddress, 0 );

    printf( "DominoFX: Initialized BMA220 motion sensor at index %i, %s ... \n",
        m_address.index, (result==0?"ok":"failed") );

    return result;
}

int BMA220::Sample( I2CBus* bus )
{
    int result = 0;

    // Lock, only one thread may have access
    std::lock_guard<std::mutex> lock(m_mutex);

    // Toggle the mux to the correct device 
    bus->WriteGlobal( m_address.muxAddress, m_address.muxField );

    // Read the useless WhoAmI register to ensure we're synchronized
    // This is required due to apparent hardware glitch with the BMA220
    uint8_t whoAmI;
    bus->ReadSingle( m_address.address, BMA220_REG_WHO_AM_I, &whoAmI );

    uint8_t dataByte;
    bus->ReadSingle( m_address.address, BMA220_REG_X, &dataByte );
    m_sample.accelSum.x += dataByte;

    bus->ReadSingle( m_address.address, BMA220_REG_Y, &dataByte );
    m_sample.accelSum.y += dataByte;

    bus->ReadSingle( m_address.address, BMA220_REG_Z, &dataByte );
    m_sample.accelSum.z += dataByte;

    m_sample.count += 1;
    
    // Toggle the mux off
    bus->WriteGlobal( m_address.muxAddress, 0 );

    return result;
}

const SensorData* BMA220::GetData()
{
    // Lock, only one thread may have access
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if( m_sample.count>0 )
    {
        // Hold previous position value; needed below
        DVec3_t accelOld(m_data.acceleration);

        // Set acceleration value
        DVec3_t accelVal;
        accelVal.x = Realdata(m_sample.accelSum.x / m_sample.count);
        accelVal.y = Realdata(m_sample.accelSum.y / m_sample.count);
        accelVal.z = Realdata(m_sample.accelSum.z / m_sample.count);
        m_sample.accelSum.x = m_sample.accelSum.y = m_sample.accelSum.z = 0;
        m_sample.count = 0;

        m_data.acceleration = accelVal;

        // Set position value
        // TODO: Calculate position from acceleration, with gravity compensation
        //m_data.position = accelVal;

        // Set velocity value; difference from current to previous value
        DVec3_t accelDif(accelVal);
        accelDif -= accelOld;
        DVec3_t velocity = accelDif; // Velocity is delta between current and previous pos

        // Set tap flags
        m_data.tap.x = (velocity.x > 0.01?  1 : 0); // TODO: HACK, arbitrary threshold
        m_data.tap.y = (velocity.y > 0.01?  1 : 0); // TODO: HACK, arbitrary threshold
        m_data.tap.z = (velocity.z > 0.01?  1 : 0); // TODO: HACK, arbitrary threshold
    }
    
    return &m_data;
}

const SensorAddress* BMA220::GetAddress()
{
    return &m_address;
}

void BMA220::DebugPrint()
{

    // Lock, only one thread may have access
    std::lock_guard<std::mutex> lock(m_mutex);

    WVec3_t& v = m_sample.accelSum;
    printf( "  BMA220 - Mux 0x%X, Slot 0x%X\n", m_address.muxAddress, m_address.muxField );
    printf( "  Queue:\t\t %i\n", (int)(m_sample.count) );
    printf( "  Accel avg:\t\t (%i,%i,%i)\n", (int)(v.x), (int)(v.y), (int)(v.z) );
}

double BMA220_RealData( int data )
{
    double x,d;
    double nfactor = 1;
    double slope = 0.0625;//  1.94/31;    sensitivity   +/- 2g range 
    int dat;

    data = data >> 2;
	
    if (data > 31)
    {          //MSB represents a negative number    
       dat = ~data + 1;      //2s compliment make it negative
       dat = dat & 0b011111; //bit mask anded to make positive 
       nfactor = -1;
    }
    else
    {
      dat = data; 
    }

    d =  (double)dat;  
    //x = d*slope*9.81*nfactor;            
    x = d*slope*nfactor;

    x = x / 2.0f; // TODO: HACK, scales values to similar range as LIS3DH 
    if( x < -1 ) x = -1; //TODO: HACK, clipping, may be unnecessary
    if( x >  1 ) x =  1;

    return x;  
}

double BMA220::Realdata (int data)
{
    return BMA220_RealData(data);
}
