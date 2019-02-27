#include "OscController.h"
#include "common.h"
#include <unistd.h> // for usleep()


OscController::OscController() : m_transmitSocket(nullptr)
{
}

bool OscController::Open(std::string address, int port)
{
    //printf("OscController() %s:%i \n", address.c_str(), port);
    IpEndpointName ipEndpointName = IpEndpointName( address.c_str(), port );
    
    if( Opened() &&
        (m_ipEndpointName.address==ipEndpointName.address) &&
        (m_ipEndpointName.port=ipEndpointName.port) )
        return true; // nothing to do

    m_ipEndpointName = ipEndpointName;
            
    SAFE_DELETE(m_transmitSocket);
    UdpSocket* temp = new UdpSocket();

    int retries = 0, retriesMax = 8;
    while( (m_transmitSocket==nullptr) && (retries<retriesMax) )
    {
        try
        {
            temp->Connect( m_ipEndpointName );
            m_transmitSocket = temp;
        }
        catch(...)
        {
            SAFE_DELETE(m_transmitSocket);
            retries++;
            fprintf( stderr, "DominoFX: Error transmitting to %s : %i, retry %i ...\n", address.c_str(), (int)port, (int)retries );
            unsigned int microseconds = 1000000; // 1 second
            usleep(microseconds);
        }
    }
    if( retries==retriesMax )
    {
        SAFE_DELETE(temp);
    }
    if( Opened() )
    {
        printf( "DominoFX: Opened transmitter socket to %s : %i...\n", address.c_str(), (int)port );
    }
    bool retval = Opened();

    return (retval);
}

bool OscController::Opened()
{
	return (m_transmitSocket!=nullptr);  // no ability to check IsConnected()
}

void OscController::Send(std::string& tag, int paramCount, float* paramBuf )
{
    //printf("OscController::Send() 1 -> %s \n", tag.c_str());
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
    //printf("OscController::Send() 1 <- %s \n", tag.c_str());
}

void OscController::Send(std::string& tag, int paramCount, int* paramBuf )
{
    //printf("OscController::Send() 2 -> %s \n", tag.c_str());
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
    //printf("OscController::Send() 2 <- %s \n", tag.c_str());
}

void OscController::Send(std::string& tag, int param0, int paramCountRest, const char** paramBufRest )
{
    //printf("OscController::Send() 3 -> %s \n", tag.c_str());
    if(m_transmitSocket != nullptr)
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
    //rintf("OscController::Send() 3 <- %s \n", tag.c_str());
}

void OscController::Send(std::string& tag, int param0, int param1, int paramCountRest, const char** paramBufRest )
{
    //printf("OscController::Send() 4 -< %s \n", tag.c_str());
    if(m_transmitSocket != nullptr)
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
    //printf("OscController::Send() 4 <- %s \n", tag.c_str());
}

void OscController::Send(std::string& tag, int param0, int paramCountRest, float* paramBufRest )
{
    //printf("OscController::Send() 5 -> %s \n", tag.c_str());
    if(m_transmitSocket != nullptr)
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
    //printf("OscController::Send() 5 <- %s \n", tag.c_str());
}

OscController::~OscController() {
    SAFE_DELETE( m_transmitSocket );
}



//
// Class OscBroadcaster
// 

OscBroadcaster::OscBroadcaster() : m_broadcastSocket(nullptr)
{
}

OscBroadcaster::~OscBroadcaster() {
    SAFE_DELETE( m_broadcastSocket );
}

bool OscBroadcaster::Open( int port )
{
    printf("OscBroadcaster() %i \n", port);
    
    m_broadcastSocket = nullptr;
    int retries = 0, retriesMax = 30;
    while( (m_broadcastSocket==nullptr) && (retries<retriesMax) )
    {
        try
        {
            m_broadcastSocket = new UdpBroadcastSocket( port );
        }
        catch(...)
        {
            retries++;
            fprintf( stderr, "DominoFX: Error broadcasting on port %i, retry %i ...\n",
                port, retries );
            usleep(1000000); // 1 second, or 1 million microseconds
        }
    }

    if( Opened() )
    {
        printf( "DominoFX: Opened broadcaster socket to %i...\n", (int)port );
    }
    bool retval = Opened();
    
    return (retval);
}

bool OscBroadcaster::Opened()
{
	return (m_broadcastSocket!=nullptr);  // no ability to check IsConnected()
}

void OscBroadcaster::Send( std::string& tag, int paramCount, int* paramBuf )
{
    //printf("OscBroadcaster::Send() 1 -> %s \n", tag.c_str());
    
    if(m_broadcastSocket != NULL)
    {
        memset(&m_outputBuffer[0], 0, sizeof(m_outputBuffer));//Reuse buffer

        osc::OutboundPacketStream packetStream( m_outputBuffer, OSC_OUTPUT_BUFFER_SIZE );

        packetStream
            << osc::BeginMessage( tag.c_str() );
        for( int i=0; i<paramCount; i++ )
        {
            packetStream << paramBuf[i];
        }
        packetStream
            << osc::EndMessage
          ;

        m_broadcastSocket->Send( packetStream.Data(), packetStream.Size() );
    }
    //printf("OscBroadcaster::Send() 1 <- %s \n", tag.c_str());
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

int OscListener::Unregister( OscCallback callbackFunc )
{
    ClientMap::iterator iter;
    for( iter=m_clientMap.begin(); iter!=m_clientMap.end(); )
    {
        if( iter->second.func==callbackFunc )
        {
            ClientMap::iterator item = iter;
            iter++; // increment must be performed before erasing item
            m_clientMap.erase(item);
        }
        else iter++;
    }
}

int OscListener::Open( int port )
{
    if( Opened() && (port==m_port) )
        return true; // nothing to do
        
    //printf( "1: OscListener::Open() %i\n", port );
        
    int result = 0;

    SAFE_DELETE( m_receiveMultiplexer );
    SAFE_DELETE( m_receiveSocket );
    UdpSocket* temp = new UdpSocket();
    
    printf( "DominoFX: Launching %s listener on port %i ...\n", m_debugName.data(), (int)port );
    
    // Listen for connections from any address on the given port
    m_port = port;
    IpEndpointName endpointName;
    endpointName.address = IpEndpointName::ANY_ADDRESS;
    endpointName.port = port;

    int retries = 0, retriesMax = 8;
    while( (m_receiveSocket==nullptr) && (retries<retriesMax) )
    {
        //printf( "2: OscListener::Open() %i\n", port );
        try
        {
            temp->Bind( endpointName );
            m_receiveSocket = temp;
            //printf( "3c: OscListener::Open() %i\n", port );
        }
        catch(std::exception const& e)
        {
            fprintf( stderr, "DominoFX: Error [%s|%s] receiving from 0x%X : %i, retry %i...\n",
                (const char*)(e.what()), (const char*)(typeid(e).name()),
                endpointName.address, (int)(endpointName.port), (int)retries );
        }
        catch(...)
        {
            fprintf( stderr, "DominoFX: Unknown error receiving from 0x%X : %i, retry %i...\n",
                endpointName.address, (int)(endpointName.port), (int)retries );
        }
        if( m_receiveSocket==nullptr )
        {
            retries++;
            unsigned int microseconds = 1000000; // 1 second
            usleep(microseconds);
        }
    }
    //printf( "4: OscListener::Open() %i\n", port );
    if( retries==retriesMax )
    {
        SAFE_DELETE(temp);
        printf( "DominoFX: Failed launching %s listener on port %i...\n", m_debugName.data(), (int)port );
    }
    else
    {
        //printf( "5: OscListener::Open() %i\n", port );
        m_receiveMultiplexer = new SocketReceiveMultiplexer();
        m_receiveMultiplexer->AttachSocketListener( m_receiveSocket, this );

        // Create thread to receive UDP packets
        if( m_receiveThread.get_id()==std::thread::id() )
        {
            m_receiveThread = std::thread(ReceiveThreadBootstrap,this);
        }

        printf( "DominoFX: Opened receiver socket on port %i...\n", (int)port );
    }
    //printf( "6: OscListener::Open() %i\n", port );
    
    return result;
}

bool OscListener::Opened()
{
    return (m_receiveSocket!=nullptr) && (m_receiveSocket->IsBound());
}

int OscListener::Port()
{
    return (m_port);
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

