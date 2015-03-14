#include <stdint.h>
#include "const.h"
#include "globals.h"

uint8_t val;
char buffer[BUFFER_SIZE];
uint16_t buf_idx;

// Current system state
uint32_t position[2]    = {0,0};
uint32_t destination[2] = {0,0}; // currently in steps
uint16_t modal_feedrate = 0;
int8_t direction[2] = {0,0};
uint16_t debug_counter = 0;

uint8_t curve_starting = 0;
uint16_t curve_pos, curve_dest; // Given in 1/100 degrees.
int16_t    curve_x, curve_y; // Note, these are absolutes from I and J respectively
uint16_t    curve_radius, curve_feedrate; // radius in steps, feedrate in mm/min

uint16_t rate[2] = {0,0}; // Rate is (half of) ms per step * 100
volatile uint16_t ms_since_step[] = {0,0}; // tenths of ms since last step

uint8_t power          = 0,
        power_setpoint = 0;

uint8_t input_ready = false;
uint8_t command_running = false;
