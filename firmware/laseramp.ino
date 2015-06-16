/*
Release 0.1
We ignore N commands.

Todo:
Support for N\d+ line prefix.
Set on run time the size of bed, the steps per mm
Save configuration to eeprom
Save to eeprom default values.
Finish Raster mode.
Support for negative values in X, Y, Z.
Add Homing option for HARDWARE positioning and WORK positioning

http://www.deskcnc.com/Homing.html
Raster Mode
G5 <[0-9A-F]>
each char is a 8bit long code, each command may send up to 64 bytes.cada
*/


#include "commandqueue.h"
#include "motion.h"
#include "globals.h"
#include "const.h"
#include "pins.h"
#include "command.h"
#include "fastio.h"

#if   MOTHERBOARD == 1 //RAMPS14
#include <TimerThree.h>
#endif
#if   MOTHERBOARD == 2 //GRBL
#include <TimerOne.h>
#endif



//void initialize_pins();

void ISRTIMER5COMPAvect() {
  ms_since_step[0]++;
  ms_since_step[1]++;
#ifdef Z_DRIVER_ENABLED
  ms_since_step[2]++;
#endif

 #ifdef EASYCNCCONTROLLER
   debug_counter0++;
  if ( debug_counter0 == 1000 ) {
    Serial.print (F("#"));
    Serial.print((motion_state>0?EASYCNC_BUSY:EASYCNC_IDLE));
    Serial.print(F(":"));
    Serial.println(BUFFER_SIZE - buf_idx);
     debug_counter0 = 0;
  }
   debug_counter1++;
  if ( debug_counter1 == 1000 ) {
 
    Serial.print (F("$"));
    Serial.print(realposition[0]);
    Serial.print(F(":"));
    Serial.print(realposition[1]);
    Serial.print(F(":"));
    #ifdef Z_DRIVER_ENABLED
        Serial.print(realposition[2]);
    #else
    Serial.print(0.0);
    #endif
    Serial.print(F(":"));
    Serial.print(modal_feedrate);
    Serial.print(F(":"));
    Serial.print((motion_state>0?EASYCNC_BUSY:EASYCNC_IDLE));
    Serial.print(F(":"));
    Serial.println(BUFFER_SIZE - buf_idx);
    debug_counter1 = 0;
}
    #endif
   /* 
    #ifdef DEBUG_OUTPUT
    Serial.print( "// IN:" ); Serial.print( input_ready ); Serial.print( " " );
    Serial.print( "MS:" ); Serial.print( motion_state ); Serial.print( " " );
    Serial.print( "CR:" ); Serial.print( command_running ); Serial.print( " " );
    Serial.print( "CQ:" ); Serial.println( CommandQueue::Instance()->count );
    #endif
  */

}

void setup() {
  initialize_pins();
#if   MOTHERBOARD == 1
  Timer3.initialize(RATE_FACTOR);
  Timer3.attachInterrupt(ISRTIMER5COMPAvect);  // attaches callback() as a timer overflow interrupt
#endif
#if   MOTHERBOARD == 2
  Timer1.initialize(RATE_FACTOR);
  Timer1.attachInterrupt(ISRTIMER5COMPAvect);  // attaches callback() as a timer overflow interrupt
#endif
  Serial.begin( BAUDRATE );

  while ( Serial.available() ) {
    Serial.read();
  }
#ifdef HOMEONSTART
  home();
  while ( motion_state != MS_IDLE ) {
    motion_control( Serial );
  }
#endif
  Serial.println( F("start") );
  Serial.flush();
  val = 0;
  buf_idx = 0;
}

void loop() {
  CommandQueue * cq = CommandQueue::Instance();
  // input_ready fires when we have room in the queue
  
  if ( input_ready) {
    if ( buffer[0] == ';' || buffer[0] == '/') { //ignore comments
      Serial.println( F("ok") );
      input_ready = false;
      buf_idx = 0; buffer[0] = '\0';
    }
  }
  // no queuing responses.
  // Add Emergency stop here.
  if ( input_ready && cq->count < QUEUESIZE ) {

    Command * c = new Command( buffer );
    if ( c->cmd_class == '?' ) {
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
    buf_idx = 0; buffer[0] = '\0';
    input_ready = false;
  }


  if ( !command_running && cq->count > 0 ) {
    Command * c = cq->dequeue();
    if ( c == NULL ) {
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
  if ( input_ready ) {
    return;
  }

  while ( Serial.available() ) {
    char c = Serial.peek();
    if ( c == '\n' ) {
      input_ready = true;
      buffer[buf_idx++] = '\0';
      Serial.read();
#ifdef DEBUG_SERIAL
      Serial.print( "// Received line: " ); Serial.println( buffer );
#endif
      return;
    }
    if ( buf_idx + 2 <= BUFFER_SIZE ) {
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
      while ( Serial.available() ) {
        if ( Serial.peek() == '\n' ) {
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

  /*
    magic I/O routines
    now you can simply SET_OUTPUT(STEP); WRITE(STEP, 1); WRITE(STEP, 0);
  */
  #if OPERATION_LED_PIN > 0
  pinMode(OPERATION_LED_PIN,OUTPUT);
  digitalWrite( OPERATION_LED_PIN, LOW);
 #endif

  SET_OUTPUT( X_STEP_PIN);
  SET_OUTPUT( X_DIR_PIN);
  SET_OUTPUT( X_ENABLE_PIN); WRITE( X_ENABLE_PIN, LOW );
  SET_OUTPUT( X_STOP_PIN);//, INPUT_PULLUP );
  // digitalWrite( X_STOP_PIN, HIGH );

  SET_OUTPUT( Y_STEP_PIN);
  SET_OUTPUT( Y_DIR_PIN);
  SET_OUTPUT( Y_ENABLE_PIN); WRITE( Y_ENABLE_PIN, LOW );
  SET_OUTPUT( Y_STOP_PIN);//, INPUT_PULLUP );
  //    digitalWrite( Y_STOP_PIN, HIGH );
#ifdef Z_DRIVER_ENABLED
  SET_OUTPUT( Z_STEP_PIN);
  SET_OUTPUT( Z_DIR_PIN);
  SET_OUTPUT( Z_ENABLE_PIN); WRITE( Z_ENABLE_PIN, LOW );
  SET_OUTPUT( Z_STOP_PIN);//, INPUT_PULLUP );
  //  digitalWrite( Z_STOP_PIN, HIGH );
#endif
#ifdef E_DRIVER_ENABLED
  SET_OUTPUT( E_STEP_PIN);
  SET_OUTPUT( E_DIR_PIN);
  SET_OUTPUT( E_ENABLE_PIN); WRITE( E_ENABLE_PIN, LOW );
#endif
  pinMode( LASER_PIN, OUTPUT); // LASER PWM TTL interface
  SET_OUTPUT( LASER_PW); WRITE( LASER_PW, HIGH ); // NPN pin when using two pins to control laser.
#ifdef FAN_PIN
  SET_OUTPUT( FAN_PIN);
#endif
}
