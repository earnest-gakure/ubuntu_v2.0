/**
 * @file statemachine.ino
 * @brief This file contains the main state machine logic for controlling the taps, including handling button presses, managing tap states (open, close, pause), and updating the LCD display accordingly.
 */
#include "pinout.h"

systenstates current_state = HOME_IDLE;   // Initialize the current state to HOME_IDLE
char phone_buffer[11] = {0};        // Buffer to store the entered phone number
char amount_buffer[5] = {0};               // Buffer to store the entered amount
uint8_t input_index = 0;                  // Index for tracking input position in buffers    
uint8_t selected_tap_index = -1;          // Variable to track which tap is selected for top-up (initialized to -1 for no selection)
unsigned long state_entry_time = 0;      // Variable to track the time when the current state was entered (for timeout handling)
/**
 * @function to reset the state machine to the home idle state and clear input buffers
 */
void state_machine_init() {
  current_state = HOME_IDLE;
  memset(phone_buffer, 0, sizeof(phone_buffer));
  memset(amount_buffer, 0, sizeof(amount_buffer));
  input_index = 0;
  selected_tap_index = -1;
}

/**
 * @function to handle stste machine timeouts and transitions back to home idle state if user takes too long to input data
 * @called from main loop to check for timeouts in different states
 */
void check_state_timeouts() {
    unsigned long elapsed = millis() - state_entry_time; // Calculate elapsed time since entering the current state
   if(current_state != HOME_IDLE && elapsed > STATEMACHINE_TIMEOUT) { // If not in HOME_IDLE and more than 60 seconds have passed
        state_machine_init(); // Reset the state machine to HOME_IDLE
        lcddisplay("Session Timeout", "Please Try Again", "", ""); // Update the LCD to show timeout message
        delay(3000); // Wait for 3 seconds to allow the user to read the timeout message
        homescreen(); // Update the LCD to show the home screen
        Serial.println("State machine reset to HOME_IDLE due to timeout."); // Log the timeout event
    }   
}

/**
 * @function to handle stet machine transitions
 */
void update_state(){
    switch(current_state) {
        case HOME_IDLE:
        // Stay in home idle until a key is pressed or tag is tapped
            
        break;
        case ENTER_PHONE:
            // Wait for user to enter phone number, then transition to ENTER_AMOUNT
            if(input_index == 10) { // Assuming phone number is 10 digits
                current_state = ENTER_TAP; // Transition to tap selection state after phone number entry    
                state_entry_time = millis(); // Record the time when entering this state for timeout handling
                input_index = 0; // Reset input index for amount entry
                lcd.clear(); // Clear the LCD display
                lcd.blink_off(); // Turn off cursor blinking
                lcddisplay("Select Tap", "", "", ""); // Update LCD to show tap selection prompt
            }
        break;
        case ENTER_AMOUNT:
        
        break;
        case ENTER_TAP:
            // Wait for user to select tap or cancel with * key
            for(uint8_t i = 0; i < NUM_OF_TAPS; i++) {
                if(taps[i].button_pressed) { // If a tap button is pressed
                    selected_tap_index = i; // Set the selected tap index
                    taps[i].button_pressed = false; // Reset the button pressed flag
                    taps[i].pending_open = true; // Set the pending open flag for the selected tap
                    current_state = ENTER_AMOUNT; // Transition to amount entry state
                    state_entry_time = millis(); // Record the time when entering this state for timeout handling
                    input_index = 0; // Reset input index for amount entry
                    lcd.clear(); // Clear the LCD display
                    lcddisplay("Enter Amount", "", "", "* CANCEL #OK"); // Update LCD to show amount entry prompt
                    Serial.print("Selected Tap: ");
                    Serial.println(i);
                    break;
                }
            }

        break;
    }
}
