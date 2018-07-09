#ifndef OSCCONTROLLER_H
#define OSCCONTROLLER_H

#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#include <string>

#define OSC_OUTPUT_BUFFER_SIZE 1024

class OscController {
public:
    OscController();
    virtual ~OscController();
    bool Init(std::string address = "127.0.0.1", int port = 8888);

    // Send data with sensor index and with zero or multiple additional parameters
    void Send(std::string& tag, int index );
    void Send(std::string& tag, int index, int paramCount, float* paramBuf );

private:
    std::string m_address;
    int m_port;
    
    UdpTransmitSocket* m_transmitSocket;
    char m_outputBuffer[OSC_OUTPUT_BUFFER_SIZE];
};

#endif /* OSCCONTROLLER_H */

