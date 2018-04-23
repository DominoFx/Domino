#include "DmxController.h"
#include <iostream>
#include <fstream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include <sys/time.h>
#include <wiringPiI2C.h>
#include <algorithm>
//#include "LIS3DH.h"
#include <soft_i2c.h>

//OSC BEGIN
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

#define ADDRESS "127.0.0.1"
#define PORT 8888

#define OUTPUT_BUFFER_SIZE 1024

UdpTransmitSocket transmitSocket( IpEndpointName( ADDRESS, PORT ) );
//OSC END

#define DMX_CONFIG_FILENAME "dmxconfig.json"
#define ACCEL_DEFAULT_ADDRESS 0xA

///Vellman

#include <usb.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

struct usb_bus *bus;
struct usb_device *dev;
usb_dev_handle *udev;
int *channels;
int *maxchannel;
int *shutdown;
int *caps;
int *shm;
int shmid;

void write_command ( unsigned char *data )
{
  usb_interrupt_write(udev,0x1,(char*)data,8,20);
}

void sendDMX()
{
  int i, n;
  unsigned char data[8];
  int m;

  m=*maxchannel;
  for (i=0;(i<100) && !channels[i] && (i < m - 6);i++); 

  data[0] = 4; /* Start of data, number of leading 0 and 6 channels */
  data[1] = i+1;
  data[2] = channels[i];
  data[3] = channels[i+1];
  data[4] = channels[i+2];
  data[5] = channels[i+3];
  data[6] = channels[i+4];
  data[7] = channels[i+5];
  write_command(data);
  i+=6;

  while (i < m - 7) {
    if (!channels[i]) {
      for(n=i+1;(n < m - 6) && (n-i<100) && !channels[n] ;n++) {
        data[0] = 5;
        data[1] = n-i;
	data[2] = channels[n];
	data[3] = channels[n+1];
	data[4] = channels[n+2];
	data[5] = channels[n+3];
	data[6] = channels[n+4];
	data[7] = channels[n+5];
	write_command(data);
	i=n+6;
      }
    } else {
      data[0] = 2; /* 7 channels */
      data[1] = channels[i];
      data[2] = channels[i+1];
      data[3] = channels[i+2];
      data[4] = channels[i+3];
      data[5] = channels[i+4];
      data[6] = channels[i+5];
      data[7] = channels[i+6];
      write_command(data);
      i+=7;
    }
  }
  
  for(;i < m;i++) {
    data[0] = 3; /* send one channel */
    data[1] = channels[i];
    write_command(data);
  }
}

int initUSB()
{
  int count = 0;

  usb_init();
  usb_find_busses();
  usb_find_devices();

  for (bus = usb_busses; bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      if ( (dev->descriptor.idVendor == 0x10cf) &&
           (dev->descriptor.idProduct == 0x8062 ) ) {
           udev=usb_open(dev);
#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
           usb_detach_kernel_driver_np( udev, 0);
#endif
           usb_set_configuration(udev, 1);
           usb_claim_interface(udev, 0);
           return 1;
      }
    }
  }
  return 0;
}

void initSHM()
{
    
    shmid=shmget(0x56444D58,sizeof(int)*522,IPC_CREAT | 0666);
    shm=(int *)shmat(shmid,NULL,0);
    memset(shm,0,sizeof(int)*522);
    maxchannel=shm;
    shutdown=shm+1;
    caps=shm+2;
    channels=shm+10;
    *shutdown=0; /* Run mode */
    *caps=0; /* Services available flags */
    *maxchannel=6;
}

void release()
{
    usb_close(udev);
    shmdt(shm);
    shmctl(shmid,IPC_RMID,NULL);
}

void timediff(struct timeval *res, struct timeval *a, struct timeval *b) {
    long sec,usec;
    sec=a->tv_sec-b->tv_sec;
    usec=a->tv_usec-b->tv_usec;
    while (usec<0) {
        usec+=1000000;
        sec--;
    }
    if (sec<0) {
	res->tv_sec=0;
	res->tv_usec=0;
    } else {
	res->tv_sec=sec;
	res->tv_usec=usec;
    }
}

void timeadd(struct timeval *res, struct timeval *a, struct timeval *b) {
    res->tv_usec=a->tv_usec+b->tv_usec;
    res->tv_sec=a->tv_sec+b->tv_sec;
    while (res->tv_usec >= 1000000) {
	res->tv_usec-=1000000;
	res->tv_sec;
    }
}

bool InitVellman()
{
    struct timeval now,next,diff,delay;

    if (initUSB())
    {
	initSHM();
        
        return true;
    }
    
    return false;
}

//Vellman end

DmxController::DmxController():
m_continuousSend(true)
,m_continuousDelay(1.0f / 15.0f)//Default to 15fps
,m_debugOutput(false)
,m_presenceTransitionSpeed(128)
,m_presenceThreshold(0.0f)
,m_fd(-1)
,m_DMXInterface(NULL)
,m_initialized(false)
, m_sensorCount(0)
, m_useDmx(true)
{
    m_value = 0;
    m_maxValue = 127;
    m_direction = true;
}

bool DmxController::Init()
{
    Json::Value jsonRoot;
    std::ifstream configfile(DMX_CONFIG_FILENAME);
    configfile >> jsonRoot;
    
    EnttecInterfaces dmxInterface = DMX_USB_PRO;
    std::string dmxDevice = "/dev/ttyUSB0";
    
    if(!jsonRoot.empty())
    {
        if(jsonRoot.isMember("dmxDevice"))
        {
            dmxDevice = jsonRoot["dmxDevice"].asString();
        }
        
        if(jsonRoot.isMember("debugOutput"))
        {
            m_debugOutput = jsonRoot["debugOutput"].asBool();
        }
        
        if(jsonRoot.isMember("maxValue"))
        {
            m_maxValue = jsonRoot["maxValue"].asBool();
        }
        
        if(jsonRoot.isMember("dmxInterface"))
        {
            std::string dmxInterfaceString = jsonRoot["dmxInterface"].asString();
            
            if(dmxInterfaceString == "open")
            {
                dmxInterface = OPEN_DMX_USB;
            }
            else if(dmxInterfaceString == "vellman")
            {
                dmxInterface = VELLMAN;
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
        
        if(jsonRoot.isMember("presenceTransitionSpeed"))
        {
            m_presenceTransitionSpeed = jsonRoot["presenceTransitionSpeed"].asInt();
        }
        
        if(jsonRoot.isMember("presenceThreshold"))
        {
            m_presenceThreshold = jsonRoot["presenceThreshold"].asFloat();
        }
        
        if(jsonRoot.isMember("mapping"))
        {
            Json::Value mappings = jsonRoot["mapping"];
            
            if(mappings.isArray())
            {
                for(int i = 0; i < mappings.size(); ++i)
                {
                    Json::Value mapping = mappings.get(i, mappings);
                    
                    if(mapping.isMember("id") && mapping.isMember("channels"))
                    {
                        Json::Value channels = mapping["channels"];
                        
                        if(channels.isArray())
                        {
                            for(int j = 0; j < channels.size(); ++j)
                            {
                                Json::Value channelDefinition = channels.get(j, channels);
                                
                                if(channelDefinition.isMember("channel") && (channelDefinition.isMember("max") || channelDefinition.isMember("static")))
                                {
                                    ChannelDefinition definition;

                                    definition.channel = channelDefinition["channel"].asInt();

                                    if(channelDefinition.isMember("max"))
                                    {
                                        definition.max = channelDefinition["max"].asInt();
                                    }
                                    
                                    if(channelDefinition.isMember("min"))
                                    {
                                        definition.min = channelDefinition["min"].asInt();
                                    }

                                    m_mappings[mapping["id"].asString()].push_back(definition);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

	if (jsonRoot.isMember("useDmx"))
	{
		m_useDmx = jsonRoot["useDmx"].asBool();
	}

	if (m_useDmx)
	{
		if (dmxInterface != VELLMAN)
		{
			m_DMXInterface = new EnttecDMXUSB(dmxInterface, dmxDevice);
			
			if (m_DMXInterface != NULL && m_DMXInterface->IsAvailable())
			{
				m_DMXInterface->ResetCanauxDMX();
				m_initialized = true;
			}
		}
		else
		{
			if (InitVellman())
			{
				m_initialized = true;
			}
		}
	}
	else
	{
		m_initialized = true;
	}
    
    
    /*if(m_DMXInterface && m_DMXInterface->IsAvailable())
    {
        m_DMXInterface->ResetCanauxDMX();
        m_DMXInterface->SetCanalDMX(1, 255);
        m_DMXInterface->SendDMX();
        m_initialized = true;
    }*/
    
    m_lastUpdate = std::chrono::system_clock::now();
    m_lastContinuousSend = std::chrono::system_clock::now();

    m_fd = wiringPiI2CSetup(0x70);//Multiplexer address

    return m_initialized;
}

DmxController::~DmxController()
{
}

void DmxController::Apply(const Json::Value& msgData)
{
    bool send = false;
    
    if(m_debugOutput)
    {
        fprintf(stderr, "Message received\n");
    }
    
    //std::ofstream file("dmxmessage.json");
    //file << msgData;
    
    int projectChannel = -1;
    int projectValue = -1;
    int projectSpeed = 0;
    double projectNormalizedValue = -1.0f;
    
    for(int i = 0; i < msgData.size(); ++i)
    {
        Json::Value data = msgData.get(i, msgData);

        if(data.isMember("channel"))
        {
            int channel = data.get("channel", data).asInt();

            if(m_debugOutput)
            {
                fprintf(stderr, "Channel: %d\n", channel);
            }
            
            int speed = 0;

            if(data.isMember("speed"))
            {
                speed = data.get("speed", data).asInt();
                
                if(m_debugOutput)
                {
                    fprintf(stderr, "Speed: %d\n", speed);
                }
            }
            
            if(speed == 0)
            {
                send = true;
            }
            
            if(data.isMember("value"))
            {
                int value = data.get("value", data).asInt();

                if(m_debugOutput)
                {
                    fprintf(stderr, "Value: %d\n", value);
                }
                
                Set(channel, value, speed);
            }
            else if(data.isMember("normalizedValue"))
            {
                double normalizedValue = data.get("normalizedValue", data).asDouble();

                Set(channel, 255.0 * normalizedValue, speed);
            }
        }
        else if(data.isMember("key") && data.isMember("value"))
        {
            std::string key = data.get("key", data).asString();
            std::string value = data.get("value", data).asString();

            if(key == "channel")
            {
                projectChannel = std::stoi(value);                    
            }
            else if(key == "value")
            {
                projectValue = std::stoi(value);
            }
            else if(key == "speed")
            {
                projectSpeed = std::stoi(value);

            }
            else if(key == "normalizedValue")
            {
                projectNormalizedValue = std::stod(value);

            }

            //fprintf(stderr, "Key: %s\n", key.c_str());
            //fprintf(stderr, "Value: %s\n", value.c_str());
        }
        else if(data.isMember("id") && data.isMember("presence"))
        {
            IdChannelMap::iterator it = m_mappings.find(data["id"].asString());
            
            if(it != m_mappings.end())
            {
                float presence = data["presence"].asFloat();
                
                for(std::vector<ChannelDefinition>::iterator itChannels = it->second.begin(); itChannels != it->second.end(); ++itChannels)
                {
                    if((presence <= 0.001f) || ((presence >= m_presenceThreshold) && (presence <= (1.0f - m_presenceThreshold))) || (presence >= 0.999f))
                    {
                        int dmxValue = ((float)itChannels->max - (float)itChannels->min) * presence + itChannels->min;
                        Set(itChannels->channel, dmxValue, m_presenceTransitionSpeed);

                        send = true;
                    }
                }
            }
        }
    }
    
    if(projectChannel >= 0 && projectValue >= 0)
    {
        //fprintf(stderr, "Speed: %d\n", projectSpeed);
        //fprintf(stderr, "NormalizedValue: %.2f\n", projectNormalizedValue);

        if(projectSpeed == 0)
        {
            send = true;
        }

        if(projectNormalizedValue >= 0.0f)
        {
            Set(projectChannel, 255.0 * projectNormalizedValue, projectSpeed);
        }
        else
        {
            Set(projectChannel, projectValue, projectSpeed);
        }
    }
    
    if(m_DMXInterface && send && !m_continuousSend)
    {
        m_DMXInterface->SendDMX();
    }
}

void DmxController::Set(int channel, int value, int speed /* = 0*/)
{
    if(!m_DMXInterface)
    {
        return;
    }
    
    if(m_debugOutput)
    {
        fprintf(stderr, "Setting channel: %d -> %d\n", channel, value);
    }
    
     Interpolations::iterator interIt = m_interpolations.find(channel);
     
    if(speed != 0)
    {        
        if(interIt != m_interpolations.end())
        {
            interIt->second.m_targetValue = value;
            interIt->second.m_speed = speed;
        }
        else
        {
            int currentValue = m_DMXInterface->GetChannelDMX(channel);

            ChannelInterpolation interpolation(value, speed, currentValue);
            
            m_interpolations[channel] = interpolation;
        }
    }
    else
    {
        m_DMXInterface->SetCanalDMX(channel, value);   
                
        if(interIt != m_interpolations.end())
        {
            m_interpolations.erase(interIt);
        }
    }    
}

//Some spots: Intensity: 0-127 = solid, 128-255 = blink
//Majority: 0-255
void DmxController::SetSpot(int spotIndex, int intensity, int red, int green, int blue)
{
    if(!m_DMXInterface || !m_initialized)
    {
        return;
    }
    
    int startChannel = (4 * spotIndex);

    m_DMXInterface->SetCanalDMX(startChannel + 1, intensity);
    m_DMXInterface->SendDMX();

    m_DMXInterface->SetCanalDMX(startChannel + 2, red);
    m_DMXInterface->SendDMX();

    m_DMXInterface->SetCanalDMX(startChannel + 3, green);
    m_DMXInterface->SendDMX();

    m_DMXInterface->SetCanalDMX(startChannel + 4, blue);
    m_DMXInterface->SendDMX();
}

void DmxController::Update()
{        
            
    std::chrono::duration<double> elapsedSeconds = std::chrono::system_clock::now() - m_lastUpdate;
    std::chrono::duration<double> elapsedSend = std::chrono::system_clock::now() - m_lastContinuousSend;
    
    m_lastUpdate = std::chrono::system_clock::now();
        
    /*Interpolations::iterator it = m_interpolations.begin();
    
    std::vector<int> completedChannels;
    
    for(; it != m_interpolations.end(); ++it)
    {
        bool complete = false;
        
        ChannelInterpolation& inter = it->second;
        
        if(inter.m_targetValue > inter.m_currentValue)
        {
            inter.m_currentInterpolation += elapsedSeconds.count() * (double)inter.m_speed;
            
            int newValue = (int)inter.m_currentInterpolation;
            
            if(newValue >= inter.m_targetValue)
            {
                newValue = inter.m_targetValue;
                complete = true;
            }
            
            if(newValue != inter.m_currentValue)
            {                
                m_DMXInterface->SetCanalDMX(it->first, newValue);
            }
        }
        else
        {
            inter.m_currentInterpolation -= elapsedSeconds.count() * (double)inter.m_speed;
            
            int newValue = (int)inter.m_currentInterpolation;
            
            if(newValue <= inter.m_targetValue)
            {
                newValue = inter.m_targetValue;
                complete = true;
            }
            
            if(newValue != inter.m_currentValue)
            {                
                m_DMXInterface->SetCanalDMX(it->first, newValue);
            }
 
        }
        
        if(complete)
        {
            completedChannels.push_back(it->first);
        }
    }
    */
    if(m_interpolations.size() > 0 || (m_continuousSend && (elapsedSend.count() >= m_continuousDelay)))
    {
        
                for (int i = 0; i < m_sensorCount; ++i)
		{
			capture(i, 10, 500000);
		}
                /*int x,y,z;
                if(LIS3DH::Read(x, y, z) && m_debugOutput)
                {
                    printf("LIS3H: %d, %d, %d\n", x, y, z);
                }*/
                
                i2c_t my_bus = i2c_init(9, 8);
    
    i2c_start(my_bus);
    
    i2c_send_bit(my_bus, 0x28 << 1 | I2C_READ);

    int xdata = i2c_read_bit(my_bus);    
    
    printf("x: %d\n", xdata);
    
    i2c_stop(my_bus);

		if (m_useDmx)
		{
			if (m_DMXInterface)
			{
				m_DMXInterface->SendDMX();
			}
			else
			{
				sendDMX();
			}
		}
        
	/*if(m_direction)
        {
            ++m_value;
            if(m_value >= m_maxValue)
            {
                m_value = m_maxValue;
                m_direction = false;
            }
        }
        else
        {
            --m_value;
            if(m_value <= 0)
            {
                m_value = 0;
                m_direction = true;
            }
        }
        
        //channels[0] = m_value;
        //channels[1] = 255;
        //channels[2] = 255;
        //channels[3] = 255;
        //sendDMX();
        
        if(m_DMXInterface)
        {
            m_DMXInterface->SetCanalDMX(1, m_value);
        
            m_DMXInterface->SendDMX();
	}*/
        m_lastContinuousSend = std::chrono::system_clock::now();
    }
    else if(!m_continuousSend)
    {
        m_lastContinuousSend = std::chrono::system_clock::now();
    }
    
    /*
    for(std::vector<int>::iterator itComplete = completedChannels.begin(); itComplete != completedChannels.end(); ++itComplete)
    {        
        Interpolations::iterator toErase = m_interpolations.find(*itComplete);
        m_interpolations.erase(toErase);    
    }  
    */
}

void DmxController::getDefaultData()
{
    int fd, result;
    int xdata,ydata,zdata;
    double xreal, yreal, zreal;
	
    printf("Bosch BMA220 Accelerometer + Raspberry Pi0W Test \n");
	
    printf("BMA220 I2C Slave address\t 0x20\n");
    printf("Register\tDefault Data\n");

    fd = wiringPiI2CSetup(ACCEL_DEFAULT_ADDRESS); //0xA BMA220 Slave address

    result = wiringPiI2CReadReg8(fd, 0x0);  printf("0x00 = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x02); printf("0x02 = %x\n", result);

    //Acceleration data///////////////////////////////////////////////////
    xdata  = wiringPiI2CReadReg8(fd, 0x04); 
    xreal = realdata(xdata);		
    printf("0x04 = %d  %f\n", xdata, xreal);

    ydata  = wiringPiI2CReadReg8(fd, 0x06); 
    yreal = realdata(ydata);
    printf("0x06 = %d  %f\n", ydata, yreal);

    zdata  = wiringPiI2CReadReg8(fd, 0x08); 
    zreal = realdata(zdata); 
    printf("0x08 = %d  %f\n", zdata, zreal);
    /////////////////////////////////////////////////////////////////////


    result = wiringPiI2CReadReg8(fd, 0x0A); printf("0x0A = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x0C); printf("0x0C = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x0E); printf("0x0E = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x10); printf("0x10 = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x12); printf("0x12 = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x14); printf("0x14 = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x16); printf("0x16 = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x18); printf("0x18 = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x1A); printf("0x1A = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x1C); printf("0x1C = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x1E); printf("0x1E = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x20); printf("0x20 = %x\n", result);
    result = wiringPiI2CReadReg8(fd, 0x22); printf("0x22 = %x\n", result);
    //result = wiringPiI2CReadReg8(fd, 0x24); printf("0x24 = %x\n", result);
    //result = wiringPiI2CReadReg8(fd, 0x26); printf("0x26 = %x\n", result);
    //result = wiringPiI2CReadReg8(fd, 0x28); printf("0x28 = %x\n", result);
    //result = wiringPiI2CReadReg8(fd, 0x30); printf("0x30 = %x\n", result);
    //result = wiringPiI2CReadReg8(fd, 0x2A); printf("0x2A = %x\n", result);
    //result = wiringPiI2CReadReg8(fd, 0x2C); printf("0x2C = %x\n", result);
    //result = wiringPiI2CReadReg8(fd, 0x2E); printf("0x2E = %x\n", result);
    //result = wiringPiI2CReadReg8(fd, 0x30); printf("0x30 = %x\n", result);
    //result = wiringPiI2CReadReg8(fd, 0x32); printf("0x32 = %x\n", result);	
	
}

int DmxController::capture(uint8_t sensorIndex, int capS,unsigned int freq)
{	
	if (sensorIndex >= m_sensorCount)
	{
		return -1;
	}

    int fd;	
    int xdata,ydata,zdata;
    double xreal, yreal, zreal;
    struct timeval tvalA, tvalB;  	
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);		
    FILE *fp;
	
    wiringPiI2CWrite(m_fd, 1 << sensorIndex);

    fd = wiringPiI2CSetup(ACCEL_DEFAULT_ADDRESS); //0xA BMA220 Slave address

    //if (sensorIndex == 1)
    {
            //Filtering
            wiringPiI2CWriteReg8(fd, 0x20, 0x05);//can be set at"0x05""0x04"......"0x01""0x00", refer to Datashhet on wiki

                                                                                     //Sensitivity
            wiringPiI2CWriteReg8(fd, 0x22, 0x0);//can be set at"0x00""0x01""0x02""0x03", refer to Datashhet on wiki
    }

    gettimeofday (&tvalB, NULL);  //must be intilised
    gettimeofday (&tvalA, NULL); //must be intilised

    //To do remove hard file path

    //fp = fopen ("/home/pi/myc/i2cwiringpitest/test.dat","wb");	
    //if (fp)
    //{	
        //fprintf(fp,"%d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);


        //while(  (tvalB.tv_sec - tvalA.tv_sec) < capS  )
        {			
            //usleep(freq);

            //Acceleration data//////////////////////////////////////////////
            xdata  = wiringPiI2CReadReg8(fd, 0x04); 
            xreal = realdata(xdata);		

            ydata  = wiringPiI2CReadReg8(fd, 0x06); 
            yreal = realdata(ydata);

            zdata  = wiringPiI2CReadReg8(fd, 0x08); 
            zreal = realdata(zdata); 

            ////////////////////////////////////////////////////////////////

            gettimeofday (&tvalB, NULL);
		
	        /*m_values.push_back(yreal);

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
			*/

			 

			double normalizedY = yreal / 0.8125;
			normalizedY = std::max(std::min(normalizedY, 1.0), -1.0);

			int newValue = (int)(((normalizedY + 1.0f) / 2.0f) * 255.0f);
			newValue = newValue <= 0 ? 0 : newValue;
			newValue = newValue > 255 ? 255 : newValue;

			if (m_debugOutput)
			{
				printf("Sensor %d,  X,Y,Z [m/s^2] = %f, %f,  %f : %d\n", sensorIndex, xreal, yreal, zreal, newValue);
			}
            
			if (m_useDmx)
			{
				if (m_DMXInterface)
				{
					m_DMXInterface->SetCanalDMX(sensorIndex, newValue);

					//m_DMXInterface->SendDMX();
				}
				else
				{
					channels[sensorIndex] = newValue;
					//channels[1] = newValue;
					//channels[2] = newValue;
					//channels[3] = newValue;
					//sendDMX();
				}
			}
                        
                        //OSC BEGIN
                        /*if(m_previousValues.size() == 0)
                        {
                                m_previousValues.push_back(newValue);
                        }
                        //if(newValue > 200 || newValue < 55) 
                        else if(abs(m_previousValues[0] - newValue) > 5)
                        {
                          m_previousValues[0] = newValue;
                           char buffer[OUTPUT_BUFFER_SIZE];
                           osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

                           p << osc::BeginBundleImmediate
                              << osc::BeginMessage( "/hits" ) 
                                << 1 << osc::EndMessage
                             << osc::EndBundle;

                            transmitSocket.Send( p.Data(), p.Size() );

                            if(m_debugOutput)
                            {
                                    printf("PLaying sound\n");
                            }
                        }*/
                        //OSC END
                        
                        
            //long int timdur;

            //timdur = ((tvalB.tv_sec - tvalA.tv_sec)*1000000L +tvalB.tv_usec) - tvalA.tv_usec ; 

            //fprintf(fp,"%ld,%f,%f,%f\n",timdur, xreal, yreal, zreal);

        }
	 	  	  
        //fclose(fp);
   //}	

		close(fd);

   return 1;	
}



double DmxController::realdata (int data)
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


