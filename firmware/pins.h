#ifndef PINS_H
#define PINS_H
#include "const.h"
#if MOTHERBOARD == 2
//GRBL
#define OPERATION_LED_PIN 0
#define X_STEP_PIN         2
#define X_DIR_PIN          5
#define X_ENABLE_PIN       8

#define X_STOP_PIN          9
#define X_STOP_ENGAGED      LOW

#define Y_STEP_PIN         3
#define Y_DIR_PIN          6
#define Y_ENABLE_PIN       8

#define Y_STOP_PIN         10
#define Y_STOP_ENGAGED     LOW

#define Z_STEP_PIN         3
#define Z_DIR_PIN          6
#define Z_ENABLE_PIN       8

#define Z_STOP_PIN         10
#define Z_STOP_ENGAGED     LOW


#define E_STEP_PIN         12
#define E_DIR_PIN          13
#define E_ENABLE_PIN       8

//#define Y_MS1_PIN           9
//#define Y_MS2_PIN           8

#define LASER_PW           13
#define LASER_PIN          12 // PWM 
#define FAN_PIN A3
#endif


#if MOTHERBOARD == 1
// RAMPS
#define OPERATION_LED_PIN 40
#define X_STEP_PIN         54
#define X_DIR_PIN          55
#define X_ENABLE_PIN       38
#define X_STOP_PIN          3
#define X_STOP_ENGAGED      LOW

//#define X_MS1_PIN          25
//#define X_MS2_PIN          26

#define Y_STEP_PIN         60
#define Y_DIR_PIN          61
#define Y_ENABLE_PIN       56
#define Y_STOP_PIN         14
#define Y_STOP_ENGAGED     LOW
//#define Y_MS1_PIN           9
//#define Y_MS2_PIN           8


#define Z_STEP_PIN         46
#define Z_DIR_PIN          48
#define Z_ENABLE_PIN       62
#define Z_STOP_PIN          18
#define Z_STOP_ENGAGED     LOW

#define E_STEP_PIN         26
#define E_DIR_PIN          28
#define E_ENABLE_PIN       24


#define LASER_PW           9
#define LASER_PIN          5
#define FAN_PIN 10
#endif
#endif
