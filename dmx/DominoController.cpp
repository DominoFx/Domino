#include "DominoController.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <wiringPiI2C.h>
#include <algorithm>

#define CONFIG_FILENAME "config.json"
#define ACCEL_DEFAULT_ADDRESS 0xA

DominoController::DominoController():
m_continuousSend(true)
, m_continuousDelay(1.0f / 15.0f)//Default to 15fps
, m_debugOutput(false)
, m_fd(-1)
, m_DMXInterface(NULL)
, m_initialized(false)
, m_sensorCount(0)
, m_useDmx(true)
, m_axis(Axis::Y)
{
}

bool DominoController::Init()
{
    Json::Value jsonRoot;
    std::ifstream configfile(CONFIG_FILENAME);
    configfile >> jsonRoot;
    
    EnttecInterfaces dmxInterface = DMX_USB_PRO;
    std::string dmxDevice = "/dev/ttyUSB0";
    
    if(jsonRoot.empty())
    {
        return false;
    }
   
    if(jsonRoot.isMember("dmxDevice"))
    {
        dmxDevice = jsonRoot["dmxDevice"].asString();
    }

    if(jsonRoot.isMember("debugOutput"))
    {
        m_debugOutput = jsonRoot["debugOutput"].asBool();
    }

    if(jsonRoot.isMember("dmxInterface"))
    {
        std::string dmxInterfaceString = jsonRoot["dmxInterface"].asString();

        if(dmxInterfaceString == "open")
        {
            dmxInterface = OPEN_DMX_USB;
        }
    }

    if (jsonRoot.isMember("sensorCount"))
    {
        m_sensorCount = jsonRoot["sensorCount"].asInt();
    }

    if(jsonRoot.isMember("continuousSend"))
    {
        m_continuousSend = jsonRoot["continuousSend"].asBool();

        if(jsonRoot.isMember("continuousFPS"))
        {
            m_continuousDelay = 1.0f / jsonRoot["continuousFPS"].asFloat();
        }

    }
    
    if(jsonRoot.isMember("axis"))
    {
        m_axis = (Axis)jsonRoot["axis"].asInt();
    }

    if (jsonRoot.isMember("useDmx"))
    {
        m_useDmx = jsonRoot["useDmx"].asBool();
    }
    
    int multiplexerAddress = 0x70;//112
    
    if(jsonRoot.isMember("multiplexerAddress"))
    {
        multiplexerAddress = jsonRoot["multiplexerAddress"].asInt();
    }
    
    m_fd = wiringPiI2CSetup(multiplexerAddress);

    if (m_useDmx)
    {
        m_DMXInterface = new EnttecDMXUSB(dmxInterface, dmxDevice);
        
        if((m_DMXInterface && m_DMXInterface->IsAvailable()))
        {
            m_DMXInterface->ResetCanauxDMX();
            m_DMXInterface->SendDMX();
            m_initialized = true;
        }
    }
    else
    {
        m_initialized = true;
    }
    
    m_lastUpdate = std::chrono::system_clock::now();
    m_lastContinuousSend = std::chrono::system_clock::now();

    return m_initialized;
}

DominoController::~DominoController()
{
}

void DominoController::Update()
{        
            
    std::chrono::duration<double> elapsedSeconds = std::chrono::system_clock::now() - m_lastUpdate;
    std::chrono::duration<double> elapsedSend = std::chrono::system_clock::now() - m_lastContinuousSend;
    
    m_lastUpdate = std::chrono::system_clock::now();
        
    if(m_continuousSend && (elapsedSend.count() >= m_continuousDelay))
    {
        
        for (int i = 0; i < m_sensorCount; ++i)
        {
            capture(i, 10, 500000);
        }

        if (m_useDmx && m_DMXInterface)
        {
            m_DMXInterface->SendDMX();
        }
       
        m_lastContinuousSend = std::chrono::system_clock::now();
    }
    else if(!m_continuousSend)
    {
        m_lastContinuousSend = std::chrono::system_clock::now();
    }
}

bool DominoController::capture(uint8_t sensorIndex, int capS,unsigned int freq)
{	
    if (sensorIndex >= m_sensorCount)
    {
            return false;
    }
	
    wiringPiI2CWrite(m_fd, 1 << sensorIndex);

    int fd = wiringPiI2CSetup(ACCEL_DEFAULT_ADDRESS); //0xA BMA220 Slave address

    /*if (sensorIndex == 1)
    {
            //Filtering
            wiringPiI2CWriteReg8(fd, 0x20, 0x05);//can be set at"0x05""0x04"......"0x01""0x00", refer to Datasheet on wiki
                                                                                     //Sensitivity
            wiringPiI2CWriteReg8(fd, 0x22, 0x0);//can be set at"0x00""0x01""0x02""0x03", refer to Datasheet on wiki
    }*/

    int intData = 0;
    
    switch(m_axis)
    {
        case Axis::X:
        {
            intData = wiringPiI2CReadReg8(fd, 0x04); 
            	
            break;
        }
        
        case Axis::Y:
        {
            int intData = wiringPiI2CReadReg8(fd, 0x06); 

            break;
        }
        
        case Axis::Z:
        {
            int intData = wiringPiI2CReadReg8(fd, 0x08); 

            break;
        }
        
        default://Default on Y
        {
            intData = wiringPiI2CReadReg8(fd, 0x06); 

            break;
        }
    }
            //Acceleration data//////////////////////////////////////////////
            	

    double realData = realdata(intData);            
		
    m_values.push_back(realData);

    if (m_values.size() > 10)
    {
        m_values.erase(m_values.begin());
    }

    double total = 0.0f;

    for (int i = 0; i < m_values.size(); ++i)
    {
        total += m_values[i];
    }

    double mean = total / (double)m_values.size();

    double normalizedY = mean / 0.8125;
    normalizedY = std::max(std::min(normalizedY, 1.0), -1.0);

    int dmxValue = (int)(((normalizedY + 1.0f) / 2.0f) * 255.0f);
    dmxValue = dmxValue <= 0 ? 0 : dmxValue;
    dmxValue = dmxValue > 255 ? 255 : dmxValue;

    if (m_debugOutput)
    {
         printf("Sensor: %d, AxisValue:  %f, DMXValue: %d\n", sensorIndex, realData, dmxValue);
    }

    if (m_useDmx && m_DMXInterface)
    {
        m_DMXInterface->SetCanalDMX(sensorIndex, dmxValue);
    }

    close(fd);

    return true;	
}

double DominoController::realdata (int data)
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
    return x;  
}


