#include "OscController.h"
#include "common.h"
#include <unistd.h> // for usleep()


OscController::OscController() : m_transmitSocket(nullptr)
{
}

// Defaults: address = "127.0.0.1", portTransmit=8888, portReceive=7777
bool OscController::Init(std::string address, int portTransmit)
{
    m_address = address;
    m_portTransmit = portTransmit;
    
    m_transmitSocket = nullptr;
    int retries = 0, retriesMax = 30;

    // Open transmit socket, with retries in case of failure,
    // can happen in early moments after system startup
    while( (m_transmitSocket==nullptr) && (retries<retriesMax) )
    {
        try
        {
            m_transmitSocket = new UdpTransmitSocket ( IpEndpointName( m_address.c_str(), m_portTransmit ) );
        }
        catch(...)
        {
            retries++;
            fprintf( stderr, "DominoFX: Error transmitting to %s : %i, retry %i ...\n", address.c_str(), portTransmit, retries );
            unsigned int microseconds = 1000000; // 1 second
            usleep(microseconds);
        }
    }

    return (m_transmitSocket != nullptr);
}

bool OscController::IsInitialized()
{
	return (m_transmitSocket!=nullptr);
}

void OscController::Send(std::string& tag, int paramCount, float* paramBuf )
{
    if(m_transmitSocket != nullptr)
    {
        memset(&m_outputBuffer[0], 0, sizeof(m_outputBuffer));//Reuse buffer

        osc::OutboundPacketStream packetStream( m_outputBuffer, OSC_OUTPUT_BUFFER_SIZE );

        packetStream //<< osc::BeginBundleImmediate
            << osc::BeginMessage( tag.c_str() );
        for( int i=0; i<paramCount; i++ )
        {
            packetStream << paramBuf[i];
        }
        packetStream
            << osc::EndMessage
          //<< osc::EndBundle
          ;

         m_transmitSocket->Send( packetStream.Data(), packetStream.Size() );
    }
}

void OscController::Send(std::string& tag, int paramCount, int* paramBuf )
{
    if(m_transmitSocket != NULL)
    {
        memset(&m_outputBuffer[0], 0, sizeof(m_outputBuffer));//Reuse buffer

        osc::OutboundPacketStream packetStream( m_outputBuffer, OSC_OUTPUT_BUFFER_SIZE );

        packetStream //<< osc::BeginBundleImmediate
            << osc::BeginMessage( tag.c_str() );
        for( int i=0; i<paramCount; i++ )
        {
            packetStream << paramBuf[i];
        }
        packetStream
            << osc::EndMessage
          //<< osc::EndBundle
          ;

        m_transmitSocket->Send( packetStream.Data(), packetStream.Size() );
    }
}

void OscController::Send(std::string& tag, int param0, int paramCountRest, const char** paramBufRest )
{
    if(m_transmitSocket != NULL)
    {
        memset(&m_outputBuffer[0], 0, sizeof(m_outputBuffer));//Reuse buffer

        osc::OutboundPacketStream packetStream( m_outputBuffer, OSC_OUTPUT_BUFFER_SIZE );

        packetStream //<< osc::BeginBundleImmediate
            << osc::BeginMessage( tag.c_str() )
                << param0;
        for( int i=0; i<paramCountRest; i++ )
        {
            packetStream << paramBufRest[i];
        }
        packetStream
            << osc::EndMessage
          //<< osc::EndBundle
          ;

         m_transmitSocket->Send( packetStream.Data(), packetStream.Size() );
    }
}

void OscController::Send(std::string& tag, int param0, int param1, int paramCountRest, const char** paramBufRest )
{
    if(m_transmitSocket != NULL)
    {
        memset(&m_outputBuffer[0], 0, sizeof(m_outputBuffer));//Reuse buffer

        osc::OutboundPacketStream packetStream( m_outputBuffer, OSC_OUTPUT_BUFFER_SIZE );

        packetStream //<< osc::BeginBundleImmediate
            << osc::BeginMessage( tag.c_str() )
                << param0
                << param1
                << paramCountRest;
        for( int i=0; i<paramCountRest; i++ )
        {
            packetStream << paramBufRest[i];
        }
        packetStream
            << osc::EndMessage
          //<< osc::EndBundle
          ;

         m_transmitSocket->Send( packetStream.Data(), packetStream.Size() );
    }
}

void OscController::Send(std::string& tag, int param0, int paramCountRest, float* paramBufRest )
{
    if(m_transmitSocket != NULL)
    {
        memset(&m_outputBuffer[0], 0, sizeof(m_outputBuffer));//Reuse buffer

        osc::OutboundPacketStream packetStream( m_outputBuffer, OSC_OUTPUT_BUFFER_SIZE );

        packetStream << osc::BeginBundleImmediate
            << osc::BeginMessage( tag.c_str() ) 
                << param0;
        for( int i=0; i<paramCountRest; i++ )
        {
            packetStream << paramBufRest[i];
        }
        packetStream
            << osc::EndMessage
          << osc::EndBundle;

         m_transmitSocket->Send( packetStream.Data(), packetStream.Size() );
    }
}

OscController::~OscController() {
    SAFE_DELETE( m_transmitSocket );
}


//
// Class OscListener
// 

OscListener::OscListener( const char* debugName ):
  m_debugName(debugName)
, m_port(0)
, m_receiveSocket(nullptr)
, m_receiveMultiplexer(nullptr)
{
}

OscListener::~OscListener()
{
    SAFE_DELETE( m_receiveMultiplexer );
    SAFE_DELETE( m_receiveSocket );
    m_clientMap.clear();
    for( int i=0; i<m_tagVec.size(); i++ )
        SAFE_DELETE_ARRAY( m_tagVec[i] );
    m_tagVec.clear();
}

int OscListener::Register( const char* tag, OscCallback callbackFunc, void* callbackParam )
{
    OscFunc client;
    client.tagStr = tag;
    client.tagLength = client.tagStr.length();
    client.func = callbackFunc;
    client.param = callbackParam;
    
    // Add the tag to the tag list as a char*...
    int tagIndex = m_tagVec.size();
    char* tagStable = new char[ 1+strlen(tag) ];
    strcpy( tagStable, tag );
    // this pointer will remain valid for the lifetime of the listener,
    // unlike the parameter passed to this method, and can be used as a key
    m_tagVec.push_back( tagStable );
    
    m_clientMap[ tagStable ] = client;
    printf( "DominoFX: Registered listener for tag %s on port %i ...\n", tagStable, m_port );
}

int OscListener::Run( int port )
{
    int result = 0;

    SAFE_DELETE( m_receiveMultiplexer );
    SAFE_DELETE( m_receiveSocket );
    
    printf( "DominoFX: Launching %s listener on port %i ...\n", m_debugName.data(), port );
    
    // Listen for connections from any address on the given port
    m_port = port;
    IpEndpointName endpointName;
    endpointName.address = IpEndpointName::ANY_ADDRESS;
    endpointName.port = port;
    
    int retries = 0, retriesMax = 30;
    while( (m_receiveSocket==nullptr) && (retries<retriesMax) )
    {
        try
        {
            m_receiveSocket = new UdpReceiveSocket ( endpointName );
        }
        catch(...)
        {
            retries++;
            fprintf( stderr, "DominoFX: Error receiving from 0x%X : %i, retry %i ...\n", endpointName.address, endpointName.port, retries );
            unsigned int microseconds = 1000000; // 1 second
            usleep(microseconds);
        }
    }
       
    SAFE_DELETE( m_receiveMultiplexer );
    m_receiveMultiplexer = new SocketReceiveMultiplexer();
    m_receiveMultiplexer->AttachSocketListener( m_receiveSocket, this );

    // Create thread to receive UDP packets
    m_receiveThread = std::thread(ReceiveThreadBootstrap,this);

    printf( "DominoFX: Launched %s listener on port %i ...\n", m_debugName.data(), port );
    
    return result;
}

bool OscListener::Running()
{
    return (m_receiveSocket!=nullptr);
}


void OscListener::ProcessPacket( const char *data, int size, const IpEndpointName& remoteEndpoint )
{
    // fields within the packet data start at 32-bit (four byte) boundaries;
    // calculate index values using a mask, round down to a multiple of four
    int mask = 0xFFFFFFFC; // zeroes two lowest bits, yields a multiple of four
    
    ClientMap::iterator it = m_clientMap.find(data);
    if( it != m_clientMap.end() )
    {
        //printf( "OscListener::ProcessPacket() processing %s : %i \n", data, size );
        //for( int i=0; i<(size&mask); i+=4 )
        //{
        //    printf( "  %.3i [%1c] ",   (int)(data[i+0]), (char)(data[i+0]) );
        //    printf( "  %.3i [%1c] ",   (int)(data[i+1]), (char)(data[i+1]) );
        //    printf( "  %.3i [%1c] ",   (int)(data[i+2]), (char)(data[i+2]) );
        //    printf( "  %.3i [%1c] \n", (int)(data[i+3]), (char)(data[i+3]) );
        //}
        OscFunc& client = it->second;
        int typeTagStart = client.tagLength+1; // index of first byte after the tag and trailing zero
        typeTagStart = (typeTagStart+3)&mask; // index rounded up to nearest 32-bits
        if( data[typeTagStart]==',' )
        {
            int index = typeTagStart+1;
            for( ; (index<size) && (data[index]!='\0'); index++ )
                ; // scan until found a terminating character or exceeded the packet size
        
            int entryStart = index+1;
            entryStart = (entryStart+3)&mask; // index rounded up to nearest 32-bits
            if( entryStart<size )
            {
                client.func( (data+entryStart), (size-entryStart), remoteEndpoint, client.param );
            }
            else printf( "OscListener::ProcessPacket() bad packet found, type tag size overrun \n" );
        }
        else
        {
            printf( "OscListener::ProcessPacket() bad packet found, type tag missing, data \"%s\" size %i \n", data, size );
        }
    }
    else
    {
        printf( "OscListener::ProcessPacket() no callback for tag %s port %i \n", data, m_port );
    }
}

void OscListener::ReceiveThread()
{
    printf( "DominoFX: %s listener thread started ...\n", m_debugName.data() );
    m_receiveMultiplexer->Run();
    printf( "DominoFX: %s listener thread stopped ...\n", m_debugName.data() );    
}

