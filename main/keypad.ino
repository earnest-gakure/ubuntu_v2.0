/**
 * @file keypad.ino
 * @brief This file contains the logic for handling the keypad input, including debouncing and mapping button presses to tap control actions
 */
#include "pinout.h"
#include <Keypad.h>
#include <MFRC522.h>   

// Keypad configuration
const byte ROWS = 4; // Four rows
const byte COLS = 3; // Four columns
char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
    };

byte rowPins[ROWS] = {5, 10, 9,7}; // Connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 4, 8}; // Connect to the column pinouts of the keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

/**
 * function to initialize keypad
 */
void keypad_init() {
    customKeypad.setDebounceTime(50); // Set debounce time to 50ms
    customKeypad.setHoldTime(500); // Set hold time to 500

    for(byte i = 0; i < ROWS; i++) {
        pinMode(rowPins[i], INPUT_PULLUP); // Set row pins as input with pull-up resistors
    }
}

/**
 * keyapad input handling function to be 
 * called in the main loop to check for key presses and update state machine variables accordingly
 */

void keypad_input_handler() {
    char key = customKeypad.getKey();
    if(!key) return; // No key pressed, exit the function
    keypadbuzz(); // Buzz to indicate key press
    switch(current_state) {
        case HOME_IDLE:
            // If in home idle state
            handle_idle_keypress(key); // Handle key press for idle state (e.g. transition to phone number entry)

            break;
        case ENTER_PHONE:
            // Handle phone number input
            handle_phone_number_input(key); // Handle key press for phone number entry (e.g. update phone number buffer, handle backspace, etc.)
            break;
        case ENTER_AMOUNT:
            // Handle amount input
            handle_amount_input(key); // Handle key press for amount entry (e.g. update amount buffer, handle backspace, etc.)
            
            break;
        case ENTER_TAP:
            // 
            handle_key_in_waiting_tap(key); // Handle key press for waiting for tap state (e.g. allow cancellation with * key)
            break;
        case WAITING_FOR_TAP_OR_KEY:

            handle_key_in_waiting_tap_or_key(key); // Handle key press for waiting for tap or key state (e.g. allow cancellation with * key)
            break;
    }
}
/**
 * @function to handle key on idle state
 */
void handle_idle_keypress(char key) {
    //if 0-9 is pressed, transition to phone number entry state
    if(key >= '0' && key <= '9') {
        current_state = ENTER_PHONE;
        state_entry_time = millis(); // Record the time when entering this state for timeout handling
        handle_phone_number_input(key); // Handle the first digit of phone number input
        lcd.clear(); // Clear the LCD display
        lcddisplay("Enter Phone Number", phone_buffer, "", "* CANCEL"); // Update LCD to show phone number entry prompt
        lcd.setCursor(input_index, 1);
        lcd.blink_on(); // Blink cursor to indicate input position for phone number entry
        
    }
}

/**
 * @function to handle a key press on mpesa phone numberentry
 */
void handle_phone_number_input(char key) {
    if(key >= '0' && key <= '9'){
        if(input_index < 10) { // Ensure we don't exceed buffer size (10 digits + null terminator)
            phone_buffer[input_index] = key; // Add digit to phone number buffer
            input_index++;
            phone_buffer[input_index] = '\0'; // Null-terminate the string
            lcddisplay("Enter Phone Number", phone_buffer, "", "* CANCEL"); // Update LCD to show current phone number input
            lcd.blink_on(); // Blink cursor to indicate input position
            lcd.setCursor(input_index, 1); // Move cursor to the right of the last entered digit
        }
    }else if(key == '*') { // Handle backspace
        if(input_index > 0) {
            input_index--;
            phone_buffer[input_index] = '\0'; // Null-terminate after removing last digit
            lcddisplay("Enter Phone Number", phone_buffer, "", "* CANCEL"); // Update LCD to show current phone number input
            lcd.setCursor(input_index, 1); // Move cursor to the position of the deleted digit
            lcd.print(" "); // Clear the character on the LCD
            lcd.setCursor(input_index, 1); // Move cursor back to the position of the deleted digit
        }
        if(input_index == 0) { // If all digits are deleted, show prompt without phone number
            current_state = HOME_IDLE; // Transition back to home idle state if user deletes all phone number input
            reset_input_buffer(); // Reset input buffers and variables
            lcd.blink_off(); // Turn off cursor blinking
            lcd.clear(); // Clear the LCD display
            lcddisplay("Cancelled", "Returning to Home", "", ""); // Update LCD to show cancellation message
            delay(3000); // Wait for 3 seconds to allow user to read message
            homescreen(); // Show home screen on LCD    
        }
    }
    
}
/**
 * @function to handle key press for tap entry state
 */
void handle_key_in_waiting_tap(char key){
    if(key == '*'){
        reset_input_buffer(); // Reset input buffers and variables if transaction is cancelled
        state_machine_init(); // Reset state machine to home idle
        lcddisplay("Transaction Cancelled", "Returning to Home", "", ""); // Update LCD to show cancellation message
        delay(3000); // Wait for 3 seconds to allow user to read message
        homescreen(); // Show home screen on LCD
    }
}
/**
 * @function to handle key press for amount entry state
 * @called from keypad input handler when in ENTER_AMOUNT state to process amount input from keypad
 */
void handle_amount_input(char key) {
    if(key >= '0' && key <= '9'){
        if(input_index < 4) { // Ensure we don't exceed buffer size (4 digits + null terminator)
            amount_buffer[input_index] = key; // Add digit to amount buffer
            input_index++;
            amount_buffer[input_index] = '\0'; // Null-terminate the string
            lcddisplay("Enter Amount", amount_buffer, "", "* CANCEL #OK"); // Update LCD to show current amount input
            lcd.setCursor(input_index, 1); // Move cursor to third line to show amount as it's being entered
            lcd.blink_on(); // Blink cursor to indicate input position
            
        }
    }
    else if(key == '*') { // Handle backspace
        if(input_index > 0) {
            input_index--;
            amount_buffer[input_index] = '\0'; // Null-terminate after removing last digit
            lcddisplay("Enter Amount", amount_buffer, "", "* CANCEL #OK"); // Update LCD to show current amount input
            lcd.setCursor(input_index, 1); // Move cursor to the position of the deleted
            lcd.print(" "); // Clear the character on the LCD
            lcd.setCursor(input_index, 1); // Move cursor back to the position of the deleted digit
        }
        if(input_index == 0) { // If all digits are deleted, show prompt without amount
            current_state = HOME_IDLE; // Transition back to tap selection state if user deletes all amount input
            reset_input_buffer(); // Reset input buffers and variables
            lcd.blink_off(); // Turn off cursor blinking
            lcd.clear(); // Clear the LCD display
            lcddisplay("Cancelled", "Returning to Home", "", ""); // Update LCD to show cancellation message
            delay(3000); // Wait for 3 seconds to allow user to read message
            homescreen(); // Show home screen on LCD    
        }
    }else if(key == '#'){
        if(input_index > 0) { // Ensure there is some amount input before proceeding
            current_state = HOME_IDLE; // Transition back to home idle state after confirming amount entry
            lcd.clear(); // Clear the LCD display
            lcd.blink_off(); // Turn off cursor blinking
            lcddisplay("Processing Payment", "", "", ""); // Update LCD to show processing message
            delay(2000); // Simulate processing time
            // Here you would add the code to actually process the payment with the entered phone number and amount
            reset_input_buffer(); // Reset input buffers and variables after processing
            homescreen(); // Show home screen on LCD after processing   
        }

    }
}
/**
 * @function to handle key press while waiting for tap or key 
 */
void handle_key_in_waiting_tap_or_key(char key){
    if(key == '*'){
        reset_input_buffer(); // Reset input buffers and variables if transaction is cancelled
        state_machine_init(); // Reset state machine to home idle
        lcd.clear(); // Clear the LCD display
        lcddisplay("Transaction Cancelled", "Returning to Home", "", ""); // Update LCD to show cancellation message
        delay(3000); // Wait for 3 seconds to allow user to read message
        homescreen(); // Show home screen on LCD
    }
    else if(key >= '0' && key <= '9') {
        // If a number key is pressed while waiting for tap, treat it as confirmation to proceed with the transaction without tap
        current_state = ENTER_PHONE; // Transition back to home idle state after confirming with key press
        state_entry_time = millis(); // Record the time when entering this state for timeout handling
        lcd.clear(); // Clear the LCD display
        lcddisplay("Enter phone number", phone_buffer, "Enter Amount", "* CANCEL #OK"); // Update LCD to show amount entry prompt with phone number displayed
        lcd.setCursor(input_index, 1); // Move cursor to third line to show amount as it's being entered
        lcd.blink_on(); // Blink cursor to indicate input position for amount entry
        handle_phone_number_input(key); // Handle the first digit of amount input
        
    }
}   

/**
 * @function to reset input buffer if transaction is cancelled
 */
void reset_input_buffer(){
    memset(phone_buffer, 0, sizeof(phone_buffer)); // Clear phone number buffer
    input_index = 0; // Reset input index
    memset(amount_buffer, 0, sizeof(amount_buffer)); // Clear amount buffer 
    selected_tap_index = -1; // Reset selected tap index

}