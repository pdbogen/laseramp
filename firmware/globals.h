#ifndef GLOBALS_H
#define GLOBALS_H

// Overall machine state

extern uint32_t position[]; // measured in steps
volatile extern float ms_since_step[];
extern uint16_t modal_feedrate;
extern uint16_t debug_counter0,debug_counter1;
extern uint16_t buffleft;
extern uint8_t absolute_mode;
extern uint8_t val;
extern char buffer[];
extern uint16_t buf_idx;
extern uint16_t lineNumber;
extern  uint8_t input_ready;
extern  uint8_t command_running;

// Motion control parameters
extern uint32_t dwell_timer;
extern uint32_t realposition[];
extern uint32_t homeposition[];
extern uint32_t maxsize[];
extern uint32_t destination[];
extern int8_t   direction[];
extern float rate[]; // In tenths of ms
extern uint8_t  curve_starting;
extern uint32_t curve_pos, curve_dest; // Given in 1/100 degrees.
extern int32_t  curve_x, curve_y; // Note, these are absolutes from I and J respectively
extern uint32_t curve_radius, curve_feedrate; // radius in steps, feedrate in mm/min

extern uint8_t power;
extern uint8_t power_setpoint;

extern uint8_t motion_state;

#endif
