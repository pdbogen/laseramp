#ifndef MOTION_H
#define MOTION_H

#include "globals.h"
#include "const.h"


extern uint8_t motion_state;

#ifdef Z_DRIVER_ENABLED
void linear_move( int32_t x, int32_t y, int32_t z,uint16_t f );
void linear_move_steps( int32_t x, int32_t y, int32_t z, uint16_t f );
#else
void linear_move( int32_t x, int32_t y, uint16_t f );
void linear_move_steps( int32_t x, int32_t y, uint16_t f );
#endif

void dwell(uint8_t t );

#ifdef SUPPORTARCMOVES
void arc_move( int32_t x, int32_t y, uint16_t f, int32_t i, int32_t j, uint8_t direction );
#endif
void motion_control( HardwareSerial s );
void power_control();
void motion_control_home_primary();
void motion_control_home_secondary();
void motion_control_dwell_primary();
void motion_control_dwell_secondary();
void emergency_stop();
void disable_motors();
void enable_motors();
void home();
 #ifndef Z_DRIVER_ENABLED
void G0_move( int32_t x, int32_t y, uint16_t f ) ;
#else
void G0_move( int32_t x, int32_t y, int32_t z, uint16_t f ) ;
#endif
void motion_control_g0_secondary();

#define MS_IDLE     0
#define MS_LINEAR   1
#define MS_CW       2
#define MS_CCW      3
#define MS_SEG_CW   4
#define MS_SEG_CCW  5
#define MS_HOMING_1 6
#define MS_HOMING_2 7
#define MS_DWELLING_1 8
#define MS_DWELLING_2 9
#define MS_G0_1 10
#define MS_G0_2 11

#define MS_STOPPING 0xFF

#endif
