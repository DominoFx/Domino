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

void OscController::Send(std::string& tag, int index)
{
    Send( tag, index, 0, nullptr );
}

void OscController::Send(std::string& tag, int index, int paramCount, float* paramBuf )
{
    if(m_transmitSocket != NULL)
    {
        memset(&m_outputBuffer[0], 0, sizeof(m_outputBuffer));//Reuse buffer

        osc::OutboundPacketStream packetStream( m_outputBuffer, OSC_OUTPUT_BUFFER_SIZE );

        // Send standard OSC packet, not a bundle; easier for receiver to parse
        packetStream
            << osc::BeginMessage( tag.c_str() ) 
                << index;
        for( int i=0; i<paramCount; i++ )
        {
            packetStream << paramBuf[i];
        }
        packetStream
            << osc::EndMessage
        ;

         m_transmitSocket->Send( packetStream.Data(), packetStream.Size() );
    }
}

OscController::~OscController() {
}

