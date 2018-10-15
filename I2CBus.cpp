/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   I2CBus.cpp
 * Author: michaelson
 *
 * Created on June 04, 2018, 03:16 AM
 */


#include "I2CBus.h"
#include <stdio.h>
#include <fcntl.h>   // open() function for devices
#include <unistd.h>  // close() function for devices
#include <sys/ioctl.h>

// I2C definitions

#define I2C_SLAVE                   0x0703
#define I2C_SLAVE_FORCE             0x0706
#define I2C_SMBUS                   0x0720 // SMBus-level access
#define I2C_RDWR                    0x0707 // Combined R/W transfer (one STOP only)

#define I2C_SMBUS_WRITE             0
#define I2C_SMBUS_READ              1

#define I2C_SMBUS_BYTE              1
#define I2C_SMBUS_BYTE_DATA         2 
#define I2C_SMBUS_BLOCK_DATA        5
#define I2C_SMBUS_BLOCK_MAX         32 // As specified in SMBus standard

#define I2C_M_TEN                   0x0010
#define I2C_M_RD                    0x0001
#define I2C_M_NOSTART               0x4000
#define I2C_M_REV_DIR_ADDR          0x2000
#define I2C_M_IGNORE_NAK            0x1000
#define I2C_M_NO_RD_ACK             0x0800
#define I2C_M_RECV_LEN              0x0400

union i2c_smbus_data
{
    uint8_t  byte;
    uint16_t word;
    uint8_t  block[I2C_SMBUS_BLOCK_MAX + 2];  // block [0] is used for length + one more for PEC
};
struct i2c_smbus_ioctl_data
{
    char read_write;
    uint8_t command;
    int size;
    union i2c_smbus_data *data;
};

struct i2c_msg {
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t* buf;
};
struct i2c_rdrw_ioctl_data {
    struct i2c_msg *msgs;
    uint32_t nmsgs;
};

I2CBus::I2CBus():
  file(-1)
, debugOutput(true)
{
}

I2CBus::~I2CBus()
{
    if( file>=0 )
        close(file);
}

int I2CBus::Init( const char* busDeviceName )
{
    // device name depends on Raspberry Pi model
    // to deterime the correct device, run the following at the command line:
    //   sudo i2cdetect -y 0
    //   sudo i2cdetect -y 1
    // if the first one works, use "/dev/i2c-0"
    // if the second one works, use "/dev/i2c-1"

    // Open connection
    file = open( busDeviceName, O_RDWR );
    if( file<0 )
        printf( "FAILED TO OPEN DEVICE %s\n", busDeviceName );
}

// Not working.  Seems to always return true
int I2CBus::IsPresent( uint8_t deviceID )
{
    int check = ioctl( file, I2C_SLAVE, deviceID   ); // transmit I2C device address

    if( check == 0 ) return 1;
    return 0;
}

// Use for devices that have only a single register,
// and don't expect a register address to be passed,
// for example the TCA9548A mux chip
int I2CBus::ReadGlobal( uint8_t deviceID, uint8_t* globalVal )
{
    int result = 0;

    union i2c_smbus_data data;

    struct i2c_smbus_ioctl_data cmd;
    cmd.read_write = I2C_SMBUS_READ;
    cmd.command    = 0;
    cmd.size       = I2C_SMBUS_BYTE;
    cmd.data       = &data;

    if( ioctl( file, I2C_SLAVE_FORCE, deviceID   ) != 0)  // transmit I2C device address
        result |= 0x01; // error code
    if( ioctl( file, I2C_SMBUS, &cmd ) != 0 ) // transmit I2C register ID and value
        result |= 0x02; // error code

    (*globalVal) = data.byte;

    if( (result!=0) && debugOutput )
        printf( "ERROR: %i <- I2CBus::WriteGlobal( 0x%X, 0x%X )\n",
            result, (int)deviceID, (int)(*globalVal) );
    
    return result;
}

int I2CBus::WriteGlobal( uint8_t deviceID, uint8_t globalVal )
{
    int result = 0;

    union i2c_smbus_data data;
    data.byte = globalVal;

    struct i2c_smbus_ioctl_data cmd;
    cmd.read_write = I2C_SMBUS_WRITE;
    cmd.command    = globalVal;
    cmd.size       = I2C_SMBUS_BYTE;
    cmd.data       = &data;

    if( ioctl( file, I2C_SLAVE_FORCE, deviceID   ) != 0)  // transmit I2C device address
        result |= 0x01; // error code
    if( ioctl( file, I2C_SMBUS, &cmd ) != 0 ) // transmit I2C register ID and value
        result |= 0x02; // error code

    if( (result!=0) && debugOutput )
        printf( "ERROR: %i <- I2CBus::WriteGlobal( 0x%X, 0x%X )\n",
            result, (int)deviceID, (int)globalVal );
    
    return result;
}

int I2CBus::ReadSingle( uint8_t deviceID, uint8_t regID, uint8_t* regVal )
{
    int result = 0;

    union i2c_smbus_data data;

    struct i2c_smbus_ioctl_data cmd;
    cmd.read_write = I2C_SMBUS_READ;
    cmd.command    = regID;
    cmd.size       = I2C_SMBUS_BYTE_DATA;
    cmd.data       = &data;

    if( ioctl( file, I2C_SLAVE_FORCE, deviceID   ) != 0)  // transmit I2C device address
        result |= 0x01; // error code
    if( ioctl( file, I2C_SMBUS, &cmd ) != 0 ) // transmit I2C register ID and value
        result |= 0x02; // error code

   (*regVal) = data.byte;
    

//    if( result!=0 )
//        printf( "ERROR: %i <- I2CBus::ReadSingle( 0x%X, 0x%X, 0x%X )\n",
//            result, (int)deviceID, (int)regID, (int)(*regVal) );

    return result;
}

int I2CBus::WriteSingle( uint8_t deviceID, uint8_t regID, uint8_t regVal )
{
    int result = 0;

    union i2c_smbus_data data;
    data.byte = regVal;

    struct i2c_smbus_ioctl_data cmd;
    cmd.read_write = I2C_SMBUS_WRITE;
    cmd.command    = regID;
    cmd.size       = I2C_SMBUS_BYTE_DATA;
    cmd.data       = &data;

    if( ioctl( file, I2C_SLAVE_FORCE, deviceID   ) != 0)  // transmit I2C device address
        result |= 0x01; // error code
    if( ioctl( file, I2C_SMBUS, &cmd ) != 0 ) // transmit I2C register ID and value
        result |= 0x02; // error code

    if( (result!=0) && debugOutput )
        printf( "ERROR: %i <- I2CBus::WriteSingle( 0x%X, 0x%X, 0x%X )\n",
            result, (int)deviceID, (int)regID, (int)regVal );
    
    return result;
}


int I2CBus::ToggleSingle( uint8_t deviceID, uint8_t regID, uint8_t regVal, uint8_t bitmask )
{
    int result = 0;

    union i2c_smbus_data data;

    struct i2c_smbus_ioctl_data cmd;
    cmd.read_write = I2C_SMBUS_READ;
    cmd.command    = regID;
    cmd.size       = I2C_SMBUS_BYTE_DATA;
    cmd.data       = &data;

    if( ioctl( file, I2C_SLAVE_FORCE, deviceID   ) != 0)  // transmit I2C device address
        result |= 0x01; // error code
    if( ioctl( file, I2C_SMBUS, &cmd ) != 0 ) // transmit I2C register ID and value
        result |= 0x02; // error code

    data.byte = (data.byte & ~bitmask) | regVal;
    cmd.read_write = I2C_SMBUS_WRITE;

    if( ioctl( file, I2C_SLAVE, deviceID   ) != 0)  // transmit I2C device address
        result |= 0x04; // error code
    if( ioctl( file, I2C_SMBUS, &cmd ) != 0 ) // transmit I2C register ID and value
        result |= 0x08; // error code

    if( (result!=0) && debugOutput )
        printf( "ERROR: %i <- I2CBus::ToggleSingle( 0x%X, 0x%X, 0x%X, 0x%X )\n",
            result, (int)deviceID, (int)regID, (int)regVal, (int)bitmask );
    
    return result;
}

int I2CBus::ReadMulti( uint8_t deviceID, uint8_t regID, uint8_t regCount, uint8_t* regBuf)
{
    int result = 0;

    struct i2c_rdrw_ioctl_data slave_cmd;
    struct i2c_msg slave_msgs[2];
    uint8_t slave_write_data[1] = {regID};

    slave_cmd.nmsgs = 2;
    slave_cmd.msgs = slave_msgs;
    slave_cmd.msgs[0].addr = deviceID;
    slave_cmd.msgs[0].flags = 0;
    slave_cmd.msgs[0].len = 1;
    slave_cmd.msgs[0].buf = slave_write_data;
    slave_cmd.msgs[1].addr = deviceID;
    slave_cmd.msgs[1].flags = I2C_M_RD;
    slave_cmd.msgs[1].len = regCount;
    slave_cmd.msgs[1].buf = regBuf;

    if( ioctl( file, I2C_RDWR, &slave_cmd ) < 0 )
        result |= 0x01; // error code

    if( (result!=0) && debugOutput )
        printf( "ERROR: %i <- I2CBus::ReadMulti( 0x%X, 0x%X, %i, regBuf[0] )\n",
            result, (int)deviceID, (int)regID, (int)regCount, (int)regBuf[0] );

    return result;
}

void I2CBus::SetDebugOutput( bool b )
{
    debugOutput = b;
}
