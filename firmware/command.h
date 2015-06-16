#ifndef _COMMAND_H
#define _COMMAND_H

#include <stdint.h>
#include <HardwareSerial.h>
#include "globals.h"

struct Command {
  Command * next;
  char    cmd_class;
  uint8_t cmd_id;
  uint16_t x,
           y,
           e,
           f;
  int16_t  i,
           j;
  Command() {
    this->cmd_class = '?';
    this->cmd_id = 0;
    this->x = 0;
    this->y = 0;
    this->e = 0;
    this->f = modal_feedrate;
    this->i = 0;
    this->j = 0;
    this->next = NULL;
  }
  Command( char * buf );
  Command( uint16_t x, uint16_t y, uint16_t e, uint16_t f ) {
    this->cmd_class = '?';
    this->cmd_id = 0;
    this->x = x;
    this->y = y;
    this->e = e;
    this->f = f;
    this->i = 0;
    this->j = 0;
    this->next = NULL;
  }
  Command( uint16_t x, uint16_t y, uint16_t e, uint16_t f, int16_t i, int16_t j ) {
    this->cmd_class = '?';
    this->cmd_id = 0;
    this->x = x;
    this->y = y;
    this->e = e;
    this->f = f;
    this->i = i;
    this->j = j;
    this->next = NULL;
  }
  uint8_t execute();
};

void print_command( HardwareSerial s, Command * c );
void parse_g_command( Command * c, char **buf_ptr, char *line );


#endif
