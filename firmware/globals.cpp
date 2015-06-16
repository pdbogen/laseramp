#include <stdint.h>
#include "const.h"
#include "globals.h"

uint8_t absolute_mode = 1;
uint8_t val;
char buffer[BUFFER_SIZE];
uint16_t buf_idx;
uint16_t lineNumber = 0;
// Current system state
uint16_t buffleft=0;
#ifndef Z_DRIVER_ENABLED
uint32_t realposition[2] = {0, 0}; // distance in mm from absolute Zero
uint32_t position[2]    = {0, 0}; // distance in steps from absolute Zero
uint32_t destination[2] = {0, 0}; // currently in steps
uint32_t homeposition[2] = {0, 0}; // currently in steps
int8_t direction[2] = {0, 0};
uint32_t maxsize[2]    = {MAX_X, MAX_Y};
#else
uint32_t realposition[3] = {0, 0, 0}; // distance in mm from absolute Zero
uint32_t position[3]    = {0, 0, 0}; // distance in steps from absolute Zero
uint32_t destination[3] = {0, 0, 0}; // currently in steps
uint32_t homeposition[3] = {0, 0, 0}; // currently in steps
int8_t direction[3] = {0, 0, 0};
uint32_t maxsize[3]    = {MAX_X,MAX_Y,MAX_Z};
#endif
uint16_t modal_feedrate = 1000;
uint16_t debug_counter0 = 0;
uint16_t debug_counter1 = 0;

uint8_t curve_starting = 0;
uint32_t curve_pos, curve_dest; // Given in 1/100 degrees.
int32_t    curve_x, curve_y; // Note, these are absolutes from I and J respectively
uint32_t    curve_radius, curve_feedrate; // radius in steps, feedrate in mm/min
#ifndef Z_DRIVER_ENABLED
float rate[2] = {0.0, 0.0}; // Rate is (half of) ms per step * 100
volatile float ms_since_step[2] = {0.0, 0.0}; // tenths of ms since last step
#else
float rate[3] = {0.0, 0.0}; // Rate is (half of) ms per step * 100
volatile float ms_since_step[3] = {0.0, 0.0, 0.0}; // tenths of ms since last step
#endif
uint8_t power          = 0,
        power_setpoint = 0;

 uint8_t input_ready = false;
 uint8_t command_running = false;
