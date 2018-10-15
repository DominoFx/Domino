/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   common.h
 * Author: simon
 *
 * Created on April 25, 2018, 12:45 AM
 */

#ifndef COMMON_H
#define COMMON_H


//
// Helper macros
//

#define SAFE_DELETE(x) if((x)!=NULL) {delete (x); (x)=NULL;}
#define SAFE_DELETE_ARRAY(x) if((x)!=NULL) {delete[] (x); (x)=NULL;}

#define MIN( a,b ) ((a)<=(b)? (a):(b))
#define MAX( a,b ) ((a)>=(b)? (a):(b))
#define CLAMP( x, lo, hi )  ( (x)<(lo)? (lo) : ((x)>(hi)? (hi) : (x)) )
#define LERP( a,b, u ) (((a)*(1.0f-(u))) + ((b)*(u)))

#define ZERO_ARRAY(a,count,val) { for(int i=0; i<(count); i++ ) (a)[i]=(val); } 

//
// Helper functions
//

bool strcmp_s( const char* a, int aSizeMax, const char* b, int bSizeMax);
int little_endian( int x );

//
// Data types
//

enum Axis
{
    X = 1,
    Y = 2,
    Z = 4,
    XYZ = 7
};

//
// Data globals
//

extern int signal_shutdown; // signals threads to stop


#endif /* ENUMS_H */

