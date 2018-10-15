import java.util.Timer;
import java.util.TimerTask;
import java.nio.ByteBuffer;
import hypermedia.net.*; // -- UDP LIBRARY --
//  import processing.net.*; // -- OSCP5 LIBRARY --
//  import oscP5.*; // -- OSCP5 LIBRARY --
//  import netP5.*; // -- OSCP5 LIBRARY --


// Constants
int     NUM_DOMINOS = 10;
int     SERVER_PORT = 12345;

float   SCOPE_SCALE = 1.0;
int     DEFAULT_WINDOW_WIDTH = 1280; // to change, also edit the setup() function
int     DEFAULT_WINDOW_HEIGHT = 800; // to change, also edit the setup() function

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
Boolean paused;
int     timeUpdate[];
int     timeIndex;
float   fps;
final Timer postSetupTimer = new Timer();


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
float   sensorAngles[];
PVector sensorVectors[];
float   sensorActive[];
float   sensorTap[];
float   sensorErr[];
float   sensorDMX[];
String  modeStatus[];
float   modeValue[];


void setup()
{ 
  size( 1280, 800 ); // set to DEFAULT_WINDOW_WIDTH and DEFAULT_WINDOW_HEIGHT
  surface.setResizable(true);
    
  background(20);
  stroke(0);
  noSmooth();
  fill(20);
  rect( 0,0, width,height );  
  //frameRate(100);
  
  tick = 0;
  paused = false;
  timeUpdate = new int[20];
  timeIndex = 0;
  fps = 0;
  sensorAngles = new float[NUM_DOMINOS];
  sensorVectors = new PVector[NUM_DOMINOS];
  sensorActive = new float[NUM_DOMINOS];
  sensorTap = new float[NUM_DOMINOS];
  sensorErr = new float[NUM_DOMINOS];
  sensorDMX = new float[NUM_DOMINOS];
  modeStatus = new String[NUM_DOMINOS];
  modeValue = new float[NUM_DOMINOS];
  for (int i = 0; i < sensorVectors.length; i++) {
    sensorAngles[i] = -1;
    sensorVectors[i] = new PVector(0,1,0);
    sensorActive[i] = 0;
    sensorTap[i] = 0;
    sensorDMX[i] = 0;
    modeStatus[i] = new String();
  }

  postSetupTimer.schedule(new TimerTask() { public void run() { redraw();} }, (long) (1));

  // Listen for connections on port 12345
  udp = new UDP( this, SERVER_PORT ); // -- UDP LIBRARY --
  udp.listen( true ); // -- UDP LIBRARY --
  //oscP5 = new OscP5(this,SERVER_PORT); // -- OSCP5 LIBRARY --
  
  // Draw only when new data packets received
  noLoop(); // must be last line
}

void draw()
{  
  if( paused )
  {
    noStroke();  
    fill( 255,0,0 );
    rect( 4,4, width-80,10 );
    return;
  }

  noStroke();  
  fill( 20 );
  rect( 0, 0, width-80, scopeVOffset );


  if( (tick%40)==0 )
  {
    rect( 0, 0, width, scopeVOffset );
    stroke( 255,255,255 );
    rect(curWidth-70, 4, 64,15 );
    fill(255,255,255);
    text( "FPS "+nf(fps,1,1), curWidth-66, 16 );//
  }
  
  
  if( (width!=curWidth) || (height!=curHeight) )
    onResize();

  noStroke();  
  fill( 20, 5 );
  rect( 0,scopeVOffset-2, curWidth,scopeVSize+4 );  

  fill( 20 );
  rect( 0,scopeVSize+scopeVOffset+2, curWidth, curHeight-scopeVSize);
  
  textSize(11);
  
  for( int i = 0; i < sensorVectors.length; i++ )
  {    
    int hpos = 10 + (scopeHSize*i) + (5*i);
    int vpos; // input range is -32768 to 32767, signed 16-bit    
    int scopePos = (tick%scopeHSize);

    
    // Scope for X in yellow
    vpos = round(scopeVOffset + (scopeVSize/2) + ((-sensorVectors[i].x) * (scopeVScale/2)));
    noStroke();
    fill( 20 );
    rect( hpos+4, scopeVOffset-12, 32, 10);
    fill( 255,255,64 );
    text( nf(sensorVectors[i].x,1,2), hpos+4, scopeVOffset-2 );
    stroke( 255,255,64);
    point(hpos+scopePos, vpos);
    //point(hpos+scopePos, vpos+1);

    // Scope for Y in cyan
    vpos = round(scopeVOffset + (scopeVSize/2) + ((-sensorVectors[i].y) * (scopeVScale/2)));
    noStroke();
    fill( 20 );
    rect( hpos+54, scopeVOffset-12, 32, 10);
    fill( 128,255,255 );
    text( nf(sensorVectors[i].y,1,2), hpos+54, scopeVOffset-2 );
    stroke( 128,255,255 );
    point(hpos+scopePos, vpos);

    // Scope for angle in white
    vpos = round(scopeVOffset + (scopeVSize/2) + ((-sensorAngles[i]/3.14159) * (scopeVScale/2)));
    noStroke();
    fill( 20 );
    rect( hpos+4, scopeVOffset-22, 32, 10);
    fill( 192,192,192 );
    text( nf(sensorAngles[i],1,2), hpos+4, scopeVOffset-12 );
    stroke( 192,192,192 );
    point(hpos+scopePos, vpos);
    
    if( sensorTap[i]>0.5f )
    {
      //{ print("tap - "); println(i); }
      stroke(255,0,0);
      line( hpos+scopePos, (scopeVSize/2)+scopeVOffset-50, hpos+scopePos, (scopeVSize/2)+scopeVOffset+50 );
      sensorTap[i]=0;
    }
    
    if( sensorErr[i]>0.5f )
    {
      fill(255,0,0);
      if( ((sensorErr[i]>0.5f) && (sensorErr[i]<1.5f)) || (sensorErr[i]>2.5) )
        text( "ERROR: SENSOR", hpos+10, scopeVSize+scopeVOffset-16 );
      if( ((sensorErr[i]>1.5f) && (sensorErr[i]<2.5f)) || (sensorErr[i]>2.5) )
        text( "ERROR: MUX", hpos+10, scopeVSize+scopeVOffset-32 );
    }
    

    // Draw rotated boxes for the Dominos
    // Pivot is the rotation center of the box
    pivot.x = (scopeHSize/2) + 10 + (scopeHSize*i) + (5*i);
    pivot.y = curHeight-28;
    // Horizontal Dir and Vertical Dir are unit vectors pointing along the width and height of the box
    upperDir.x = -sin(sensorAngles[i]); //*1.2f
    upperDir.y = -cos(sensorAngles[i]); //*1.2f
    leftDir.x  = -upperDir.y; 
    leftDir.y  =  upperDir.x;
    // Thanks for making vector operations so gross, Processing
    lowerLeft  = PVector.sub( PVector.add(pivot,PVector.mult(leftDir,(housingHalfWidth))), PVector.mult(upperDir,(housingLowerHeight)) ); 
    lowerRight = PVector.sub( PVector.sub(pivot,PVector.mult(leftDir,(housingHalfWidth))), PVector.mult(upperDir,(housingLowerHeight)) ); 
    upperRight = PVector.add( PVector.sub(pivot,PVector.mult(leftDir,(housingHalfWidth))), PVector.mult(upperDir,(housingUpperHeight)) ); 
    upperLeft  = PVector.add( PVector.add(pivot,PVector.mult(leftDir,(housingHalfWidth))), PVector.mult(upperDir,(housingUpperHeight)) ); 
    
    float u = sensorActive[i];
    float dmx = lerp( 40,255, sensorDMX[i] );
    stroke( 255*u + 192*(1-u), 192*(1-u), 192*(1-u) );
    fill( dmx, dmx, dmx ); 
    quad( lowerLeft.x, lowerLeft.y, lowerRight.x, lowerRight.y, upperRight.x, upperRight.y, upperLeft.x, upperLeft.y);
    fill(20,20,20);
    ellipse( pivot.x, pivot.y, 5, 5 );

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
    
    // Mode text
    fill(255,255,255);
    text( modeStatus[i], hpos+10, scopeVSize+scopeVOffset+16 );
    text( str(modeValue[i]), hpos+10, scopeVSize+scopeVOffset+30 );

    // Column number text
    text( str(i), pivot.x-3, pivot.y+housingLowerHeight+15 );
  }
  
  tick = (tick+1);
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

void UpdateFPS()
{
  int timeNow = millis();
  int timeOldest = timeUpdate[timeIndex];
  timeUpdate[timeIndex] = timeNow;
  timeIndex = (timeIndex+1) % timeUpdate.length;
  float fpsNow = timeUpdate.length * (1000.0f / (timeNow-timeOldest));
  if( timeIndex == 0 )
    fps = fpsNow; // clears problems when fps becomes denormal or infinity
  else fps = (0.95f*fps) + (0.05f*fpsNow);
}


// -- UDP LIBRARY --
// fields within the packet data start at 32-bit (four byte) boundaries;
// calculate index values using a mask, round down to a multiple of four
int maskOffsetDWord = 0xFFFFFFFC; // zeroes two lowest bits, yields a multiple of four

// -- UDP LIBRARY --
void ProcessMessage( String tag, ByteBuffer buf, int argIndex )
{
  UpdateFPS();
  
  if( tag.equals( "/diag" ) )
  {
    int fieldCount = 6;
    for( int i=0; i<NUM_DOMINOS; i++ )
    {
      int a = (i * fieldCount * 4);
      float vecAngle = getFloatArg( buf, argIndex+a+0 );
      float vecX     = getFloatArg( buf, argIndex+a+4 );
      float vecY     = getFloatArg( buf, argIndex+a+8 );
      float active   = getFloatArg( buf, argIndex+a+12 );
      float dmx      = getFloatArg( buf, argIndex+a+16 );
      float err      = getFloatArg( buf, argIndex+a+20 );
      if( err>0.5 )
      {  print("err - "); println(i);  }
      sensorAngles[i] = vecAngle;
      sensorVectors[i].x = vecX;
      sensorVectors[i].y = vecY;
      sensorActive[i] = sensorActive[i]-0.02f;
      if( active>10.0f )
      {
        //{  print("tap - "); println(i);  }
        sensorTap[i] = 1.0f;
      }
      
      if( active>0.5f )
        sensorActive[i] = 1.0f;
      else if( sensorActive[i]<0.0f )
        sensorActive[i] = 0.0f;
      sensorDMX[i] = dmx;
      sensorErr[i] = err;
    }
  }
  else if( tag.equals( "/mode" ) )
  {
    int a = 0;
    int index        = getIntArg( buf, argIndex+a+0 );
    String s         = getStringArg( buf, argIndex+a+4 );
    int len = (s.length()+1); // includes trailing zero
    int n = (len+3)&maskOffsetDWord;
    modeStatus[index] = s;
    modeValue[index]  = getFloatArg( buf, argIndex+a+4+n );
  }

  redraw();
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
      typeTagEnd++;
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
      sensorAngles[i] = vecAngle;
      sensorVectors[i].x = vecX;
      sensorVectors[i].y = vecY;
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
    hpos = round(hposArrow + (sensorVectors[i].x * scopeHScale/2));
    vpos = round(vposArrow + (sensorVectors[i].y * scopeHScale/2));

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