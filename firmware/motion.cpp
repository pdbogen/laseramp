#include <stdint.h>
#include <HardwareSerial.h>
#include "motion.h"

#include "globals.h"
#include "const.h"
#include "pins.h"
#include "math.h"
#include "fastio.h"
uint32_t dwell_timer;

void power_control();
void motion_control_linear();
void motion_control_arc();

uint8_t motion_state = MS_IDLE;
uint8_t motors_enabled = 1;

void emergency_stop() {
  disable_motors();
  digitalWrite( LASER_PIN, LOW );
  for (;;);

}

void disable_motors() {
  if ( motors_enabled == 1 ) {
    SET_OUTPUT( X_ENABLE_PIN ); WRITE( X_ENABLE_PIN, HIGH );
    SET_OUTPUT( Y_ENABLE_PIN ); WRITE( Y_ENABLE_PIN, HIGH );
#ifdef Z_DRIVER_ENABLED
    SET_OUTPUT( Z_ENABLE_PIN ); WRITE( Z_ENABLE_PIN, HIGH );
#endif
    motors_enabled = 0;
  } else {
    return;
  }
}

void enable_motors() {
  if ( motors_enabled == 0 ) {
    SET_OUTPUT( X_ENABLE_PIN ); WRITE( X_ENABLE_PIN, LOW );
    SET_OUTPUT( Y_ENABLE_PIN ); WRITE( Y_ENABLE_PIN, LOW );
#ifdef Z_DRIVER_ENABLED
    SET_OUTPUT( Z_ENABLE_PIN ); WRITE( Z_ENABLE_PIN, LOW );
#endif
    motors_enabled = 1;
  } else {
    return;
  }
}

void motion_control( HardwareSerial s ) {
  power_control();

  if ( motion_state == MS_LINEAR || motion_state == MS_SEG_CW || motion_state == MS_SEG_CCW || motion_state == MS_G0_1 || motion_state == MS_G0_2)
    motion_control_linear();
#ifdef SUPPORTARCMOVES
  if ( motion_state == MS_CW || motion_state == MS_CCW )
    motion_control_arc();
#endif
  if ( motion_state == MS_HOMING_1 )
    motion_control_home_primary();
  if ( motion_state == MS_HOMING_2 )
    motion_control_home_secondary();
  if ( motion_state == MS_DWELLING_1)
    motion_control_dwell_primary();
  if ( motion_state == MS_DWELLING_2)
    motion_control_dwell_secondary();
  if ( motion_state == MS_STOPPING ) {
    disable_motors();
    #ifdef LASERRESETSTOZERO
    power_setpoint = 0;
    #endif
    command_running = false;
    motion_state = MS_IDLE;
  }
}

#ifdef Z_DRIVER_ENABLED
void G0_move( int32_t x, int32_t y, int32_t z, uint16_t f ) {
#else
void G0_move( int32_t x, int32_t y, uint16_t f ) {
#endif

  realposition[0] = x;
  realposition[1] = y;
  x = x * XSTEPS_PER_MM / 100;
  y = y * YSTEPS_PER_MM / 100;
  // Clamp to minimums
  x = (x < 0 ? 0 : x);
  y = (y < 0 ? 0 : y);
#ifdef Z_DRIVER_ENABLED
  realposition[2] = z;
  z = z * ZSTEPS_PER_MM / 100;
  z = (z < 0 ? 0 : z);
  destination[2] = z;
#endif

#ifdef DEBUG_MOVE
  Serial.print( F("// Executing move to (") );
  Serial.print( x / XSTEPS_PER_MM ); Serial.print( F(",") ); Serial.print( y / YSTEPS_PER_MM );
  Serial.print( F(") at ") ); Serial.print( f );
  Serial.println( F(" mm/min") );
#endif

  destination[0] = x;
  destination[1] = y;

  // for now F will be steps/min
  // total travel time in minutes T is sqrt(x^2+y^2) / F
  // rate (steps/min) in X dir is X / T
  // rate (steps/min) in Y dir is Y / T
  uint32_t dx = (position[0] > destination[0] ? position[0] - destination[0] : destination[0] - position[0]);
  uint32_t dy = (position[1] > destination[1] ? position[1] - destination[1] : destination[1] - position[1]);
#ifdef Z_DRIVER_ENABLED
  uint32_t dz = (position[2] > destination[2] ? position[2] - destination[2] : destination[2] - position[2]);
#endif

  // rate per direction in mm/min
  // ^-1 -> min/mm, /60 -> sec/mm, /1000 -> ms/mm
  float T = (float)dx * XSTEPS_PER_MM / f * RATE_FACTOR;
  rate[0] = (float)T / dx / 2.0;
  T = (float)dy * YSTEPS_PER_MM / f * RATE_FACTOR;
  rate[1] = (float)T  / dy / 2.0;
#ifdef Z_DRIVER_ENABLED
  T = (float)dz * ZSTEPS_PER_MM / f * RATE_FACTOR;
  rate[2] = (float)T  / dz / 2.0;
#endif

#ifdef DEBUG_OUTPUT
  Serial.print( F("// dx: ") ); Serial.print( dx ); Serial.print( " (" ); Serial.print( dx / XSTEPS_PER_MM ); Serial.print( "mm)" );
  Serial.print( F(" dy: ") ); Serial.print( dy ); Serial.print( " (" ); Serial.print( dy / YSTEPS_PER_MM ); Serial.print( "mm)" );
  Serial.print( F(" d: " ) ); Serial.print( sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) ); Serial.print( " (" ); Serial.print( sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) / XSTEPS_PER_MM ); Serial.print( "mm)" );
  Serial.print( F(" ms/step: ") ); Serial.print( XSTEPS_PER_MM / f * 60 * 1000 );
  Serial.print( F(" T: ") ); Serial.print( T ); Serial.println( F("ms") );
#endif



#ifdef DEBUG_OUTPUT
  Serial.print( "// " );
  Serial.print( rate[0] ); Serial.print( F(" RF*ms / X step, ") );
  Serial.print( rate[1] ); Serial.println( F(" RF*ms / Y step") );
  Serial.print( "// " );
#endif
  if ( destination[0] > position[0] ) {
    direction[0] = 1;
    WRITE( X_DIR_PIN, X_POS );
  } else {
    direction[0] = -1;
    WRITE( X_DIR_PIN, X_NEG );
  }

  if ( destination[1] > position[1] ) {
    direction[1] = 1;
    WRITE( Y_DIR_PIN, Y_POS );
  } else {
    direction[1] = -1;
    WRITE( Y_DIR_PIN, Y_NEG );
  }
#ifdef Z_DRIVER_ENABLED
  if ( destination[2] > position[2] ) {
    direction[2] = 1;
    WRITE( Z_DIR_PIN, Z_POS );
  } else {
    direction[2] = -1;
    WRITE( Z_DIR_PIN, Z_NEG );
    ms_since_step[2] = 0;
  }
#endif

  ms_since_step[0] = 0;
  ms_since_step[1] = 0;
  motion_state = MS_G0_1;
}

void motion_control_home_primary() {
  uint8_t x = READ( X_STOP_PIN );
  uint8_t y = READ( Y_STOP_PIN );

  WRITE( X_DIR_PIN, X_NEG );
  WRITE( Y_DIR_PIN, Y_NEG );
#ifdef Z_DRIVER_ENABLED
  uint8_t z = READ( Z_STOP_PIN );
  WRITE( Z_DIR_PIN, Z_NEG );
  if (  z == Z_STOP_ENGAGED &&  x == X_STOP_ENGAGED && y == Y_STOP_ENGAGED ) {
#else
  if (  x == X_STOP_ENGAGED && y == Y_STOP_ENGAGED ) {
#endif
    motion_state = MS_HOMING_2;
    return;
  }

#ifdef Z_DRIVER_ENABLED
  if ( z != Z_STOP_ENGAGED ) {
    if ( ms_since_step[2] > 0 ) {
      enable_motors();
      WRITE( Z_STEP_PIN, !READ( Z_STEP_PIN ) );
      ms_since_step[2] = 0;
    }
  }
#endif
  if ( x != X_STOP_ENGAGED ) {
    if ( ms_since_step[0] > 0 ) {
      enable_motors();
      WRITE( X_STEP_PIN, !READ( X_STEP_PIN ) );
      ms_since_step[0] = 0;
    }
  }
  if ( y != Y_STOP_ENGAGED ) {
    if ( ms_since_step[1] > 0 ) {
      enable_motors();
      WRITE( Y_STEP_PIN, !READ( Y_STEP_PIN ) );
      ms_since_step[1] = 0;
    }
  }
}

void motion_control_home_secondary() {
  uint8_t x = READ( X_STOP_PIN );
  uint8_t y = READ( Y_STOP_PIN );

  WRITE( X_DIR_PIN, X_POS );
  WRITE( Y_DIR_PIN, Y_POS );
#ifdef Z_DRIVER_ENABLED
  uint8_t z = READ( Z_STOP_PIN );
  WRITE( Z_DIR_PIN, Z_POS );
  if ( z != Z_STOP_ENGAGED && x != X_STOP_ENGAGED && y != Y_STOP_ENGAGED ) {
    realposition[2] = 0;
#else
  if ( x != X_STOP_ENGAGED && y != Y_STOP_ENGAGED ) {
#endif
    realposition[0] = 0;
    realposition[1] = 0;
    position[0] = 0;
    position[1] = 0;
    homeposition[0] = 0;
    homeposition[1] = 0;
#ifdef Z_DRIVER_ENABLED
    realposition[2] = 0;
    position[2] = 0;
    homeposition[2] = 0;

#endif
    motion_state = MS_STOPPING;
    return;
  }
#ifdef Z_DRIVER_ENABLED
  if ( z == Z_STOP_ENGAGED ) {
    if ( ms_since_step[2] > HOMEDELAY) {
      enable_motors();
      WRITE( Z_STEP_PIN, !READ( Z_STEP_PIN ) );
      ms_since_step[2] = 0;
    }
  }
#endif
  if ( x == X_STOP_ENGAGED ) {
    if ( ms_since_step[0] > HOMEDELAY) {
      enable_motors();
      WRITE( X_STEP_PIN, !READ( X_STEP_PIN ) );
      ms_since_step[0] = 0;
    }
  }
  if ( y == Y_STOP_ENGAGED ) {
    if ( ms_since_step[1] > HOMEDELAY ) {
      enable_motors();
      WRITE( Y_STEP_PIN, !READ( Y_STEP_PIN ) );
      ms_since_step[1] = 0;
    }
  }
}

// This can be stateless, as it turns out.

// At any given moment, we're at x,y and heading for Dx, Dy, around a circle
// with center Cx, Cy and radius R.

// We can be in one of four quadrants:
// x<Cx     |     x>Cx
// y>Cy     |     y>Cy
// 90..180  |    0..90
//          |
// ---------|---------
//          |
// 180..270 | 270..360
// x<Cx     |     x>Cx
// y<Cy     |     y<Cy

#ifdef SUPPORTARCMOVES
void motion_control_arc() {
  // We hit this when motion_state is MS_CW or MS_CCW. We recalculate
  // parameters for the next linear move and then set state to MS_SEG_CW or
  // MS_SEG_CCW as appropriate.

  uint32_t next_pos; // 1/100ths of degrees, [0,36000)
  uint8_t  mstate;

  // Update position; this is now previous position
  if ( motion_state == MS_CW ) {
    // Degrees counting down
    curve_pos = (curve_pos <= 99    ? curve_pos + 35900 : curve_pos - 100);

    // Stop at precisely the end point when close.
    if ( curve_pos > curve_dest && curve_pos - 100 <= curve_dest ) {
      next_pos = curve_dest;
      mstate = MS_LINEAR;
    } else {
      next_pos = (curve_pos <= 99    ? curve_pos + 35900 : curve_pos - 100);
      mstate = MS_SEG_CW;
    }
  } else {
    // Degrees counting up
    curve_pos = (curve_pos >= 35900 ? curve_pos - 35900 : curve_pos + 100);

    if ( curve_pos < curve_dest && curve_pos + 100 >= curve_dest ) {
      next_pos = curve_dest;
      mstate = MS_LINEAR;
    } else {
      next_pos = (curve_pos >= 35900 ? curve_pos - 35900 : curve_pos + 100);
      mstate = MS_SEG_CCW;
    }

  }

  curve_starting = 0;

#ifdef DEBUG_CURVE
  Serial.print( F("// curve: " ) ); Serial.print( curve_pos );
  Serial.print( F(" -> " ) ); Serial.print( next_pos );
  Serial.print( F(" / " ) ); Serial.print( curve_dest );
  Serial.print( F(" starting:" ) ); Serial.println( curve_starting );
  //		Serial.print( "           Curve: " ); Serial.print( curve_radius ); Serial.print( " around (" ); Serial.print( curve_x ); Serial.print( "," ); Serial.print( curve_y ); Serial.println( ")" );
  //		Serial.print( "           X-Off: " ); Serial.println( curve_radius * cos( next_pos * M_PI / 18000 ) );
  //		Serial.print( "           Y-Off: " ); Serial.println( curve_radius * sin( next_pos * M_PI / 18000 ) );
  //		Serial.print( "          X-Next: " ); Serial.println( curve_x + curve_radius * cos( next_pos * M_PI / 18000 ) );
  //		Serial.print( "          Y-Next: " ); Serial.println( curve_y + curve_radius * sin( next_pos * M_PI / 18000 ) );
#endif

#ifdef Z_DRIVER_ENABLED
  linear_move_steps(
    curve_x + curve_radius * cos( next_pos * M_PI / 18000 ),
    curve_y + curve_radius * sin( next_pos * M_PI / 18000 ),
    realposition[2] * ZSTEPS_PER_MM / 100,
    curve_feedrate
  );
#else
  linear_move_steps(
    curve_x + curve_radius * cos( next_pos * M_PI / 18000 ),
    curve_y + curve_radius * sin( next_pos * M_PI / 18000 ),
    curve_feedrate
  );
#endif
  motion_state = mstate;
}
#endif

// Motion control needs to advance the stepper motor at a given rate r (mm/min) => r * steps/mm (steps/min) => r / 60 * steps / mm (steps/sec)
// Cycle every ms, (1000Hz), track time-since-last-step per axis, when > target, reduce by time and step
// Continue this until position matches destination
void motion_control_linear() {
#ifdef Z_DRIVER_ENABLED
  if ( position[2] != destination[2] ) {
    if ( ms_since_step[2] >= rate[2] ) {
      enable_motors();
      ms_since_step[2] -= rate[2];
      if ( READ( Z_STEP_PIN ) ) {
        WRITE( Z_STEP_PIN, LOW );
        position[2] += direction[2];
        if ( position[2] == maxsize[2] )
          destination[2] = position[2];
      } else {
        WRITE( Z_STEP_PIN, HIGH );
      }
    }
    ms_since_step[0] = ms_since_step[1] = 0;
  }
#endif
  if ( position[0] != destination[0] ) {
    if ( ms_since_step[0] >= rate[0] ) {
      enable_motors();
      ms_since_step[0] -= rate[0];
      if ( READ( X_STEP_PIN ) ) {
        WRITE( X_STEP_PIN, LOW );
        position[0] += direction[0];
        if ( position[0] == maxsize[0] )
          destination[0] = position[0];
      } else {
        WRITE( X_STEP_PIN, HIGH );
      }
    }
  }
  if (motion_state == MS_G0_1 &&  position[0] == destination[0] ) {
    motion_state = MS_G0_2;
    ms_since_step[1] = 0;
  } else {
    if (motion_state != MS_G0_1 ||  position[0] == destination[0] ) {
      if ( position[1] != destination[1] ) {
        if ( ms_since_step[1] >= rate[1] ) {
          enable_motors();
          ms_since_step[1] -= rate[1];
          if ( READ( Y_STEP_PIN ) ) {
            WRITE( Y_STEP_PIN, LOW );
            position[1] += direction[1];
            if ( position[1] == maxsize[1] )
              destination[1] = position[1];
          } else {
            WRITE( Y_STEP_PIN, HIGH );
          }
        }
      }
    }
  }
#ifdef Z_DRIVER_ENABLED
  if ( position[0] == destination[0] && position[1] == destination[1] && position[2] == destination[2] ) {
#else
  if ( position[0] == destination[0] && position[1] == destination[1] ) {
#endif
    if ( motion_state == MS_LINEAR || motion_state == MS_G0_2 ) {
      motion_state = MS_STOPPING;
    } else if ( motion_state == MS_SEG_CW ) {
      motion_state = MS_CW;
    } else if ( motion_state == MS_SEG_CCW ) {
      motion_state = MS_CCW;
    }
  }
}




// We can be in one of four quadrants:
// x<Cx     |     x>Cx
// y>Cy     |     y>Cy
// 90..180  |    0..90
//          |
// ---------|---------
//          |
// 180..270 | 270..360
// x<Cx     |     x>Cx
// y<Cy     |     y<Cy

#ifdef SUPPORTARCMOVES
// Values given in mm (mm/min for f)
void arc_move( int32_t x, int32_t y, uint16_t f, int32_t i, int32_t j, uint8_t direction ) {
  realposition[0] = x;
  realposition[1] = y;
  uint32_t sx = x * XSTEPS_PER_MM / 100,
           sy = y * YSTEPS_PER_MM / 100;

  // curve_x and y make a "virtual point" and can be outside the normal
  // printable bounds
  curve_x      = position[0] + i * XSTEPS_PER_MM / 100;
  curve_y      = position[1] + j * YSTEPS_PER_MM / 100;
  curve_radius = sqrt( pow( i * XSTEPS_PER_MM / 100, 2 ) + pow( j * YSTEPS_PER_MM / 100, 2 ) );
  curve_feedrate = f;

#ifdef DEBUG_CURVE
  Serial.print( F("curve_x: ") ); Serial.print( position[0] ); Serial.println( F(" +") );
  Serial.print( F("curve_x: ") ); Serial.print( i ); Serial.println( F(" *") );
  Serial.print( F("curve_x: ") ); Serial.print( XSTEPS_PER_MM ); Serial.println( F(" /") );
  Serial.print( F("curve_x: ") ); Serial.print( 100 ); Serial.println( F(" =") );
  Serial.print( F("curve_x: ") ); Serial.println( curve_x );
#endif

  if ( curve_y < 0 ||   static_cast<unsigned int32_t>(curve_y) < position[1] )
    curve_pos = 0;
  else
    curve_pos = 18000;

#ifdef DEBUG_CURVE
  Serial.print( F("Start quadrant: ") ); Serial.println( curve_pos );
#endif
  curve_pos += acos( ( curve_x - position[0] ) / curve_radius ) * 18000.0f / M_PI;

  if ( curve_y < 0 ||  static_cast<unsigned int32_t>(curve_y) < sy )
    curve_dest = 0;
  else
    curve_dest = 18000;

#ifdef DEBUG_CURVE
  Serial.print( F("  End quadrant: ") ); Serial.println( curve_dest );
#endif
  curve_dest += acos( ( curve_x - sx ) / curve_radius ) * 18000.0f / M_PI;

  if ( direction == MS_CW ) {
    if ( curve_dest >= 36000 )
      curve_dest -= 36000;
    if ( curve_pos  <= 0 )
      curve_pos  += 36000;
  } else {
    if ( curve_dest <= 0 )
      curve_dest += 36000;
    if ( curve_pos >= 36000 )
      curve_pos -= 36000;
  }

  motion_state = direction;
  curve_starting = 1;
#ifdef DEBUG_CURVE
  Serial.print( F("Executing move from (") );
  Serial.print( position[0] ); Serial.print( F(",") ); Serial.print( position[1] );
  Serial.print( F(") to (") );
  Serial.print( sx ); Serial.print( F(",") ); Serial.print( sy );
  Serial.print( F(") along curve centered at (") );
  Serial.print( curve_x ); Serial.print( F(",") ); Serial.print( curve_y );
  Serial.flush();
  Serial.print( F(") ") );
  Serial.print( (direction == MS_CW ? F("clockwise") : F("counter-clockwise")) );
  Serial.print( F(" at ") ); Serial.print( f ); Serial.print( F(" mm/min theta=(") );
  Serial.print( curve_pos ); Serial.print( F(" to ") ); Serial.print( curve_dest );
  Serial.println( F(")") );
  Serial.flush();
#endif
}
#endif

#ifdef Z_DRIVER_ENABLED
void linear_move( int32_t x, int32_t y, int32_t z, uint16_t f ) {
  realposition[0] = x;
  realposition[1] = y;
  realposition[2] = z;
  linear_move_steps( x * XSTEPS_PER_MM / 100, y * YSTEPS_PER_MM / 100, z * ZSTEPS_PER_MM / 100, f );
}
#else
void linear_move( int32_t x, int32_t y, uint16_t f ) {
  realposition[0] = x;
  realposition[1] = y;
  linear_move_steps( x * XSTEPS_PER_MM / 100, y * YSTEPS_PER_MM / 100, f );
}
#endif


// Set up motion parameters for a linear movement
#ifdef Z_DRIVER_ENABLED
void linear_move_steps( int32_t x, int32_t y, int32_t z, uint16_t f ) {
  z = (z < 0 ? 0 : z);
  destination[2] = z;
  uint32_t dz = (position[2] > destination[2] ? position[2] - destination[2] : destination[2] - position[2]); // this is the same as abs(position[2] - destination[2]);
#else
void linear_move_steps( int32_t x, int32_t y, uint16_t f ) {
#endif
  // Clamp to minimums
  x = (x < 0 ? 0 : x);
  y = (y < 0 ? 0 : y);
  destination[0] = x;
  destination[1] = y;
#ifdef DEBUG_MOVE
  Serial.print( F("// Executing move to (") );
  Serial.print( x / XSTEPS_PER_MM ); Serial.print( F(",") ); Serial.print( y / YSTEPS_PER_MM );
#ifdef Z_DRIVER_ENABLED
  Serial.print( F(",") ); Serial.print( z / ZSTEPS_PER_MM );
#endif
  Serial.print( F(") at ") ); Serial.print( f );
  Serial.println( F(" mm/min") );
#endif
  // for now F will be steps/min
  // total travel time in minutes T is sqrt(x^2+y^2) / F
  // rate (steps/min) in X dir is X / T
  // rate (steps/min) in Y dir is Y / T
  uint32_t dx = (position[0] > destination[0] ? position[0] - destination[0] : destination[0] - position[0]); //this is the same as abs( a - b)
  uint32_t dy = (position[1] > destination[1] ? position[1] - destination[1] : destination[1] - position[1]);

  //uint16_t T = sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) / STEPS_PER_MM / f * 60 * 1000 ; // in ms
#ifdef Z_DRIVER_ENABLED
  float T = (float)max(dz * ZSTEPS_PER_MM, max(dx * XSTEPS_PER_MM, dy * YSTEPS_PER_MM)) / f * RATE_FACTOR;
#else
  float T = (float)max(dx * XSTEPS_PER_MM, dy * YSTEPS_PER_MM) / f * RATE_FACTOR;
#endif
#ifdef DEBUG_OUTPUT
  Serial.print( F("// dx: ") ); Serial.print( dx ); Serial.print( " (" ); Serial.print( dx / XSTEPS_PER_MM ); Serial.print( "mm)" );
  Serial.print( F(" dy: ") ); Serial.print( dy ); Serial.print( " (" ); Serial.print( dy / YSTEPS_PER_MM ); Serial.print( "mm)" );
  Serial.print( F(" d: " ) ); Serial.print( sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) ); Serial.print( " (" ); Serial.print( sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) / STEPS_PER_MM ); Serial.print( "mm)" );
  Serial.print( F(" ms/step: ") ); Serial.print( STEPS_PER_MM / f * 60 * 1000 );
  Serial.print( F(" T: ") ); Serial.print( T ); Serial.println( F("ms") );
#endif

  // rate per direction in mm/min
  // ^-1 -> min/mm, /60 -> sec/mm, /1000 -> ms/mm
#ifdef Z_DRIVER_ENABLED
  rate[2] = (float)T  / dz / 2.0;
#endif
  rate[0] = (float)T  / dx / 2.0;
  rate[1] = (float)T  / dy / 2.0;
#ifdef DEBUG_OUTPUT
  Serial.print( "// " );
  Serial.print( rate[0] ); Serial.print( F(" RF*ms / X step, ") );
  Serial.print( rate[1] ); Serial.println( F(" RF*ms / Y step") );
  Serial.print( "// " );
  Serial.print( T / 1000 ); Serial.print( F("s to move ") ); Serial.print( sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) ); Serial.println( F(" steps") );
#endif
#ifdef Z_DRIVER_ENABLED
  if ( destination[2] > position[2] ) {
    direction[2] = 1;
    WRITE( Z_DIR_PIN, Z_POS );
  } else {
    direction[2] = -1;
    WRITE( Z_DIR_PIN, Z_NEG );
  }
#endif
  if ( destination[0] > position[0] ) {
    direction[0] = 1;
    WRITE( X_DIR_PIN, X_POS );
  } else {
    direction[0] = -1;
    WRITE( X_DIR_PIN, X_NEG );
  }

  if ( destination[1] > position[1] ) {
    direction[1] = 1;
    WRITE( Y_DIR_PIN, Y_POS );
  } else {
    direction[1] = -1;
    WRITE( Y_DIR_PIN, Y_NEG );
  }
  ms_since_step[0] = 0;
  ms_since_step[1] = 0;
#ifdef Z_DRIVER_ENABLED
  ms_since_step[2] = 0;
#endif
  motion_state = MS_LINEAR;
}

void home() {
  motion_state = MS_HOMING_1;
}

void dwell(uint8_t t ) {
  dwell_timer = (long)t * 1000l  ;
  ms_since_step[0] = 0;
  motion_state = MS_DWELLING_1;
}


void motion_control_dwell_primary() {
  if (dwell_timer < ms_since_step[0]) {
    motion_state = MS_DWELLING_2;
  }
}

void motion_control_dwell_secondary() {
  motion_state = MS_IDLE;
  command_running = false;
}


void power_control() {
  if ( power_setpoint != power ) {
    power = power_setpoint;
    if ( power == 0 )
      digitalWrite(LASER_PIN, LOW );
    else if ( power == 255 ) {
#ifdef DEBUG_POWER
      Serial.println( F("// lasing!") );
#endif
      digitalWrite(LASER_PIN, HIGH );
    } else {
#ifdef DEBUG_POWER
      Serial.println( F("// lasing!") );
#endif
      analogWrite( LASER_PIN, power );
    }
#ifdef FAN_PIN
    if ( power > 0 )
      WRITE( FAN_PIN, HIGH );
    else
      WRITE( FAN_PIN, LOW );
#endif
  }
}
