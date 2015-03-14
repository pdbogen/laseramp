#include "command.h"
#include "strutil.h"
#include "const.h"
#include "globals.h"
#include "motion.h"

Command::Command( char *buf ) {
	char * buf_ptr = buf;
	this->cmd_class = '?';
	this->cmd_id = 0;
	this->x = position[0] * 100 / STEPS_PER_MM;
	this->y = position[1] * 100 / STEPS_PER_MM;
	this->e = 0;
	this->f = modal_feedrate;
	this->i = 0;
	this->j = 0;
	this->next = NULL;

	#ifdef DEBUG_OUTPUT
		Serial.print( F( "// Start position: (" ) );
		Serial.print( position[0] / STEPS_PER_MM );
		Serial.print( "," );
		Serial.print( position[1] / STEPS_PER_MM );
		Serial.println( ")" );
	#endif
	switch( *buf_ptr++ ) {
		case 'F':
			this->cmd_class = 'F';
			this->f = get_float( &buf_ptr );
			break;
		case 'G':
			parse_g_command( this, &buf_ptr, buf );
			break;
		default:
			Serial.print( F("// warning: unknown command " ) );
			Serial.println( buf );
			Serial.flush();
			break;
	}
	#ifdef DEBUG_OUTPUT
		Serial.print( F( "// Instantiated command from buffer: " ) );
		print_command( Serial, this );
	#endif
}

void parse_g_command( Command * c, char **buf_ptr, char *line ) {
	#ifdef DEBUG_OUTPUT
		Serial.println( F("// G command, reading numeric code..") );
	#endif
	c->cmd_class = 'G';
	// Parse out command ID
	switch( get_int( buf_ptr ) ) {
		case 0:
		case 1:
			c->cmd_id = 1;
			break;
		case 2:
			c->cmd_id = 2;
			break;
		case 3:
			c->cmd_id = 3;
			break;
		case 28:
			c->cmd_id = 28;
			return; // Early return, G28 ignores parameters
		default:
			Serial.print( F("// warning: unknown command " ) );
			Serial.println( line );
			Serial.flush();
			return; // Early return, unidentified command
	}
	// Parse out parameters
	while( **buf_ptr ) {
		skip_whitespace( buf_ptr );
		switch( **buf_ptr ) {
			case 'X':
				(*buf_ptr)++;
				c->x = (get_float( buf_ptr ) * 100.0);
				break;
			case 'Y':
				(*buf_ptr)++;
				c->y = (get_float( buf_ptr ) * 100.0);
				break;
			case 'E':
				(*buf_ptr)++;
				c->e = get_int( buf_ptr );
				break;
			case 'F':
				(*buf_ptr)++;
				c->f = get_float( buf_ptr );
				break;
			case 'I':
				(*buf_ptr)++;
				c->i = (get_float( buf_ptr ) * 100.0);
				break;
			case 'J':
				(*buf_ptr)++;
				c->j = (get_float( buf_ptr ) * 100.0);
				break;
			default:
				(*buf_ptr)++;
				break;
		}
	}
	return;
}

void print_command( HardwareSerial s, Command * c ) {
	s.print( "// " );
	s.print( c->cmd_class ); s.print( c->cmd_id );
	s.print( " X" ); s.print( c->x );
	s.print( " Y" ); s.print( c->y );
	s.print( " E" ); s.print( c->e );
	s.print( " I" ); s.print( c->i );
	s.print( " J" ); s.print( c->j );
	s.print( " F" ); s.println( c->f );
	s.flush();
}

uint8_t Command::execute() {
	switch( this->cmd_class ) {
		case 'G':
			switch( this->cmd_id ) {
				case 1:
					if( this->f > 0 ) {
						power_setpoint = this->e;
						linear_move( this->x, this->y, this->f );
						command_running = true;
					}
					modal_feedrate = this->f;
					break;
				case 2:
				case 3:
					if( this->f > 0 ) {
						power_setpoint = this->e;
						arc_move( this->x, this->y, this->f, this->i, this->j, (this->cmd_id==2?MS_CW:MS_CCW) );
						command_running = true;
					}
					break;
				case 28:
					command_running = true;
					home();
					command_running = false;
					break;
			}
			// Intentionally omit break;, so that G commands also update modal feedrate
		case 'F':
			modal_feedrate = this->f;
			break;
		default:
			command_running = false;
			return 0;
	}
	delete this;
	return 1;
}
