#ifndef _STRUTIL_H
#define _STRUTIL_H

#include "stdint.h"
#include "const.h"

float get_float( char ** source );
uint16_t get_int( char ** source );
void skip_whitespace( char ** source );

#endif
