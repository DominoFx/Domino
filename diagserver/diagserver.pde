import processing.net.*;


float ACCEL_SCALE = 1.0;
int ACCEL_SCOPE_HSIZE = 100;
int ACCEL_SCOPE_VSIZE = 600;
float ACCEL_SCOPE_HSCALE = ((float)ACCEL_SCOPE_HSIZE) / ACCEL_SCALE;
float ACCEL_SCOPE_VSCALE = ((float)ACCEL_SCOPE_VSIZE) / ACCEL_SCALE;
int SERVER_PORT = 12345;


int tick;
PVector vectors[];
String inputs[];
String input;
Server s; 
Client c;

void setup()
{ 
  size(1280, 800);
  background(255);
  stroke(0);
  noSmooth();
  frameRate(100);
  s = new Server(this, SERVER_PORT);  // Start a simple server on a port
  
  tick = 0;
  vectors = new PVector[12];
  for (int i = 0; i < vectors.length; i++) {
    vectors[i] = new PVector(0.1*ACCEL_SCALE,0.0*ACCEL_SCALE,-0.1*ACCEL_SCALE);
  }
}


void draw()
{
  fill( 20, 2 );
  noStroke();
  rect( 0,0, 1280,800 );
  
  for( int i = 0; i < vectors.length; i++ )
  {    
    int hpos = 10 + (ACCEL_SCOPE_HSIZE*i) + (5*i) + tick;
    int vpos; // input range is -32768 to 32767, signed 16-bit    

    // Scope for Z in blue
    vpos = round(10 + (ACCEL_SCOPE_VSIZE/2) + (vectors[i].z * ACCEL_SCOPE_VSCALE/2));
    stroke(128,128,255);
    point(hpos, vpos);
    point(hpos, vpos+1);

    // Scope for Y in green
    vpos = round(10 + (ACCEL_SCOPE_VSIZE/2) + (vectors[i].y * ACCEL_SCOPE_VSCALE/2));
    stroke(128,255,128);
    point(hpos, vpos);
    point(hpos, vpos+1);
    
    // Scope for X in red
    vpos = round(10 + (ACCEL_SCOPE_VSIZE/2) + (vectors[i].x * ACCEL_SCOPE_VSCALE/2));
    stroke(255,128,128);
    point(hpos, vpos);
    point(hpos, vpos+1);
    
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
      hpos = 7 + (100*i) + (5*i);
      line( hpos, 10, hpos, 640 );
      line( hpos-1, 310, hpos+1, 310 );
    }

    // Columne number text
    text( str(i), hposArrow-10, vposArrow+70 );
  }
  
  tick = (tick+1) % 100;
  
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
}