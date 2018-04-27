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
#include "BMA220.h"
#include "LISD3H.h"

#define CONFIG_FILENAME "config.json"

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
, m_normalizationValue(1.0f)
, m_sensor(NULL)
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
    
    if(jsonRoot.isMember("normalizationValue"))
    {
        m_normalizationValue = jsonRoot["normalizationValue"].asFloat();
    }

    if (jsonRoot.isMember("useDmx"))
    {
        m_useDmx = jsonRoot["useDmx"].asBool();
    }
    
    if (jsonRoot.isMember("oscAddress") && jsonRoot.isMember("oscPort") && jsonRoot.isMember("oscTag"))
    {
        std::string address = jsonRoot["oscAddress"].asString();
        int port = jsonRoot["oscPort"].asInt();
        m_oscTag = jsonRoot["oscTag"].asString();
        
        m_oscController.Init(address, port);
    }
    
    int multiplexerAddress = 0x70;//112
    
    if(jsonRoot.isMember("multiplexerAddress"))
    {
        multiplexerAddress = jsonRoot["multiplexerAddress"].asInt();
    }
    
    std::string sensorType = "BMA220";
    
    if(jsonRoot.isMember("sensor"))
    {
        sensorType = jsonRoot["sensor"].asString();
    }
    
    if(sensorType == "LIS3DH")
    {
        m_sensor = new LISD3H();
    }
    else
    {
        m_sensor = new BMA220();
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
            capture(i);
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

bool DominoController::capture(uint8_t sensorIndex)
{	
    if (m_sensor == NULL || sensorIndex >= m_sensorCount)
    {
            return false;
    }
	
    wiringPiI2CWrite(m_fd, 1 << sensorIndex);

    
    double realData = m_sensor->GetValue(m_axis);
             	
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

    double normalized = mean / m_normalizationValue;
    normalized = std::max(std::min(normalized, 1.0), -1.0);

    int dmxValue = (int)(((normalized + 1.0f) / 2.0f) * 255.0f);
    dmxValue = std::max(std::min(dmxValue, 255), 0);

    if (m_debugOutput)
    {
         printf("Sensor: %d, AxisValue:  %f, DMXValue: %d\n", sensorIndex, realData, dmxValue);
    }

    if (m_useDmx && m_DMXInterface)
    {
        m_DMXInterface->SetCanalDMX(sensorIndex, dmxValue);
    }
    
    m_oscController.Send(m_oscTag, sensorIndex + 1);

    return true;	
}


