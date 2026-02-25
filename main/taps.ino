/**
 * Taps.ino
 * This file contains the main logic for controlling the taps, including opening and closing them, handling
 */
#include "pinout.h"
Tap taps[NUM_OF_TAPS];
#define debounce 100

// Button state tracking
bool D1_state = HIGH;
bool D2_state = HIGH;
bool D3_state = HIGH;
bool D4_state = HIGH;

uint8_t x1 = 0, x2 = 0, x3 = 0, x4 = 0;

void buttons_init() {
  PCICR |= B00000100;  // Enable PCIE2 (PCINT16-23)
  Serial.println("Tap button interrupts enabled");
}
//pins set up
uint8_t motor_open_pins[NUM_TAPS] = {motor1_open, motor2_open, motor3_open, motor4_open};
uint8_t motor_close_pins[NUM_TAPS] = {motor1_close, motor2_close, motor3_close, motor4_close};
uint8_t led_pins[NUM_TAPS] = {tap_1_led, tap_2_led, tap_3_led, tap_4_led};
uint8_t flowmeter_pins[NUM_TAPS] = {flowmeter_1, flowmeter_2, flowmeter_3, flowmeter_4};
uint8_t button_pins[NUM_TAPS] = {A8, A9, A10, A11};

void taps_init() {
    for (int i = 0; i < NUM_OF_TAPS; i++) {
        taps[i].motor_open_pin = motor_open_pins[i];
        taps[i].motor_close_pin = motor_close_pins[i];
        taps[i].led_pin = led_pins[i];
        taps[i].flowmeter_pin = flowmeter_pins[i];
        taps[i].button_pin = button_pins[i];

        // Initialize state flags and counters
        taps[i].pending_open = false;
        taps[i].running = false;
        taps[i].paused = false;
        taps[i].button_pressed = false;
        taps[i].pulse_count = 0;
        taps[i].target_pulses = 0;
        taps[i].pause_start_time = 0;

        // Set pin modes
        pinMode(taps[i].motor_open_pin, OUTPUT);
        pinMode(taps[i].motor_close_pin, OUTPUT);
        pinMode(taps[i].led_pin, OUTPUT);
        pinMode(taps[i].flowmeter_pin, INPUT_PULLUP);
        pinMode(taps[i].button_pin, INPUT_PULLUP);

        //close all taps at startup
        digitalWrite(taps[i].motor_open_pin, LOW);
        digitalWrite(taps[i].motor_close_pin, HIGH); // Assuming HIGH closes the tap        
        digitalWrite(taps[i].led_pin, LOW); // Turn off LED at startup

    }
}
/**
 * @function to initialize the flowmeter ISRs for each tap
 */


/**
 * @function to open a tap
 * @param tap_index the index of the tap to open
 */

void tap_open(uint8_t tap_index) {
    if (tap_index < 0 || tap_index >= NUM_OF_TAPS) return; // Invalid index
    
    if (!(taps[tap_index].running) && !(taps[tap_index].paused)) { // Only open if not already running and not paused
        taps[tap_index].running = true;
        taps[tap_index].pending_open = false;
        digitalWrite(taps[tap_index].motor_close_pin, LOW); // Ensure close pin is LOW
        digitalWrite(taps[tap_index].motor_open_pin, HIGH); // Activate open pin
        digitalWrite(taps[tap_index].led_pin, HIGH); // Turn on LED to indicate tap is opening
    }
    //attach flowmeter interrupt here to start counting pulses for flow measurement
    attachInterrupt(digitalPinToInterrupt(taps[tap_index].flowmeter_pin), flowmeter_isrs[tap_index], FALLING);
}
/**
 * @function to close a tap
 * @param tap_index the index of the tap to close
 */
void tap_close(uint8_t tap_index) {
    if (tap_index < 0 || tap_index >= NUM_OF_TAPS) return; // Invalid index 
    //close valves
    digitalWrite(taps[tap_index].motor_open_pin, LOW); // Ensure open pin is LOW
    digitalWrite(taps[tap_index].motor_close_pin, HIGH); // Activate close pin
    digitalWrite(taps[tap_index].led_pin, LOW); // Turn off LED to indicate tap is closed
    //reset state flags and counters
    taps[tap_index].running = false;
    taps[tap_index].paused = false;
    taps[tap_index].pulse_count = 0;
    taps[tap_index].target_pulses = 0;
    taps[tap_index].pause_start_time = 0;

    //detach flowmeter interrupt to stop counting pulses
    detachInterrupt(digitalPinToInterrupt(taps[tap_index].flowmeter_pin));
}  

/**
 * @function to pause a tap
 * @param tap_index the index of the tap to pause               
 */
void tap_pause(uint8_t tap_index) {
    if (tap_index < 0 || tap_index >= NUM_OF_TAPS) return; // Invalid index 
    if (taps[tap_index].running && !taps[tap_index].paused) { // Only pause if currently running and not already paused
        taps[tap_index].paused = true;
        taps[tap_index].button_pressed = false; // Reset button pressed flag
        digitalWrite(taps[tap_index].motor_open_pin, LOW); // Deactivate open pin to stop flow
        digitalWrite(taps[tap_index].motor_close_pin, LOW); // Ensure close pin is LOW
        taps[tap_index].pause_start_time = millis(); // Record the time when the tap was paused
    }

    //debugging
    Serial.print("========== PAUSE TAP ");
    Serial.print(tap_index);
    Serial.println(" ==========");
    Serial.print("  Current counter: ");
    Serial.println(taps[tap_index].pulse_count);
    Serial.print("  Target pulses: ");
    Serial.println(taps[tap_index].target_pulses);
}

/**
 * @function to resume a paused tap
 * @param tap_index the index of the tap to resume      
 */
void tap_resume(uint8_t tap_index) {
    if (tap_index < 0 || tap_index >= NUM_OF_TAPS) return; // Invalid index 
    if (taps[tap_index].paused) { // Only resume if currently paused
        taps[tap_index].paused = false;
        taps[tap_index].button_pressed = false; // Reset button pressed flag
        taps[tap_index].pause_start_time = 0; // Reset pause start time
        digitalWrite(taps[tap_index].motor_open_pin, HIGH); // Reactivate open pin to resume flow
        digitalWrite(taps[tap_index].motor_close_pin, LOW); // Ensure close pin is LOW
    }


    //debugging
    Serial.print("========== RESUME TAP ");
    Serial.print(tap_index);
    Serial.println(" ==========");
    Serial.print("  Current counter: ");
    Serial.println(taps[tap_index].pulse_count);
    Serial.print("  Target pulses: ");
    Serial.println(taps[tap_index].target_pulses);
}
/**
 * @function to handle button press for a running tap (to pause it)
 * @called from main*/
void check_pause_buttons() {
    for (uint8_t i = 0; i < NUM_OF_TAPS; i++) {
        if(taps[i].running && !taps[i].paused && taps[i].button_pressed) {
            tap_pause(i);
        }
    }
}
/**
 * @function to handle button press for a paused tap (to resume it)
 * @called from main*/
void check_resume_buttons() {
    for (uint8_t i = 0; i < NUM_OF_TAPS; i++) {
        if(taps[i].running && taps[i].paused && taps[i].button_pressed) {
            tap_resume(i);
        }
    }
} 
/**
 * @function to handle pause timeout for a tap (to automatically close it if paused for too long)
 */
void check_pause_timeouts() {
    unsigned long current_time = millis();
    for (uint8_t i = 0; i < NUM_OF_TAPS; i++) {
        if(taps[i].running && taps[i].paused) {
            if (current_time - taps[i].pause_start_time >= PAUSE_TIMEOUT) {
                //close tap and reset flags and counters
                tap_close(i);
                Serial.print("Tap ");
                Serial.print(i);
                Serial.println(" automatically closed due to pause timeout.");
            }
        }
    }
}
/**
 * @function to handle autoclose when target pulses are reached
 * @called from flowmeter ISRs to check if the tap should be closed after counting pulses
 * */
void check_targetpulses_to_autoclose(){
    for (uint8_t i = 0; i < NUM_OF_TAPS; i++) {
        if(taps[i].running && !taps[i].paused) {
            if (taps[i].target_pulses > 0 && taps[i].pulse_count >= taps[i].target_pulses) {
                tap_close(i);
                Serial.print("Tap ");
                Serial.print(i);
                Serial.println(" automatically closed after reaching target pulses.");
            }
        }
    }
}
/**
 * @function to clear to clear invalid tap presses (e.g. if a button was pressed but the tap is not running or already paused)
 * @called from main to reset button pressed flags for any taps that are not in a valid state to be paused or resumed       
 */
void clear_invalid_tap_presses() {
    for (uint8_t i = 0; i < NUM_OF_TAPS; i++) {
        if ((taps[i].button_pressed) && 
            (!taps[i].running && !taps[i].paused)) {
            taps[i].button_pressed = false; // Reset button pressed flag for invalid presses
        }
    }
}

// Pin change interrupt for tap buttons
ISR(PCINT2_vect) {
  
  // Check tap 0 (A8)
  if ((digitalRead(A8) && D1_state) == 0) {
    while ((digitalRead(A8) && D1_state) == 0) {
      x1++;
      if (x1 > debounce && !taps[0].button_pressed) {
        Serial.println("ISR: Tap 0 button pressed");
        taps[0].button_pressed = true;
        x1 = 0;
        break;
      }
    }
  }
  
  // Check tap 1 (A9)
  else if ((digitalRead(A9) && D2_state) == 0) {
    while ((digitalRead(A9) && D2_state) == 0) {
      x2++;
      if (x2 > debounce && !taps[1].button_pressed) {
        Serial.println("ISR: Tap 1 button pressed");
        taps[1].button_pressed = true;
        x2 = 0;
        break;
      }
    }
  }
  
  // Check tap 2 (A10)
  else if ((digitalRead(A10) && D3_state) == 0) {
    while ((digitalRead(A10) && D3_state) == 0) {
      x3++;
      if (x3 > debounce && !taps[2].button_pressed) {
        Serial.println("ISR: Tap 2 button pressed");
        taps[2].button_pressed = true;
        x3 = 0;
        break;
      }
    }
  }
  
  // Check tap 3 (A11)
  else if ((digitalRead(A11) && D4_state) == 0) {
    while ((digitalRead(A11) && D4_state) == 0) {
      x4++;
      if (x4 > debounce && !taps[3].button_pressed) {
        Serial.println("ISR: Tap 3 button pressed");
        taps[3].button_pressed = true;
        x4 = 0;
        break;
      }
    }
  }
}
