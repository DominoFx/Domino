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

#define SAFE_DELETE(x) if((x)!=nullptr) {delete (x); (x)=nullptr;}
#define SAFE_DELETE_ARRAY(x) if((x)!=nullptr) {delete[] (x); (x)=nullptr;}

#define MIN( a,b ) ((a)<=(b)? (a):(b))
#define MAX( a,b ) ((a)>=(b)? (a):(b))
#define CLAMP( x, lo, hi )  ( (x)<(lo)? (lo) : ((x)>(hi)? (hi) : (x)) )
#define LERP( a,b, u ) (((a)*(1.0f-(u))) + ((b)*(u)))

#define GET_FLAG( flags, bit ) ( ((flags) & (bit)) > 0 )
#define SET_FLAG( flags, bit, onOff ) ( (flags) = ( (onOff)?  ((flags)|(bit)) : ((flags)&~(bit)) ) )
#define TOGGLE_FLAG( flags, bit ) ( (flags) = ( ((flags)&(bit))?  ((flags)&~(bit)) : ((flags)|(bit)) ) )

#define ZERO_ARRAY(a,count,val) { for(int i=0; i<(count); i++ ) (a)[i]=(val); } 

//
// Helper functions
//

inline bool strcmp_s( const char* a, int aSizeMax, const char* b, int bSizeMax)
{
    int i = 0;
    for( i=0; (i<aSizeMax) && (a[i]!='\0') && (i<bSizeMax) && (b[i]!='\0'); i++ )
    {
        if( a[i]!=b[i] )
            break;
    }
    return ((a[i]=='\0') && (b[i]=='\0'));
}

inline int little_endian( int big )
{
    int little;
    char* dest = (char*)(&little);
    char* src  = (char*)(&big);
    dest[3] = src[0];
    dest[2] = src[1];
    dest[1] = src[2];
    dest[0] = src[3];
    return little;
}

inline float little_endian( float big )
{
    int temp = *(int*)(&big);
    temp = little_endian( temp );
    return *(float*)(&temp);
}


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

