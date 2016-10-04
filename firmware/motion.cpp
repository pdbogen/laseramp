#include "motion.h"
#include "globals.h"
#include "const.h"
#include "pins.h"
#include "math.h"

void power_control();
void motion_control_linear();
void motion_control_arc();

uint8_t motion_state = MS_IDLE;
uint8_t motors_enabled = 1;

void disable_motors() {
  if( motors_enabled == 1 ) {
    pinMode( X_ENABLE_PIN, OUTPUT ); digitalWrite( X_ENABLE_PIN, HIGH );
    pinMode( Y_ENABLE_PIN, OUTPUT ); digitalWrite( Y_ENABLE_PIN, HIGH );
    motors_enabled = 0;
  } else {
    return;
  }
}

void enable_motors() {
  if( motors_enabled == 0 ) {
    pinMode( X_ENABLE_PIN, OUTPUT ); digitalWrite( X_ENABLE_PIN, LOW );
    pinMode( Y_ENABLE_PIN, OUTPUT ); digitalWrite( Y_ENABLE_PIN, LOW );
    motors_enabled = 1;
  } else {
    return;
  }
}

void motion_control( HardwareSerial s ) {
  power_control();

  if( motion_state == MS_LINEAR || motion_state == MS_SEG_CW || motion_state == MS_SEG_CCW)
    motion_control_linear();
  if( motion_state == MS_CW || motion_state == MS_CCW )
    motion_control_arc();
  if( motion_state == MS_HOMING_1 )
    motion_control_home_primary();
  if( motion_state == MS_HOMING_2 )
    motion_control_home_secondary();
  if( motion_state == MS_STOPPING ) {
    disable_motors();
    power_setpoint = 0;
    command_running = false;
    motion_state = MS_IDLE;
  }
}

void motion_control_home_primary() {
  uint8_t x = digitalRead( X_STOP_PIN );
  uint8_t y = digitalRead( Y_STOP_PIN );

  digitalWrite( X_DIR_PIN, HIGH );
  digitalWrite( Y_DIR_PIN, HIGH );

  if( x == X_STOP_ENGAGED && y == Y_STOP_ENGAGED ) {
    motion_state = MS_HOMING_2;
    return;
  }

  if( x != X_STOP_ENGAGED ) {
    if( ms_since_step[0] > HOME_SPEED_PRIMARY ) {
      enable_motors();
      digitalWrite( X_STEP_PIN, !digitalRead( X_STEP_PIN ) );
      ms_since_step[0] = 0;
    }
  }
  if( y != Y_STOP_ENGAGED ) {
    if( ms_since_step[1] > HOME_SPEED_PRIMARY ) {
      enable_motors();
      digitalWrite( Y_STEP_PIN, !digitalRead( Y_STEP_PIN ) );
      ms_since_step[1] = 0;
    }
  }
}

void motion_control_home_secondary() {
  uint8_t x = digitalRead( X_STOP_PIN );
  uint8_t y = digitalRead( Y_STOP_PIN );

  digitalWrite( X_DIR_PIN, LOW );
  digitalWrite( Y_DIR_PIN, LOW );

  if( x != X_STOP_ENGAGED && y != Y_STOP_ENGAGED ) {
    position[0] = 53 * STEPS_PER_MM;
    position[1] = 0;
    motion_state = MS_STOPPING;
    return;
  }

  if( x == X_STOP_ENGAGED ) {
    if( ms_since_step[0] > HOME_SPEED_SECONDARY ) {
      enable_motors();
      digitalWrite( X_STEP_PIN, !digitalRead( X_STEP_PIN ) );
      ms_since_step[0] = 0;
    }
  }
  if( y == Y_STOP_ENGAGED ) {
    if( ms_since_step[1] > HOME_SPEED_SECONDARY ) {
      enable_motors();
      digitalWrite( Y_STEP_PIN, !digitalRead( Y_STEP_PIN ) );
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


void motion_control_arc() {
  // We hit this when motion_state is MS_CW or MS_CCW. We recalculate
  // parameters for the next linear move and then set state to MS_SEG_CW or
  // MS_SEG_CCW as appropriate.

  uint16_t next_pos; // 1/100ths of degrees, [0,36000)
  uint8_t  mstate;

  // Update position; this is now previous position
  if( motion_state == MS_CW ) {
    // Degrees counting down
    curve_pos = (curve_pos <= 99    ? curve_pos + 35900 : curve_pos - 100);

    // Stop at precisely the end point when close.
    if( curve_pos > curve_dest && curve_pos - 100 <= curve_dest ) {
      next_pos = curve_dest;
      mstate = MS_LINEAR;
    } else {
      next_pos = (curve_pos <= 99    ? curve_pos + 35900 : curve_pos - 100);
      mstate = MS_SEG_CW;
    }
  } else {
    // Degrees counting up
    curve_pos = (curve_pos >= 35900 ? curve_pos - 35900 : curve_pos + 100);

    if( curve_pos < curve_dest && curve_pos + 100 >= curve_dest ) {
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
//    Serial.print( "           Curve: " ); Serial.print( curve_radius ); Serial.print( " around (" ); Serial.print( curve_x ); Serial.print( "," ); Serial.print( curve_y ); Serial.println( ")" );
//    Serial.print( "           X-Off: " ); Serial.println( curve_radius * cos( next_pos * M_PI / 18000 ) );
//    Serial.print( "           Y-Off: " ); Serial.println( curve_radius * sin( next_pos * M_PI / 18000 ) );
//    Serial.print( "          X-Next: " ); Serial.println( curve_x + curve_radius * cos( next_pos * M_PI / 18000 ) );
//    Serial.print( "          Y-Next: " ); Serial.println( curve_y + curve_radius * sin( next_pos * M_PI / 18000 ) );
  #endif

  linear_move_steps(
    curve_x + curve_radius * cos( next_pos * M_PI / 18000 ),
    curve_y + curve_radius * sin( next_pos * M_PI / 18000 ),
    curve_feedrate
  );
  motion_state = mstate;
}

// Motion control needs to advance the stepper motor at a given rate r (mm/min) => r * steps/mm (steps/min) => r / 60 * steps / mm (steps/sec)
// Cycle every ms, (1000Hz), track time-since-last-step per axis, when > target, reduce by time and step
// Continue this until position matches destination
void motion_control_linear() {
  if( position[0] != destination[0] ) {
    if( ms_since_step[0] >= rate[0] ) {
      enable_motors();
      ms_since_step[0] -= rate[0];
      if( digitalRead( X_STEP_PIN ) ) {
        digitalWrite( X_STEP_PIN, LOW );
        position[0] += direction[0];
        if( position[0] == MAX_X )
          destination[0] = position[0];
      } else {
        digitalWrite( X_STEP_PIN, HIGH );
      }
    }
  }

  if( position[1] != destination[1] ) {
    if( ms_since_step[1] >= rate[1] ) {
      enable_motors();
      ms_since_step[1] -= rate[1];
      if( digitalRead( Y_STEP_PIN ) ) {
        digitalWrite( Y_STEP_PIN, LOW );
        position[1] += direction[1];
        if( position[1] == MAX_Y )
          destination[1] = position[1];
      } else {
        digitalWrite( Y_STEP_PIN, HIGH );
      }
    }
  }

  if( position[0] == destination[0] && position[1] == destination[1] ) {
    if( motion_state == MS_LINEAR ) {
      motion_state = MS_STOPPING;
    } else if( motion_state == MS_SEG_CW ) {
      motion_state = MS_CW;
    } else if( motion_state == MS_SEG_CCW ) {
      motion_state = MS_CCW;
    }
  }
}


void power_control() {
  if( power_setpoint != power ) {
    power = power_setpoint;
    if( power == 0 )
      digitalWrite( LASER_PIN, LOW );
    else if( power == 255 ) {
      #ifdef DEBUG_POWER
        Serial.println( F("// lasing!") );
      #endif
      digitalWrite( LASER_PIN, HIGH );
    } else {
      #ifdef DEBUG_POWER
        Serial.println( F("// lasing!") );
      #endif
      analogWrite( LASER_PIN, power );
    }
    if( power > 0 )
      digitalWrite( FAN_PIN, HIGH );
    else
      digitalWrite( FAN_PIN, LOW );
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

// Values given in mm (mm/min for f)
void arc_move( int16_t x, int16_t y, uint16_t f, int16_t i, int16_t j, uint8_t direction ) {
  uint16_t sx = x * STEPS_PER_MM / 100,
           sy = y * STEPS_PER_MM / 100;

  // curve_x and y make a "virtual point" and can be outside the normal
  // printable bounds
  curve_x      = position[0] + i * STEPS_PER_MM / 100;
  curve_y      = position[1] + j * STEPS_PER_MM / 100;
  curve_radius = sqrt( pow( i * STEPS_PER_MM / 100, 2 ) + pow( j * STEPS_PER_MM / 100, 2 ) );
  curve_feedrate = f;

  #ifdef DEBUG_CURVE
    Serial.print( F("curve_x: ") ); Serial.print( position[0] ); Serial.println( F(" +") );
    Serial.print( F("curve_x: ") ); Serial.print( i ); Serial.println( F(" *") );
    Serial.print( F("curve_x: ") ); Serial.print( STEPS_PER_MM ); Serial.println( F(" /") );
    Serial.print( F("curve_x: ") ); Serial.print( 100 ); Serial.println( F(" =") );
    Serial.print( F("curve_x: ") ); Serial.println( curve_x );
  #endif

  if( curve_y < 0 || position[1] > static_cast<unsigned int>(curve_y) )
    curve_pos = 0;
  else
    curve_pos = 18000;

  #ifdef DEBUG_CURVE
    Serial.print( F("Start quadrant: ") ); Serial.println( curve_pos );
  #endif
  curve_pos += acos( ( curve_x - position[0] ) / curve_radius ) * 18000.0f / M_PI;

  if( curve_y < 0 || sy > static_cast<unsigned int>(curve_y) )
    curve_dest = 0;
  else
    curve_dest = 18000;

  #ifdef DEBUG_CURVE
    Serial.print( F("  End quadrant: ") ); Serial.println( curve_dest );
  #endif
  curve_dest += acos( ( curve_x - sx ) / curve_radius ) * 18000.0f / M_PI;

  if( direction == MS_CW ) {
    if( curve_dest >= 36000 )
      curve_dest -= 36000;
    if( curve_pos  <= 0 )
      curve_pos  += 36000;
  } else {
    if( curve_dest <= 0 )
      curve_dest += 36000;
    if( curve_pos >= 36000 )
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
    Serial.print( (direction==MS_CW?F("clockwise"):F("counter-clockwise")) );
    Serial.print( F(" at ") ); Serial.print( f ); Serial.print( F(" mm/min theta=(") );
    Serial.print( curve_pos ); Serial.print( F(" to ") ); Serial.print( curve_dest );
    Serial.println( F(")") );
    Serial.flush();
  #endif
}

void linear_move( int32_t x, int32_t y, uint16_t f ) {
  linear_move_steps( x * STEPS_PER_MM / 100, y * STEPS_PER_MM / 100, f );
}

// Set up motion parameters for a linear movement
void linear_move_steps( int32_t x, int32_t y, uint16_t f ) {
  // Clamp to minimums
  x = (x<0?0:x);
  y = (y<0?0:y);

  #ifdef DEBUG_OUTPUT
    Serial.print( F("// Executing move to (") );
    Serial.print( x / STEPS_PER_MM ); Serial.print( F(",") ); Serial.print( y / STEPS_PER_MM );
    Serial.print( F(") at ") ); Serial.print( f );
    Serial.println( F(" mm/min") );
  #endif

  destination[0] = x;
  destination[1] = y;

  // for now F will be steps/min
  // total travel time in minutes T is sqrt(x^2+y^2) / F
  // rate (steps/min) in X dir is X / T
  // rate (steps/min) in Y dir is Y / T
  uint32_t dx = (position[0]>destination[0]?position[0]-destination[0]:destination[0]-position[0]);
  uint32_t dy = (position[1]>destination[1]?position[1]-destination[1]:destination[1]-position[1]);
  uint16_t T = sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) / STEPS_PER_MM / f * 60 * 1000 ; // in ms
  #ifdef DEBUG_OUTPUT
    Serial.print( F("// dx: ") ); Serial.print( dx ); Serial.print( " (" ); Serial.print( dx / STEPS_PER_MM ); Serial.print( "mm)" );
    Serial.print( F(" dy: ") ); Serial.print( dy ); Serial.print( " (" ); Serial.print( dy / STEPS_PER_MM ); Serial.print( "mm)" );
    Serial.print( F(" d: " ) ); Serial.print( sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) ); Serial.print( " (" ); Serial.print( sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) / STEPS_PER_MM ); Serial.print( "mm)" );
    Serial.print( F(" ms/step: ") ); Serial.print( STEPS_PER_MM / f * 60 * 1000 );
    Serial.print( F(" T: ") ); Serial.print( T ); Serial.println( F("ms") );
  #endif

  // rate per direction in mm/min
  // ^-1 -> min/mm, /60 -> sec/mm, /1000 -> ms/mm
  rate[0] = T * RATE_FACTOR / dx / 2;
  rate[1] = T * RATE_FACTOR / dy / 2;
  #ifdef DEBUG_OUTPUT
    Serial.print( "// " );
    Serial.print( rate[0] ); Serial.print( F(" RF*ms / X step, ") );
    Serial.print( rate[1] ); Serial.println( F(" RF*ms / Y step") );
    Serial.print( "// " );
    Serial.print( T / 1000 ); Serial.print( F("s to move ") ); Serial.print( sqrt( pow( dx, 2 ) + pow( dy, 2 ) ) ); Serial.println( F(" steps") );
  #endif
  if( destination[0] > position[0] ) {
    direction[0] = 1;
    digitalWrite( X_DIR_PIN, X_POS );
  } else {
    direction[0] = -1;
    digitalWrite( X_DIR_PIN, X_NEG );
  }

  if( destination[1] > position[1] ) {
    direction[1] = 1;
    digitalWrite( Y_DIR_PIN, Y_POS );
  } else {
    direction[1] = -1;
    digitalWrite( Y_DIR_PIN, Y_NEG );
  }
  ms_since_step[0] = 0;
  ms_since_step[1] = 0;
  motion_state = MS_LINEAR;
}

void home() {
  motion_state = MS_HOMING_1;
}
