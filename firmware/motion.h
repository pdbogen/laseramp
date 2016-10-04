#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>
#include <HardwareSerial.h>

extern uint8_t motion_state;
void linear_move( int32_t x, int32_t y, uint16_t f );
void linear_move_steps( int32_t x, int32_t y, uint16_t f );
void arc_move( int16_t x, int16_t y, uint16_t f, int16_t i, int16_t j, uint8_t direction );
void motion_control( HardwareSerial s );
void motion_control_home_primary();
void motion_control_home_secondary();
void home();

#define MS_IDLE     0
#define MS_LINEAR   1
#define MS_CW       2
#define MS_CCW      3
#define MS_SEG_CW   4
#define MS_SEG_CCW  5
#define MS_HOMING_1 6
#define MS_HOMING_2 7
#define MS_STOPPING 0xFF

// Interval in ms between steps, so lower is faster; minimum 0.
#define HOME_SPEED_PRIMARY 1
#define HOME_SPEED_SECONDARY 10

#endif
