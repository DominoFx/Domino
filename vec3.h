/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Vec3.h
 * Author: michaelson
 *
 * Created on June 17, 2018, 3:41 AM
 */

#ifndef VEC3_H
#define VEC3_H
 
#include <stdint.h>

struct WVec3Data_t { int16_t x, y, z; };
struct FVec3Data_t { float   x, y, z; };
struct DVec3Data_t { double  x, y, z; };

// Vector of three 16-bit word values
struct WVec3_t : public WVec3Data_t
{
    WVec3_t() { x = y = z = 0; }
    int16_t operator[]( int i ) const
    { return (i==0? x : (i==1? y : (i==2? z : 0))); }
    //int16_t x, y, z; // inherited from WVec3Data_t
};

// Vector of three 64-bit float values
struct FVec3_t : public FVec3Data_t
{
    FVec3_t() { x = y = z = 0; }
    FVec3_t( const WVec3Data_t& that ) { x=that.x, y=that.y, z=that.z; }
    FVec3_t( const FVec3Data_t& that ) { x=that.x, y=that.y, z=that.z; }
    FVec3_t( const DVec3Data_t& that ) { x=that.x, y=that.y, z=that.z; }
    FVec3_t& operator=( const WVec3Data_t& that )
    { this->x = that.x, this->y = that.y, this->z = that.z; return *this; }
    FVec3_t& operator=( const FVec3Data_t& that )
    { this->x = that.x, this->y = that.y, this->z = that.z; return *this; }
    FVec3_t& operator=( const DVec3Data_t& that )
    { this->x = that.x, this->y = that.y, this->z = that.z; return *this; }
    FVec3_t& operator+=( const FVec3Data_t& that )
    { this->x += that.x, this->y += that.y, this->z += that.z; }
    FVec3_t& operator*=( float f )
    { this->x *= f, this->y *= f, this->z *= f; }
    float operator[]( int i ) const
    { return (i==0? x : (i==1? y : (i==2? z : 0))); }
    //float x, y, z; // inherited from FVec3Data_t
};

// Vector of three 64-bit double values
struct DVec3_t : public DVec3Data_t
{
    DVec3_t() { x = y = z = 0; }
    DVec3_t( const WVec3Data_t& that ) { x=that.x, y=that.y, z=that.z; }
    DVec3_t( const FVec3Data_t& that ) { x=that.x, y=that.y, z=that.z; }
    DVec3_t( const DVec3Data_t& that ) { x=that.x, y=that.y, z=that.z; }
    DVec3_t& operator+=( const WVec3Data_t& that )
    { this->x += that.x, this->y += that.y, this->z += that.z; return *this; }
    DVec3_t& operator+=( const DVec3Data_t& that )
    { this->x += that.x, this->y += that.y, this->z += that.z; return *this; }
    DVec3_t& operator-=( const DVec3Data_t& that )
    { this->x -= that.x, this->y -= that.y, this->z -= that.z; return *this; }
    DVec3_t& operator*=( double d )
    { this->x *= d, this->y *= d, this->z *= d; }
    DVec3_t& operator/=( double d )
    { this->x /= d, this->y /= d, this->z /= d; }
    DVec3_t& operator=( const WVec3Data_t& that )
    { this->x = that.x, this->y = that.y, this->z = that.z; return *this; }
    DVec3_t& operator=( const FVec3Data_t& that )
    { this->x = that.x, this->y = that.y, this->z = that.z; return *this; }
    DVec3_t& operator=( const DVec3Data_t& that )
    { this->x = that.x, this->y = that.y, this->z = that.z; return *this; }
    double operator[]( int i ) const
    { return (i==0? x : (i==1? y : (i==2? z : 0))); }
    //double x, y, z; // inherited from DVec3Data_t
};

#endif // VEC3_H
