#ifndef CONST_H
#define CONST_H

#include <Arduino.h>
//#define HOMEONSTART
#define BAUDRATE 115200
#define EASYCNCCONTROLLER
#ifdef EASYCNCCONTROLLER
#define EASYCNC_BUSY 4
#define EASYCNC_IDLE 0
#endif
//#define DEBUG_MOVE
//#define DEBUG_OUTPUT
//#define DEBUG_EXECUTE
//#define DEBUG_QUEUE
//#define DEBUG_CURVE
//#define DEBUG_SERIAL
//#define DEBUG_POWER

// MOTHERBOARDS
// 1 = RAMPS14
// 2 = GRBL

//#define CNCBOARD RAMPS14
#define MOTHERBOARD 1
//
#define LASERPRUSA
//#define CNCTesla


#define HOMESPEED 1000
#define Z_DRIVER_ENABLED

//#define E_DRIVER_ENABLED

#define SUPPORTLINENUMBERING
# define SUPPORTARCMOVES
// shut laser Off when stopping
#define LASERRESETSTOZERO
#define MAXSPEED 1000

// Models
// 1 = belts
// 2 = drives
#define DRIVEMODEL 1

#if DRIVEMODEL == 1
#define HOMEDELAY 10
#define MAXSPEED 5000
//#define STEPS_PER_MM ( 106.67 )
//#define STEPS_PER_MM ( 53.335  )
#define STEPS_PER_MM  100l
#define XSTEPS_PER_MM 100l
#define YSTEPS_PER_MM 100l
#define ESTEPS_PER_MM 100l
#ifdef Z_DRIVER_ENABLED
#define ZSTEPS_PER_MM 100l
#endif
#endif

#if DRIVEMODEL == 2
#define HOMEDELAY 1
#define MAXSPEED 50000
//#define STEPS_PER_MM ( 200.0 )
#define STEPS_PER_MM 800l
#define XSTEPS_PER_MM 800l
#define YSTEPS_PER_MM 800l
#define ESTEPS_PER_MM 100l
#ifdef Z_DRIVER_ENABLED
#define ZSTEPS_PER_MM 1600l
#endif
#endif



//#define UM_PER_STEP  ( 15000l / 1600l )

#ifdef LASERPRUSA
#define MAX_X 320l * XSTEPS_PER_MM
#define MAX_Y 630l * YSTEPS_PER_MM
#define MAX_Z 500l * ZSTEPS_PER_MM
#endif


#ifdef CNCTesla
#define MAX_X 190l * XSTEPS_PER_MM
#define MAX_Y 250l * YSTEPS_PER_MM
#define MAX_Z 100l * ZSTEPS_PER_MM
#endif

#if MOTHERBOARD == 1
#define BUFFER_SIZE 1024
#define QUEUESIZE 2
#endif


#if MOTHERBOARD == 2
#define BUFFER_SIZE 512
#define QUEUESIZE 2
#endif


//#define INVERSE_X
//#define INVERSE_Y
//#define INVERSE_Z

#ifdef INVERSE_X
#define X_POS LOW
#define X_NEG HIGH

#else
#define X_POS HIGH
#define X_NEG LOW
#endif

#ifdef INVERSE_Y
#define Y_POS LOW
#define Y_NEG HIGH
#else
#define Y_POS HIGH
#define Y_NEG LOW
#endif

#ifdef INVERSE_Z
#define Z_POS LOW
#define Z_NEG HIGH
#else
#define Z_POS HIGH
#define Z_NEG LOW
#endif
// Higher numbers give higher max speed, but also higher min speed...
#define RATE_FACTOR 100l

#endif
