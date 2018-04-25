#include "OscController.h"

OscController::OscController()
{
}

bool OscController::Init(std::string address/* = "127.0.0.1"*/, int port/* = 8888*/)
{
    m_address = address;
    m_port = port;
    
    m_transmitSocket = new UdpTransmitSocket ( IpEndpointName( m_address.c_str(), m_port ) );
    
    return m_transmitSocket != NULL;
}

void OscController::Send(std::string& tag, int index /*float normalizedValue*/)
{
    if(m_transmitSocket != NULL)
    {
        memset(&m_outputBuffer[0], 0, sizeof(m_outputBuffer));//Reuse buffer

        osc::OutboundPacketStream packetStream( m_outputBuffer, OSC_OUTPUT_BUFFER_SIZE );

        packetStream << osc::BeginBundleImmediate
            << osc::BeginMessage( tag.c_str() ) 
                << index 
            << osc::EndMessage
          << osc::EndBundle;

         m_transmitSocket->Send( packetStream.Data(), packetStream.Size() );
    }
}

OscController::~OscController() {
}

