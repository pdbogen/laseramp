#ifndef CONST_H
#define CONST_H

#include <Arduino.h>

//#define DEBUG_OUTPUT
//#define DEBUG_QUEUE
//#define DEBUG_CURVE
//#define DEBUG_SERIAL
//#define DEBUG_POWER

#define STEPS_PER_MM ( 1600l / 15 )
#define UM_PER_STEP  ( 15000l / 1600l )

#define MAX_X 53 * STEPS_PER_MM
#define MAX_Y 100 * STEPS_PER_MM

#define BUFFER_SIZE 1024

#define X_POS HIGH
#define X_NEG LOW

#define Y_POS LOW
#define Y_NEG HIGH

// Higher numbers give higher max speed, but also higher min speed...
#define RATE_FACTOR 10l

#endif
