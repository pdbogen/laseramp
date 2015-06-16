#ifndef _COMMAND_H
#define _COMMAND_H

#include <stdint.h>
#include <HardwareSerial.h>
#include "globals.h"

struct Command {
  Command * next;
  char    cmd_class;
  uint8_t s,p,
          cmd_id;
#ifdef Z_DRIVER_ENABLED
  int32_t z;
#endif
  int32_t x,
          y;
#ifdef E_DRIVER_ENABLED
  int32_t e;
#endif
  uint16_t f;
           int16_t c;
//#ifdef SUPPORTARCMOVES
  int32_t  i,
           j;
//#endif
  Command( char * buf );
  uint8_t execute();
};

void print_command( HardwareSerial s, Command * c );
void parse_g_command( Command * c, char **buf_ptr, char *line );
void parse_m_command( Command * c, char **buf_ptr, char *line );
void report_position();
void parseOutParameters(Command * c, char **buf_ptr);
#endif
