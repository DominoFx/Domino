/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   I2CBus.h
 * Author: michaelson britt
 *
 * Created on June 04, 2018, 03:16 AM
 */

#ifndef I2CBUS_H
#define I2CBUS_H

#include <stdint.h>

class I2CBus
{
public:
    I2CBus();
    virtual ~I2CBus();

    int Init( const char* busDeviceName );

    // Access to general info
    int IsPresent(    uint8_t deviceID );

    // Access to the global register
    // Use for devices that have only a single register, for example the TCA9548A mux chip
    int ReadGlobal(   uint8_t deviceID, uint8_t* globalVal );
    int WriteGlobal(  uint8_t deviceID, uint8_t  globalVal );

    // Access to a single register
    int ReadSingle(   uint8_t deviceID, uint8_t regID, uint8_t* regVal );
    int WriteSingle(  uint8_t deviceID, uint8_t regID, uint8_t  regVal );
    int ToggleSingle( uint8_t deviceID, uint8_t regID, uint8_t  regVal, uint8_t bitmask );

    // Access to a block of multiple register
    int ReadMulti(    uint8_t deviceID, uint8_t regID, uint8_t bufCount, uint8_t* regBuf );

    void SetDebugOutput( bool b );

protected:
    int file;
    bool debugOutput;
};

#endif /* I2CBUS_H */

