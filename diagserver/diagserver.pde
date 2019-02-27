import java.util.Timer;
import java.util.TimerTask;
import java.util.Vector;
import java.nio.ByteBuffer;
import hypermedia.net.*; // -- UDP LIBRARY --
//  import processing.net.*; // -- OSCP5 LIBRARY --
//  import oscP5.*; // -- OSCP5 LIBRARY --
//  import netP5.*; // -- OSCP5 LIBRARY --


// Constants
int     NUM_DOMINOS = 10;
int     SERVER_PORT = 12345;

float   SCOPE_SCALE = 1.0;
int     DEFAULT_WINDOW_WIDTH = 1600; // to change, also edit the setup() function
int     DEFAULT_WINDOW_HEIGHT = 1000; // to change, also edit the setup() function

// Window and drawing
int     curWidth = 0;
int     curHeight = 0;
float   scopeHScale = 0;
float   scopeVScale = 0;
int     scopeHSize = 0;
int     scopeVSize = 0;
int     scopeVOffset = 36;
float   housingHalfWidth;
float   housingLowerHeight;
float   housingUpperHeight;

// Timing
int     tick;
int     messageCount=0;
int     drawCount=0;
Boolean paused;
final Timer postSetupTimer = new Timer();

// FPS calculation
int     messageCountPrev=0;
int     drawCountPrev=0;
int     timePrev=0;
float   messageFps=0;
float   drawFps=0;

// Message handling
//int     messagePrev;

// Temp values used by draw()
PVector pivot      = new PVector();
PVector upperDir   = new PVector();
PVector leftDir    = new PVector();
PVector lowerLeft  = new PVector();
PVector lowerRight = new PVector();
PVector upperRight = new PVector();
PVector upperLeft  = new PVector();


// Communication
UDP udp; // -- UDP LIBRARY --
//  OscP5 oscP5; // -- OSCP5 LIBRARY --


// Data received
class SensorState
{
  SensorState() {sensorAccel=new PVector();}
  public int     sensorUpdateId;
  public float   sensorActive;
  public float   sensorAngle;
  public PVector sensorAccel;
  public float   sensorVeloc;
  public float   sensorTap;
  public float   sensorErr;
  public float   sensorDMX;
}
class DominoState
{
  DominoState() {sensorAccel=new PVector(); modeStatus=new String();}
  public float   sensorActive;
  public float   sensorAngle;
  public PVector sensorAccel;
  public float   sensorVeloc;
  public float   sensorDMX;
  public float   sensorVolume;
  public String  modeStatus;
  public float   modeValue;
}
DominoState states[];

class SensorUpdate
{
  SensorUpdate() {
    states=new SensorState[NUM_DOMINOS];
    for( int i=0; i<NUM_DOMINOS; i++ ) states[i]=new SensorState();
  }
  public SensorState states[];
  public int     id;
}
Vector<SensorUpdate> updates = new Vector<SensorUpdate>();
int latestUpdateId = 0;

void setup()
{ 
  size( 1600, 1000 ); // to change, also set DEFAULT_WINDOW_WIDTH and DEFAULT_WINDOW_HEIGHT
  surface.setResizable(true);
    
  // int port = 12345;
  // String ip = new String("192.168.1.5");
  // UDP udpTX = new UDP(this,port,ip);
  // udpTX.send("Hello",ip,port);
  // println("sending done");
    
  background(20);
  stroke(0);
  noSmooth();
  fill(0);
  rect( 0,0, width,height );  
  frameRate(240);
  
  tick = 0;
  timePrev = millis();
  paused = false;
  states = new DominoState[NUM_DOMINOS];
  for( int i=0; i<NUM_DOMINOS; i++ )
    states[i] = new DominoState();

  postSetupTimer.schedule(new TimerTask() { public void run() { redraw();} }, (long) (1));

  // Listen for connections on port 12345
  udp = new UDP( this, SERVER_PORT ); // -- UDP LIBRARY --
  udp.listen( true ); // -- UDP LIBRARY --
  //oscP5 = new OscP5(this,SERVER_PORT); // -- OSCP5 LIBRARY --
  
  // Draw only when new data packets received
  //noLoop(); // must be last line
}

void draw()
{
  //print("draw()"); print(drawCount);print(",");println(updates.size());
  drawCount++;

  if( paused )
  {
    while( updates.size()>0 )
      updates.remove(0);
    
    noStroke();  
    fill( 255,0,0 );
    rect( 4,4, width-80,10 );
    return;
  }

  if( (width!=curWidth) || (height!=curHeight) )
    onResize();
    
  for( int i=0; (i<5) && (updates.size()>0); i++ )
  {
    SensorUpdate update = updates.remove(0);
    
    int diff = (update.id-latestUpdateId); 
    //print("draw() ... sensor update,");println(diff);
    
    int catchup = 5;
    if( diff!=1 ) // out of order packet found, skip to latest
      catchup = 0;
      
    while( updates.size()>catchup ) // too many packets found, skip ahead
    {
      update = updates.remove(0);
      latestUpdateId=(update.id-1);
    }
      
    if( update.id>latestUpdateId )
      UpdateScopes( update );
  }
  
  UpdateBoxes();
}


void UpdateScopes( SensorUpdate update )
{
  if( latestUpdateId < (update.id-scopeHSize) )
    latestUpdateId = (update.id-scopeHSize);
  //print(" UpdateScopes "); println(update.id - latestUpdateId);

  //blendMode(SUBTRACT);
  //fill( 1,1,1 );
  //rect( 0,scopeVOffset-2, curWidth,scopeVSize+4 );  
  //blendMode(BLEND);
  //noStroke();  
  //fill( 0,5 );
  //rect( 0,scopeVOffset-2, curWidth,scopeVSize+4 );

  for( int i=latestUpdateId; i < update.id; i++, latestUpdateId++ )
  {
    //int hpos = 10 + (scopeHSize*i) + (5*i);
    //int scopePos = (update.id % scopeHSize);
    
    //blendMode(SUBTRACT);
    //fill( 1,1,1 );
    //rect( 0,scopeVOffset-2, curWidth,scopeVSize+4 );  
    //blendMode(BLEND);
    //fill( 0 );
    //rect( hpos+scopePos,scopeVOffset-2, hpos+scopePos,scopeVSize+4 );
    noStroke();  
    fill( 0,4 );
    rect( 0,scopeVOffset-2, curWidth,scopeVSize+4 );
  }

  for( int i = 0; i < NUM_DOMINOS; i++ )
  {
    SensorState state = update.states[i];
    states[i].sensorActive = state.sensorActive;
    states[i].sensorAngle = state.sensorAngle;
    states[i].sensorAccel = state.sensorAccel;
    states[i].sensorVeloc = state.sensorVeloc;
    states[i].sensorDMX = state.sensorDMX;
    states[i].sensorVolume = constrain(states[i].sensorVolume-0.02f,0,1) + (abs(state.sensorVeloc)*0.1);
        
    int hpos = 10 + (scopeHSize*i) + (5*i);
    int vpos; // input range is -32768 to 32767, signed 16-bit
    int scopePos = (update.id % scopeHSize);

    // Scope for X in yellow
//    vpos = round(scopeVOffset + (scopeVSize/2) + ((-state.sensorAccel.x) * (scopeVScale/2)));
//    noStroke();
//    fill( 20 );
//    rect( hpos+4, scopeVOffset-12, 32, 10);
//    fill( 255,255,64 );
//    stroke( 255,255,64);
//    point(hpos+scopePos, vpos);

    // Scope for Y in cyan
//    vpos = round(scopeVOffset + (scopeVSize/2) + ((-state.sensorAccel.y) * (scopeVScale/2)));
//    noStroke();
//    fill( 20 );
//    rect( hpos+54, scopeVOffset-12, 32, 10);
//    fill( 128,255,255 );
//    stroke( 128,255,255 );
//    point(hpos+scopePos, vpos);

    // Scope for velocity in magenta
    vpos = round(scopeVOffset + (scopeVSize/2) + ((state.sensorVeloc) * (scopeVScale/2)));
    //vpos = round(scopeVOffset + (scopeVSize/2) + ((-states[i].sensorVolume) * (scopeVScale/2)));
    noStroke();
    fill( 20 );
    rect( hpos+54, scopeVOffset-22, 32, 10);
    //fill( 255,64,255 );
    //stroke( 255,64,255 );
    fill( 180,0,255 );
    stroke( 180,0,255 );
    point(hpos+scopePos, vpos);

    // Scope for angle in white
    vpos = round(scopeVOffset + (scopeVSize/2) + ((-state.sensorAngle/3.14159) * (scopeVScale/2)));
    noStroke();
    fill( 20 );
    rect( hpos+4, scopeVOffset-22, 32, 10);
    fill( 192,192,192 );
    stroke( 192,192,192 );
    point(hpos+scopePos, vpos);
    
    if( state.sensorTap>1.0f )
    {
      float tapHeight = ((state.sensorTap-0.5)/20) * (scopeVSize/2);
      float tapVPos = (scopeVSize/2)+scopeVOffset;
      stroke(255,255,255);
      line( hpos+scopePos, tapVPos-tapHeight, hpos+scopePos, tapVPos+tapHeight );
      state.sensorTap=0;
    }
    
    if( state.sensorErr>0.5f )
    {
      fill(255,0,0);
      if( ((state.sensorErr>0.5f) && (state.sensorErr<1.5f)) || (state.sensorErr>2.5) )
        text( "ERROR: SENSOR", hpos+10, scopeVSize+scopeVOffset-16 );
      if( ((state.sensorErr>1.5f) && (state.sensorErr<2.5f)) || (state.sensorErr>2.5) )
        text( "ERROR: MUX", hpos+10, scopeVSize+scopeVOffset-32 );
    } 
  }
}

void UpdateBoxes()
{
  //println("UpdateBoxes()");
  noStroke();  
  fill( 20 );
  rect( 0, 0, width-80, scopeVOffset );

  fill( 0 ); //20
  rect( 0,scopeVSize+scopeVOffset+2, curWidth, curHeight-scopeVSize);
  
  textSize(11);
  
  for( int i = 0; i < NUM_DOMINOS; i++ )
  {
    DominoState state = states[i];

    int hpos = 10 + (scopeHSize*i) + (5*i);

    noStroke();

    // Scope for X in yellow
    fill( 255,255,64 );
    text( nfx(state.sensorAccel.x,1,2), hpos+4, scopeVOffset-2 );

    // Scope for Y in cyan
    fill( 128,255,255 );
    text( nfx(state.sensorAccel.y,1,2), hpos+54, scopeVOffset-2 );

    // Scope for velocity in magenta
    fill( 255,64,255 );
    text( nfx(state.sensorVeloc,1,3), hpos+54, scopeVOffset-12 );

    // Scope for angle in white
    fill( 192,192,192 );
    text( nfx(state.sensorAngle,1,2), hpos+4, scopeVOffset-12 );
        
    // Draw rotated boxes for the Dominos
    // Pivot is the rotation center of the box
    pivot.x = (scopeHSize/2) + 10 + (scopeHSize*i) + (5*i);
    pivot.y = curHeight-28;
    // Horizontal Dir and Vertical Dir are unit vectors pointing along the width and height of the box
    upperDir.x = -sin(states[i].sensorAngle); //*1.2f
    upperDir.y = -cos(state.sensorAngle); //*1.2f
    leftDir.x  = -upperDir.y; 
    leftDir.y  =  upperDir.x;
    // Thanks for making vector operations so gross, Processing
    lowerLeft  = PVector.sub( PVector.add(pivot,PVector.mult(leftDir,(housingHalfWidth))), PVector.mult(upperDir,(housingLowerHeight)) ); 
    lowerRight = PVector.sub( PVector.sub(pivot,PVector.mult(leftDir,(housingHalfWidth))), PVector.mult(upperDir,(housingLowerHeight)) ); 
    upperRight = PVector.add( PVector.sub(pivot,PVector.mult(leftDir,(housingHalfWidth))), PVector.mult(upperDir,(housingUpperHeight)) ); 
    upperLeft  = PVector.add( PVector.add(pivot,PVector.mult(leftDir,(housingHalfWidth))), PVector.mult(upperDir,(housingUpperHeight)) ); 
    
    float u = constrain( state.sensorActive*20, 0.0, 1.0);
    float dmx = lerp( 40,255, state.sensorDMX );
    stroke( 255*u + 192*(1-u), 192*(1-u), 192*(1-u) );
    fill( dmx, dmx, dmx ); 
    quad( lowerLeft.x, lowerLeft.y, lowerRight.x, lowerRight.y, upperRight.x, upperRight.y, upperLeft.x, upperLeft.y);
    fill(20,20,20);
    ellipse( pivot.x, pivot.y, 5, 5 );

    // Mode text
    fill(255,255,255);
    text( state.modeStatus, hpos+10, scopeVSize+scopeVOffset+16 );
    text( str(state.modeValue), hpos+10, scopeVSize+scopeVOffset+30 );

    // Column number text
    text( str(i), pivot.x-3, pivot.y+housingLowerHeight+15 );
  
    // Vertical dividing line
    if( i>0 )
    {
      stroke(255,255,255);
      hpos = 7 + (scopeHSize*i) + (5*i);
      line( hpos, scopeVOffset, hpos, scopeVSize+scopeVOffset+30 );
      line( hpos-1, (scopeVSize/2)+scopeVOffset, hpos+1, (scopeVSize/2)+scopeVOffset ); // tick at zero
      line( hpos-1, scopeVOffset, hpos+1, scopeVOffset ); // tick at +1
      line( hpos-1, scopeVSize+scopeVOffset, hpos+1, scopeVSize+scopeVOffset ); // tick at -1
    }
  }


  tick = (tick+1);
  if( (tick%40)==0 )
    UpdateFPS();
    
  rect( 0, 0, width, scopeVOffset );
  stroke( 128,64,64 );
  rect(curWidth-50, curHeight-60, 46, 56);
  fill( 128,64,64 );
  text( "FPS\n"+nf(messageFps,1,1)+"\n"+nf(drawFps,1,1), curWidth-44, curHeight-44 );//
}

void UpdateFPS()
{
  int timeNow = millis();
  int timeDiff = timeNow - timePrev;
  if( timeDiff>0 )
  {
    int messageDiff = messageCount-messageCountPrev;
    int drawDiff = drawCount-drawCountPrev;
    float messageFpsCur = 1000.0f * (messageDiff / (float)timeDiff);
    float drawFpsCur = 1000.0f * (drawDiff / (float)timeDiff);
    timePrev = timeNow;
    messageCountPrev = messageCount;
    drawCountPrev = drawCount;
    
    messageFps = constrain( (0.5f*messageFps) + (0.5f*messageFpsCur), 0, 1000 );
    drawFps = constrain( (0.5f*drawFps) + (0.5f*drawFpsCur), 0, 1000 );
  }
  
//  timeIndex = (timeIndex+1) % timeUpdate.length;
//  int timeOldest = timeUpdate[timeIndex];
//  timeUpdate[timeIndex] = timeNow;
//  float fpsNow = timeUpdate.length * (1000.0f / (timeNow-timeOldest));
//  if( timeIndex == 0 )
//    fps = fpsNow; // clears problems when fps becomes denormal or infinity
//  else fps = (0.95f*fps) + (0.05f*fpsNow);
}

void onResize()
{
  curWidth = width;
  curHeight = height;
  housingHalfWidth = (curWidth / NUM_DOMINOS) * 0.15;
  housingLowerHeight = housingHalfWidth * 0.33f;
  housingUpperHeight = housingHalfWidth * 8.0f;
  
  scopeHSize = (curWidth-80) / NUM_DOMINOS;
  scopeVSize = ((curHeight-scopeVOffset)-(80+(int)(8.5*housingHalfWidth)));
  scopeHScale = ((float)scopeHSize) / SCOPE_SCALE;
  scopeVScale = ((float)scopeVSize) / SCOPE_SCALE;
  // Housing dimensions are [0.4 x 0.8] relative to the width of a scope
}

void keyPressed() {
  print("keyPressed \n");
  if (key == ' ') {
    paused = !paused;
  }
}

String nfx( float x, int a, int b )
{
  String retval;
  if( x<0 ) retval = nf(x,a,b);
  else retval = (new String("  ")) + nf(x,a,b);
  return retval;
}

// -- UDP LIBRARY --
// fields within the packet data start at 32-bit (four byte) boundaries;
// calculate index values using a mask, round down to a multiple of four
int maskOffsetDWord = 0xFFFFFFFC; // zeroes two lowest bits, yields a multiple of four

// -- UDP LIBRARY --
void ProcessMessage( String tag, ByteBuffer buf, int argIndex )
{
  //println("ProcessMessage()");
  messageCount++;
  
  if( tag.equals( "/diag" ) )
  {
    SensorUpdate update = new SensorUpdate();

    int elementCount = 7;
    int preambleCount = 1;
    int updateId = (int)getFloatArg( buf, argIndex+0 );
    
    //if( updateId!=(latestUpdateId+1) )
    //{ print("packet out of order "); print(latestUpdateId); print(" -> "); println(updateId); }
    
    update.id = updateId;
    
    //messagePrev = messageCur;
    
    for( int i=0; i<NUM_DOMINOS; i++ )
    {
      SensorState state = update.states[i];
      
      int a = 4 * ( preambleCount + (i * elementCount) );
      float vecAngle = getFloatArg( buf, argIndex+a+0 );
      float vecX     = getFloatArg( buf, argIndex+a+4 );
      float vecY     = getFloatArg( buf, argIndex+a+8 );
      float active   = getFloatArg( buf, argIndex+a+12 );
      float dmx      = getFloatArg( buf, argIndex+a+16 );
      float err      = getFloatArg( buf, argIndex+a+20 );
      float veloc    = getFloatArg( buf, argIndex+a+24 );
      if( err>0.5 )
      {  print(" err_"); print(i);  }
      state.sensorUpdateId = updateId;
      state.sensorAngle = vecAngle;
      state.sensorAccel.x = vecX;
      state.sensorAccel.y = vecY;
      state.sensorVeloc = veloc;
      state.sensorErr = err;
      state.sensorDMX = dmx;
      
      if( active>=0.5f )
        state.sensorActive = 1.0;
      else
        state.sensorActive = 0.0;
        
      if( active>=1.0f )
      {
        //jprint("tap "); println(active);
        state.sensorTap = active;
      }
      else
        state.sensorTap = 0.0f;
    }
    updates.add(update);
  }
  else if( tag.equals( "/mode" ) )
  {    
    int a = 0;
    int index        = getIntArg( buf, argIndex+a+0 );
    String s         = getStringArg( buf, argIndex+a+4 );
    int len = (s.length()+1); // includes trailing zero
    int n = (len+3)&maskOffsetDWord;
    
    DominoState state = states[index];
    state.modeStatus = s;
    state.modeValue  = getFloatArg( buf, argIndex+a+4+n );
  }
  else if( tag.equals( "/broadcast" ) )
  {
    println("...polo...\n");
  }

  //redraw();
}

// -- UDP LIBRARY --
String getStringArg( ByteBuffer buf, int index )
{
  int argLength = 0;
  byte array[] = buf.array();
  for( int i=0; (i<array.length) && (array[i]!='\0'); i++ )
    argLength++;
  String arg = new String( array, index, argLength );
  return arg;
}

// -- UDP LIBRARY --
int getIntArg( ByteBuffer buf, int index )
{
  int arg = 0;
  if( (index+4)<buf.array().length )
  {
    arg = buf.getInt(index);
  }
  return arg;
}

// -- UDP LIBRARY --
float getFloatArg( ByteBuffer buf, int index )
{
  float arg = 0;
  if( (index+4)<=buf.array().length )
  {
    arg = buf.getFloat( index );
  }
  return arg;
}

// -- UDP LIBRARY --
void receive( byte[] data )
{  
  ByteBuffer buf = ByteBuffer.wrap(data);
  
  // Extract the OSC address tag from the data
  String tag = getStringArg(buf,0); 
  int addrTagEnd = tag.length();

  // Extract the OSC type tag
  int typeTagBegin = addrTagEnd+1;
  typeTagBegin = (typeTagBegin+3) & maskOffsetDWord;
  int typeTagEnd = typeTagBegin;
  if( (typeTagBegin<data.length) && (data[typeTagBegin]==',') )
  {
    for( int i=typeTagBegin; (i<data.length) && (data[i]!=0); i++ )
    {
      typeTagEnd++;
    }
  }

  // Extract the OSC arguments
  int argBegin = typeTagEnd+1;
  argBegin = (argBegin+3) & maskOffsetDWord;
  if( argBegin<data.length )
  {
    ProcessMessage( tag, buf, argBegin );
  }
}

/*
// -- OSCP5 LIBRARY --
void oscEvent(OscMessage oscMessage) {
  UpdateFPS();
  
  if (oscMessage.checkAddrPattern("/diag") == true)
  {
    int elementCount = 5;
    
    int numVecs = ((oscMessage.arguments().length) / elementCount);
    for( int i=0; i<numVecs; i++ )
    {
      int a = i * elementCount;
      float vecAngle = oscMessage.get(a+0).floatValue();
      float vecX = oscMessage.get(a+1).floatValue();
      float vecY = oscMessage.get(a+2).floatValue();
      float active = oscMessage.get(a+3).floatValue();
      float dmx = oscMessage.get(a+4).floatValue();
      sensorAngle[i] = vecAngle;
      sensorAccel[i].x = vecX;
      sensorAccel[i].y = vecY;
      sensorActive[i] = sensorActive[i]-0.02f;
      if( active>0.5f )
        sensorActive[i] = 1.0f;
      else if( sensorActive[i]<0.0f )
        sensorActive[i] = 0.0f;
      sensorDMX[i] = dmx;
    }
  }
  if (oscMessage.checkAddrPattern("/mode") == true)
  {
     int index = oscMessage.get(0).intValue();
     modeStatus[index] = oscMessage.get(1).stringValue();
     modeValue[index] = oscMessage.get(2).floatValue();
  }

  redraw();
}
*/


/* CODE JUNKYARD

  // Receive data from client
  c = s.available();
  while( (c != null) && (c.available()>0) )
  {
    input = c.readStringUntil('\n'); 
    if( input != null ) // why does this happen?
    {
      input = input.substring(0, input.indexOf("\n"));  // Only up to the newline
      String token[] = split(input, ' '); 
      int i = int(token[0]);
      float x = float(token[1]), y = float(token[2]), z = float(token[3]);
      vectors[ i ].x = x;
      vectors[ i ].y = y;
      vectors[ i ].z = z;
      if( mousePressed || (abs(x)>1) || (abs(y)>1) || (abs(z)>1) )
      {
        print( "message \""+input+"\"\n" );
      }
    }
  }

    
    int hposArrow = (scopeHSize/2) + 10 + (scopeHSize*i) + (5*i);
    int vposArrow = 720;
    hpos = round(hposArrow + (sensorAccel[i].x * scopeHScale/2));
    vpos = round(vposArrow + (sensorAccel[i].y * scopeHScale/2));

    // Arrow indicator line and dot
    noStroke();
    fill(255,255,255);
    ellipse( hpos, vpos, 5, 5 );    
    stroke(255,255,255);
    line( hposArrow, vposArrow, hpos, vpos );
    
    
    int lowerLeftX  = pivotX + ((leftDirX)  * housingHalfWidth) + ((-upperDirX) * housingBottomHeight);
    int lowerLeftY  = pivotY + ((leftDirY)  * housingHalfWidth) + ((-upperDirY) * housingBottomHeight);
    int lowerRightX = pivotX + ((-leftDirX) * housingHalfWidth) + ((-upperDirX) * housingBottomHeight);
    int lowerRightY = pivotY + ((-leftDirY) * housingHalfWidth) + ((-upperDirY) * housingBottomHeight);
    int upperRightX = pivotX + ((-leftDirX) * housingHalfWidth) + ((upperDirX)  * housingBottomHeight);
    int upperRightY = pivotY + ((-leftDirY) * housingHalfWidth) + ((upperDirY)  * housingBottomHeight);
    int upperLeftX  = pivotX + ((leftDirX)  * housingHalfWidth) + ((upperDirX)  * housingBottomHeight);
    int upperLeftY  = pivotY + ((leftDirX)  * housingHalfWidth) + ((upperDirX)  * housingBottomHeight);
*/