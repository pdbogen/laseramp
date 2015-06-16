#include "const.h"
#include "command.h"
#include "strutil.h"
#include "globals.h"
#include "motion.h"
#include "pins.h"
int counter = 0;
Command::Command( char *buf ) {
  char * buf_ptr = buf;
  this->cmd_class = '?';
  this->cmd_id = 0;

  this->x = 0;
  this->y = 0;
#ifdef Z_DRIVER_ENABLED
  this->z = 0;
#endif
#ifdef E_DRIVER_ENABLED
  this->e = 0;
#endif
  this->f = modal_feedrate;
#ifdef SUPPORTARCMOVES
  this->i = 0;
  this->j = 0;
#endif
  this->s = 0;
  this->p = 0;
  this->next = NULL;
  this->c = ++counter;
#ifdef DEBUG_OUTPUT
  Serial.print( F( "// Start position: (" ) );
  Serial.print( position[0] / XSTEPS_PER_MM );
  Serial.print( "," );
  Serial.print( position[1] / YSTEPS_PER_MM );
#ifdef Z_DRIVER_ENABLED
  Serial.print( "," );
  Serial.print( position[2] / ZSTEPS_PER_MM );
#endif
  Serial.println( ")" );
#endif

#ifdef SUPPORTLINENUMBERING
  if (toupper(*buf_ptr) == 'N') {
    *buf_ptr++;
    lineNumber = get_int( &buf_ptr );
    skip_whitespace( &buf_ptr );
  }
#endif
  switch (toupper( *buf_ptr++) ) {
    case 'F':
      this->cmd_class = 'F';
      this->f = get_float( &buf_ptr );
      break;
    case 'M':
      parse_m_command( this, &buf_ptr, buf );
      break;
    case 'G':
      parse_g_command( this, &buf_ptr, buf );
      break;
    case ';': //redundant code, comments should never get this far
    //ignore
    case '/': //redundant code, comments should never get this far
      //ignore
      break;
    default:
      Serial.print( F("// warning: unknown command " ) );
      Serial.print(this->c) ; Serial.print(" ");
      Serial.println( buf );
      Serial.flush();
      break;
  }
#ifdef DEBUG_OUTPUT
  Serial.print( F( "// Instantiated command from buffer: " ) );
  print_command( Serial, this );
#endif
}

void parse_m_command( Command * c, char **buf_ptr, char *line ) {
#ifdef DEBUG_OUTPUT
  Serial.println( F("// M command, reading numeric code..") );
#endif
  c->cmd_class = 'M';
  // Parse out command ID
  switch ( get_int( buf_ptr ) ) {
    case 0: //Stop or Unconditional Stop
      c->cmd_id = 0;
      return ;
      break;
    case 1: // M1 Mark start of batch job
      c->cmd_id = 1;
      break;
    case 2: // M2 Mark end of batch job
      c->cmd_id = 2;
      break;
    case 3: //M3 turn laser on
      c->cmd_id = 3;
      break;
    case 5: // M5 Laser Off
      c->cmd_id = 5;
      return ;
      break;
    case 17: //M17 Enable/Power all stepper motors
      c->cmd_id = 17;
      return ;
      break;
    case 18: //M18 Disable all stepper motors
      c->cmd_id = 18;
      return ;
      break;
    case 24: //M24 Continue
      c->cmd_id = 24;
      return ;
      break;
    case 25: //M25 Pause
      c->cmd_id = 25;
      return ;
      break;
    case 42: //M42 P<pin> S<power>
      c->cmd_id = 42;
      break;
    case 84: //M84 disable motors
      c->cmd_id = 84;
      break;
    case 90: //home Z to --
      break;
    case 91: //home X Y to --
      break;
    case 92: //move X to ++
      break;
    case 93: //move Y to ++
      break;
    case 94:// move Z to ++
      break;
    case 105: //report temperature
      break;
    case 106: //set LASER power
      c->cmd_id = 106;
      break;
    case 107: //set LASER power Off
      c->cmd_id = 107;
      break;
    case 110: //N0 M110....
      // set N counter
      c->cmd_id = 110;
      return;
      break;
    case 112: //M112 Emergency stop
      c->cmd_id = 112;
      emergency_stop();
      break;
    case 114: //M114 report current position
      c->cmd_id = 114;
      return;
    default:
      Serial.print( F("// warning: unknown command " ) );
      Serial.print(c->c) ; Serial.print(" ");
      Serial.println( line );
      Serial.flush();
      return; // Early return, unidentified command
  }
  parseOutParameters(c, buf_ptr);

}

void parse_g_command( Command * c, char **buf_ptr, char *line ) {
#ifdef DEBUG_OUTPUT
  Serial.println( F("// G command, reading numeric code..") );
#endif
  c->cmd_class = 'G';
  // Parse out command ID
  switch ( get_int( buf_ptr ) ) {
    case 0:
      c->cmd_id = 0;
      break;
    case 1:
      c->cmd_id = 1;
      break;
    case 2:
      c->cmd_id = 2;
      break;
    case 3:
      c->cmd_id = 3;
      break;
    case 4:
      c->cmd_id = 4; // G4: Dwell
      break;
    case 5: // Raster Right
      c->cmd_id = 5;
      break;
    case 6: //Raster Left
      c->cmd_id = 6;
      break;
    case 7: // Raster up
      c->cmd_id = 7;
      break;
    case 8: // Raster down
      c->cmd_id = 8;
      break;
    case 20:
      c->cmd_id = 20;
      break;
    case 21:
      c->cmd_id = 21;
      break;
    case 28:
      c->cmd_id = 28;
      return; // Early return, G28 ignores parameters
    case 30:
      c->cmd_id = 30;
      return; // Early return, G30 ignores parameters
    case 90:
      c->cmd_id = 90;// G90 - Absolute positioning
      break;
    case 91:
      c->cmd_id = 91; // G91 - Relative positioning
      break;
    case 92:
      c->cmd_id = 92; // G92 - Set home Position
      break;
    case 93:
      c->cmd_id = 93; // G93 - Set Table Position
      break;

    default:
      Serial.print( F("// warning: unknown command " ) );
      Serial.print(c->c) ; Serial.print(" ");
      Serial.println( line );
      Serial.flush();
      return; // Early return, unidentified command
  }
  // Parse out parameters
  parseOutParameters(c, buf_ptr);
  return;
}



void parseOutParameters( Command * c, char **buf_ptr) {
  // Parse out parameters
  while ( **buf_ptr ) {
    skip_whitespace( buf_ptr );
    switch ( toupper(**buf_ptr) ) {
      case 'X':
        (*buf_ptr)++;
        c->x =  (get_float( buf_ptr ) * 100.0);

        break;
      case 'Y':
        (*buf_ptr)++;
        c->y =  (get_float( buf_ptr ) * 100.0);

        break;
#ifdef Z_DRIVER_ENABLED
      case 'Z':
        (*buf_ptr)++;
        c->z =  (get_float( buf_ptr ) * 100.0);

        break;
#endif
#ifdef E_DRIVER_ENABLED
      case 'E':
        (*buf_ptr)++;
        c->e = get_int( buf_ptr );
        break;
#endif
      case 'F':
        (*buf_ptr)++;
        c->f = get_int( buf_ptr );
        modal_feedrate = c->f;
        break;
#ifdef SUPPORTARCMOVES
      case 'I':
        (*buf_ptr)++;
        c->i = (get_float( buf_ptr ) * 100.0);
        break;
      case 'J':
        (*buf_ptr)++;
        c->j = (get_float( buf_ptr ) * 100.0);
        break;
#endif
      case 'S':
        (*buf_ptr)++;
        c->s = get_int( buf_ptr );
        break;
      case 'P':
        (*buf_ptr)++;
        c->p = get_int( buf_ptr );
        break;

      default:
        (*buf_ptr)++;
        break;
    }
  }
}


void print_command( HardwareSerial s, Command * c ) {
  s.print( "// " );
  s.print( c->cmd_class ); s.print( c->cmd_id );
  s.print( " X" ); s.print( c->x );
  s.print( " Y" ); s.print( c->y );
#ifdef Z_DRIVER_ENABLED
  s.print( " Z" ); s.print( c->z );
#endif
#ifdef E_DRIVER_ENABLED
  s.print( " E" ); s.print( c->e );
#endif
#ifdef SUPPORTARCMOVES
  s.print( " I" ); s.print( c->i );
  s.print( " J" ); s.print( c->j );
#endif
  s.print( " F" ); s.println( c->f );
  s.flush();
}

uint8_t Command::execute() {
#ifdef DEBUG_EXECUTE
  Serial.print("//E:"); Serial.println(counter);
  Serial.print(F("//Executing: "));
  Serial.print(this->cmd_class);
  Serial.print(F(", "));
  Serial.print(this->cmd_id);
  if (this->cmd_class == 'G' ) {
    Serial.print(F(",x: "));
    Serial.print(this->x);
    Serial.print(F(",y: "));
    Serial.print(this->y);
    Serial.print(F(",e: "));
    Serial.print(this->e);
    Serial.print(F(",f: "));
    Serial.print(this->f);
    Serial.print(F(" ,modal_feedrate: "));
    Serial.print(modal_feedrate);
  }
  Serial.println();
#endif
  uint8_t prevabsmode;
  switch ( this->cmd_class ) {
    case 'M':
      switch ( this->cmd_id ) {
        case 0:
          power_setpoint = 0;
          power_control();
          disable_motors();
          while (1);
          break;
#if OPERATION_LED_PIN > 0
        case 1:
          digitalWrite(OPERATION_LED_PIN, HIGH);
          break;
        case 2:
          digitalWrite(OPERATION_LED_PIN, LOW);
          break;
#endif
        case 3:
          power_setpoint = this->s;
          break;
        case 5:
          power_setpoint = 0;
          break;
        case 17:
          enable_motors();
          break;
        case 18:
          disable_motors();
          break;
        case 24:
          break;
        case 25:
          break;
        case 42:
          if (this->p > 0) {
            pinMode(this->p, OUTPUT);
            analogWrite(this->p, constrain(this->s, 0, 255));
          }
          break;
        case 84:
          disable_motors();
          break;
        case 106:
          power_setpoint = this->s;
          break;
        case 107:
          power_setpoint = 0;
          break;
        case 110:
          break;
        case 112:
          break;
        case 114:
          report_position();
          break;
      }
      break;
    case 'G':
      switch ( this->cmd_id ) {
        case 0:
        case 1:
        case 2:
        case 3:
          if (absolute_mode == 1) {
            this->x += homeposition[0];
            this->y += homeposition[1];
#ifdef Z_DRIVER_ENABLED
            this->z += homeposition[2];
#endif
          } else {
            this->x += realposition[0];
            this->y += realposition[1];
#ifdef Z_DRIVER_ENABLED
            this->z += realposition[2];
#endif
          }
          if (this->x > maxsize[0]) this->x = maxsize[0];
          if (this->y > maxsize[1]) this->y = maxsize[1];
#ifdef Z_DRIVER_ENABLED
          if (this->z > maxsize[2]) this->z = maxsize[2];
#endif
          /*
                    Serial.print(F("//MAX( "));
                    Serial.print(maxsize[0]) ; Serial.print(F(","));
                    Serial.print(maxsize[1]) ; Serial.print(F(","));
                    Serial.print(maxsize[2]) ; Serial.println(F(")"));
                    */
      }
      switch ( this->cmd_id ) {
        case 0:
          if ( this->f > 0 ) {
            power_setpoint = 0;
#ifdef Z_DRIVER_ENABLED
            G0_move( this->x, this->y, this->z, this->f );
#else
            G0_move( this->x, this->y, this->f );
#endif
            command_running = true;

          }
          break;
        case 1:
          if ( this->f > 0 ) {
            #ifdef E_DRIVER_ENABLED
            #else
            power_setpoint = this->s;
            #endif
#ifdef Z_DRIVER_ENABLED
            linear_move(  this->x, this->y, this->z, this->f );
#else
            linear_move(  this->x, this->y, this->f );
#endif
            command_running = true;
          }
          break;
#ifdef SUPPORTARCMOVES
        case 2:
        case 3:
          if ( this->f > 0 ) {
            #ifdef E_DRIVER_ENABLED
            #else
            power_setpoint = this->s;
            #endif
            arc_move(  this->x,  this->y, this->f, this->i, this->j, (this->cmd_id == 2 ? MS_CW : MS_CCW) );
            command_running = true;
          }
          break;
#endif
        case 4:
          //dwell here
          command_running = true;
          dwell(this->s);
          break;
        case 28: // Work Home
          power_setpoint = 0;
#ifdef Z_DRIVER_ENABLED
          G0_move( homeposition[0], homeposition[1], homeposition[2], HOMESPEED );
#else
          G0_move( homeposition[0], homeposition[1], HOMESPEED );
#endif
          command_running = true;
          break;
        case 30: // Machine Home
          command_running = true;
          home();
          break;
        case 90:
          absolute_mode = 1;
          break;
        case 91:
          prevabsmode = absolute_mode;
#ifdef Z_DRIVER_ENABLED
          if (this->x == 0 && this->y == 0  && this->z == 0) {
#else
          if (this->x == 0 && this->y == 0  ) {
#endif
            absolute_mode = 0;
          } else {
#ifdef Z_DRIVER_ENABLED
            linear_move(  constrain(realposition[0] + this->x, 0 , maxsize[0]),  constrain(realposition[1] + this->y, 0, maxsize[1]),  constrain(realposition[2] + this->z, 0, maxsize[2]), this->f );
#else
            linear_move( constrain(realposition[0] + this->x, 0 , maxsize[0]), constrain(realposition[1] + this->y, 0 , maxsize[1]), this->f );
#endif
            absolute_mode = prevabsmode;
          }
          break;
        case 92:
          homeposition[0] = realposition[0];
          homeposition[1] = realposition[1];
#ifdef Z_DRIVER_ENABLED
          homeposition[2] = realposition[2];

#endif
          break;
        case 93:
          homeposition[0] = realposition[0] = this->x;
          homeposition[1] = realposition[1] = this->y;
#ifdef Z_DRIVER_ENABLED
          homeposition[2] = realposition[2] = this->z;

#endif
          break;
      }
#ifdef E_DRIVER_ENABLED      
    case 'E':
      power_setpoint = this->e;
    // Intentionally omit break;, so that G commands also update modal feedrate
#endif
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

void report_position() {
  Serial.print(F("ok C: X:"));
  Serial.print(((float)realposition[0] - (float)homeposition[0]) / 100.0);
  Serial.print(F(" Y:"));
  Serial.print(((float)realposition[1] - (float)homeposition[1]) / 100.0);
#ifdef Z_DRIVER_ENABLED
  Serial.print(F(" Z:"));
  Serial.print(((float)realposition[2] - (float)homeposition[2])  / 100.0);
#endif
  Serial.println();
}
