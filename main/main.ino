#include "pinout.h"

/**
 * @file main.ino
 */

void setup() {
    Serial.begin(115200);
    buzzerinit();
    taps_init();
    buttons_init();
    keypad_init();
    lcd_init();
    welcome_screen_animation();
    delay(500); 
    
}
void loop() {
    //tap management
    check_pause_buttons();   // Check if any running taps have their button pressed to pause them
    check_resume_buttons();  // Check if any paused taps have their button pressed to resume them
    check_pause_timeouts();  // Check if any paused taps have exceeded the pause timeout to automatically close them
    clear_invalid_tap_presses(); // Clear button pressed flags for any taps that are not in a valid state to be paused or resumed
    check_targetpulses_to_autoclose(); // Check if any running taps have reached their target pulses to automatically close them
    
}
