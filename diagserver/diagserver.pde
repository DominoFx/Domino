import processing.net.*;


// Constants
int NUM_DOMINOS = 10;
int SERVER_PORT = 12345;

// Window and drawing
int     curWidth = 0;
int     curHeight = 0;
float   scopeHScale = 0;
float   scopeVScale = 0;
int     scopeHSize = 0;
int     scopeVSize = 0;
int     scopeVOffset = 36;

// Timing
int tick;
float   sensorAngles[];
PVector sensorVectors[];
String inputs[];
String input;
Server s; 
Client c;

void setup()
{ 
  size(1280, 800);
  surface.setResizable(true);

  background(20);
  stroke(0);
  noSmooth();
  fill(20);
  rect( 0,0, width,height );  

  s = new Server(this, SERVER_PORT);  // Start a simple server on a port
  
  tick = 0;
  sensorVectors = new PVector[NUM_DOMINOS];
  for (int i = 0; i < sensorVectors.length; i++) {
    sensorVectors[i] = new PVector( -1.0, 0.0, 0.0 );
  }
}


void draw()
{
  fill( 20, 2 );
  noStroke();
  rect( 0,0, width-80, scopeVOffset );
  
  if( (width!=curWidth) || (height!=curHeight) )
    onResize();

  noStroke();  
  fill( 20, 5 );
  rect( 0,scopeVOffset-2, curWidth,scopeVSize+4 );  

  fill( 20 );
  rect( 0,scopeVSize+scopeVOffset+2, curWidth, curHeight-scopeVSize);

  for( int i = 0; i < sensorVectors.length; i++ )
  {    
    int hpos = 10 + (scopeHSize*i) + (5*i) + tick;
    int vpos; // input range is -32768 to 32767, signed 16-bit    

    // Scope for X in yellow
    vpos = round(scopeVOffset + (scopeVSize/2) + (-sensorVectors[i].x * (scopeVScale/2)));
    // Scope point
    stroke( 255,255,64);
    point(hpos+scopePos, vpos);

    // Scope for Y in cyan
    vpos = round(scopeVOffset + (scopeVSize/2) + (-sensorVectors[i].y * (scopeVScale/2)));
    // Scope point
    stroke( 128,255,255 );
    point(hpos+scopePos, vpos);
    
    // Scope for angle in white
    sensorAngles[i] = atan2( sensorVectors[i].x, sensorVectors[i].y );
    vpos = round(scopeVOffset + (scopeVSize/2) + ((-sensorAngles[i]/3.14159) * (scopeVScale/2)));
    stroke( 192,192,192 );
    point(hpos+scopePos, vpos);
        
    // Arrow indicator position
    int hposArrow = (ACCEL_SCOPE_HSIZE/2) + 10 + (ACCEL_SCOPE_HSIZE*i) + (5*i);
    int vposArrow = 720;
    hpos = round(hposArrow + (vectors[i].x * ACCEL_SCOPE_HSCALE/2));
    vpos = round(vposArrow + (vectors[i].y * ACCEL_SCOPE_HSCALE/2));

    // Arrow indicator line and dot
    noStroke();
    fill(255,255,255);
    ellipse( hpos, vpos, 5, 5 );    
    stroke(255,255,255);
    line( hposArrow, vposArrow, hpos, vpos );
    
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

    // Column number text
    text( str(i), hposArrow-10, vposArrow+70 );
  }
  
  tick = (tick+1) % 100;
  
  receive();
}


void onResize()
{
  curWidth = width;
  curHeight = height;
  
  scopeHSize = (curWidth-80) / NUM_DOMINOS;
  scopeVSize = ((curHeight-scopeVOffset)-(80+(int)(5*housingHalfWidth)));
  scopeHScale = ((float)scopeHSize);
  scopeVScale = ((float)scopeVSize);
}

void keyPressed() {
  print("keyPressed \n");
  if (key == ' ') {
    paused = !paused;
  }
}

// Receive data from client
// Blocks waiting for packet
void receive()
{
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
}
