#ifndef GLOBALS_H
#define GLOBALS_H

// Overall machine state

extern uint32_t position[]; // measured in steps
volatile extern uint16_t ms_since_step[];
extern uint16_t modal_feedrate;
extern uint16_t debug_counter;

extern uint8_t val;
extern char buffer[];
extern uint16_t buf_idx;

extern uint8_t input_ready;
extern uint8_t command_running;

// Motion control parameters
extern uint32_t position[];
extern uint32_t destination[];
extern int8_t   direction[];
extern uint16_t rate[]; // In tenths of ms
extern uint8_t  curve_starting;
extern uint16_t curve_pos, curve_dest; // Given in 1/100 degrees.
extern int16_t  curve_x, curve_y; // Note, these are absolutes from I and J respectively
extern uint16_t curve_radius, curve_feedrate; // radius in steps, feedrate in mm/min

extern uint8_t power;
extern uint8_t power_setpoint;

extern uint8_t motion_state;

#endif
