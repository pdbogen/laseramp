#include "strutil.h"
#include <stdlib.h>

// Consumes only integers and . from the buffer
float get_float( char ** source ) {
  char val_buffer[BUFFER_SIZE];
  char c;
  uint16_t i = 0;
  val_buffer[i] = '\0';
  skip_whitespace( source );
  while( (c = **source) ) {
    switch( c ) {
      case '0': case '1':
      case '2': case '3':
      case '4': case '5':
      case '6': case '7':
      case '8': case '9':
      case '-': case '.':
        val_buffer[i++] = c;
        val_buffer[i]   = '\0';
        (*source)++;
        break;
      default:
        return atof( val_buffer );
    }
  }
  return atof( val_buffer );
}

// Consumes only integers from the buffer
uint16_t get_int( char ** source ) {
  char val_buffer[BUFFER_SIZE];
  char c;
  uint16_t i = 0;
  val_buffer[i] = '\0';
  skip_whitespace( source );
  while( (c = **source) ) {
    switch( c ) {
      case '0': case '1':
      case '2': case '3':
      case '4': case '5':
      case '6': case '7':
      case '8': case '9':
      case '-':
        val_buffer[i++] = c;
        val_buffer[i]   = '\0';
        (*source)++;
        break;
      case '.':
        while( **source != ' ' && **source != '\t' && **source != '\0' && **source != '\n' )
          (*source)++;
      default:
        return atoi( val_buffer );
    }
  }
  return atoi( val_buffer );
}

void skip_whitespace( char ** source ) {
  while( **source == ' ' || **source == '\t' )
    (*source)++;
  return;
}
