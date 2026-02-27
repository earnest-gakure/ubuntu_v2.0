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
    rfid_init();
    welcome_screen_animation();
    delay(500); 
    loading_screen();
    state_machine_init();
    timer_init();

    gsm_init();
    build_mqtt_topics();
    homescreen();
    
}
void loop() {

    //gsm updating
    gsm_update(); // Drives the GSM connection state machine (non-blocking)
    //tap management
    check_pause_buttons();   // Check if any running taps have their button pressed to pause them
    check_resume_buttons();  // Check if any paused taps have their button pressed to resume them
    check_pause_timeouts();  // Check if any paused taps have exceeded the pause timeout to automatically close them
    
    check_targetpulses_to_autoclose(); // Check if any running taps have reached their target pulses to automatically close them
    check_state_timeouts(); // Check for state machine timeouts to reset to home idle state if user takes too long to input data    

    keypad_input_handler(); // Check for keypad input and update state machine variables accordingly
    update_state(); // Update the state machine based on current state and inputs
    check_for_rfid_tags(); // Check for RFID tag presence and read its ID to update state machine variables accordingly

}
