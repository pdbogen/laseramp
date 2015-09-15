#include "commandqueue.h"
#include "motion.h"
#include "globals.h"
#include "pins.h"
#include "command.h"
#include "const.h"

void initialize_pins();

ISR(TIMER5_COMPA_vect) {
  ms_since_step[0]++;
  ms_since_step[1]++;
  #ifdef DEBUG_OUTPUT
  debug_counter++;
  if( debug_counter == 10000 ) {
    debug_counter = 0;
    Serial.print( "// IN:" ); Serial.print( input_ready ); Serial.print( " " );
    Serial.print( "MS:" ); Serial.print( motion_state ); Serial.print( " " );
    Serial.print( "CR:" ); Serial.print( command_running ); Serial.print( " " );
    Serial.print( "CQ:" ); Serial.println( CommandQueue::Instance()->count );
  }
  #endif
}

void setup() {
  initialize_pins();

  TCCR5A = 0;
  TCCR5B = _BV(WGM52) | _BV(CS50); // 16MHz, OVF @ 16000 -> 1kHz
  TIMSK5 = _BV(OCIE5A);
  OCR5A  = 16000 / RATE_FACTOR;

  Serial.begin( 57600 );

  while( Serial.available() ) {
    Serial.read();
  }
  home();
  while( motion_state != MS_IDLE ) {
    motion_control( Serial );
  }
  Serial.println( F("start") );
  Serial.flush();
  val = 0;
  buf_idx = 0;
}

void loop() {
  CommandQueue * cq = CommandQueue::Instance();
  // input_ready fires when we have room in the queue
  if( input_ready && cq->count < 10 ) {
    Command * c = new Command( buffer );
    if( c->cmd_class == '?' ) {
      Serial.print( "// Refusing to enqueue invalid command " ); print_command( Serial, c );
      delete c;
      c = NULL;
    } else {
      #ifdef DEBUG_QUEUE
        Serial.print( "// Enqueueing " ); print_command( Serial, c );
      #endif
      cq->enqueue( c );
      #ifdef DEBUG_QUEUE
        Serial.print( "// Queue Size: " ); Serial.println( cq->count );
      #endif
    }
    Serial.println( F("ok") );
    input_ready = false;
    buf_idx = 0; buffer[0] = '\0';
  }
  if( !command_running && cq->count > 0 ) {
    Command * c = cq->dequeue();
    if( c == NULL ) {
      Serial.println( "// Warning: Thought I could dequeue a command, but nothing was there." );
    } else {
      #ifdef DEBUG_QUEUE
        Serial.print( "// Dequeuing " ); print_command( Serial, c );
      #endif
      c->execute();
      c = NULL;
    }
  }
  motion_control( Serial );
}

void serialEvent() {
  // Input has not been consumed yet for some reason
  // (Only read one line at a time)
  if( input_ready ) {
    return;
  }

  while( Serial.available() ) {
    char c = Serial.peek();
    if( c == '\n' ) {
      input_ready = true;
      buffer[buf_idx++] = '\0';
      Serial.read();
      #ifdef DEBUG_SERIAL
        Serial.print( "// Received line: " ); Serial.println( buffer );
      #endif
      return;
    }
    if( buf_idx + 2 <= BUFFER_SIZE ) {
      buffer[buf_idx++] = c;
      buffer[buf_idx] = '\0';
      Serial.read();
    } else {
      Serial.print( "// Error: Line too long, buffer is only " );
      Serial.print( BUFFER_SIZE );
      Serial.println( "bytes; discarding line." );
      buf_idx = 0; buffer[ 0 ] = '\0';

      // This could only discard part of a line if we eat up the entire
      // serial buffer. The other easy option is to just hang, which wouldn't
      // be optimal.
      while( Serial.available() ) {
        if( Serial.peek() == '\n' ) {
          Serial.read();
          break;
        }
        Serial.read();
      }
      return;
    }
  }
}

void initialize_pins() {

  pinMode( X_STEP_PIN, OUTPUT );
  pinMode( X_DIR_PIN, OUTPUT );
  pinMode( X_ENABLE_PIN, OUTPUT ); digitalWrite( X_ENABLE_PIN, LOW );
  pinMode( X_STOP_PIN, INPUT_PULLUP );

  pinMode( Y_STEP_PIN, OUTPUT );
  pinMode( Y_DIR_PIN, OUTPUT );
  pinMode( Y_ENABLE_PIN, OUTPUT ); digitalWrite( Y_ENABLE_PIN, LOW );
  pinMode( Y_STOP_PIN, INPUT_PULLUP );

  pinMode( LASER_PIN, OUTPUT ); // digitalWrite( LASER_PIN, HIGH ); //analogWrite( LASER_PIN, 255 );
  pinMode( LASER_PW, OUTPUT ); digitalWrite( LASER_PW, HIGH );
  pinMode( FAN_PIN, OUTPUT );
}
