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
        rfid_initiated = false; 
        
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
            amount_buffer[input_index] = key; 
            input_index++;
            amount_buffer[input_index] = '\0'; 
            lcddisplay("Enter Amount", amount_buffer, "", "* CANCEL #OK"); 
            lcd.setCursor(input_index, 1); 
            lcd.blink_on(); 
            
        }
    }
    else if(key == '*') { // Handle backspace
        if(input_index > 0) {
            input_index--;
            amount_buffer[input_index] = '\0'; 
            lcddisplay("Enter Amount", amount_buffer, "", "* CANCEL #OK"); 
            lcd.setCursor(input_index, 1); 
            lcd.print(" "); 
            lcd.setCursor(input_index, 1);
            lcd.blink_on(); 
        }
        if(input_index == 0) { // If all digits are deleted, show prompt without amount
            current_state = HOME_IDLE; 
            reset_input_buffer(); 
            lcd.blink_off(); 
            lcd.clear(); 
            lcddisplay("Cancelled", "Returning to Home", "", ""); 
            delay(3000); 
            homescreen();    
        }
    }else if(key == '#'){
        if(input_index > 0) { // Ensure there is some amount input before proceeding
            if(active_transaction_type[selected_tap_index] == trxmpesapay) { 
                mqtt_publish_mpesa_pay(phone_buffer, selected_tap_index, amount_buffer);
                if(publish_flag) {
                    lcd.clear();
                    lcd.blink_off();
                    lcddisplay("Payment Initiated", "Please complete", "payment on your phone", ""); 
                    delay(3000);
                    taps[selected_tap_index].pending_open = true;
                    current_state = HOME_IDLE; 
                    homescreen();
                }
                else {
                    lcd.clear();
                    lcd.blink_off();
                    lcddisplay("Payment Failed", "Network Error", "Please try again", ""); 
                    delay(3000);
                    current_state = HOME_IDLE;
                    reset_input_buffer();
                    homescreen();
                }
            }
            else if(active_transaction_type[selected_tap_index] == trxcardpay) { 
                mqtt_publish_card_pay(scanned_tag_id.c_str(), selected_tap_index, amount_buffer); 
                if(publish_flag) {
                    lcd.clear();
                    lcd.blink_off();
                    lcddisplay("Payment Initiated", "Please wait ...", "", ""); 
                    delay(3000);
                    taps[selected_tap_index].pending_open = true;
                    current_state = HOME_IDLE; 
                    homescreen();
                }
                else {
                    lcd.clear();
                    lcd.blink_off();
                    lcddisplay("Payment Failed", "Network Error", "Please try again", ""); 
                    delay(3000);
                    current_state = HOME_IDLE;
                    reset_input_buffer();
                    homescreen();
                }
            }
            else if(rfid_initiated && selected_tap_index == -1) {
                mqtt_publish_card_topup(scanned_tag_id.c_str(), phone_buffer, amount_buffer);  
                if(publish_flag) {
                    lcd.clear();
                    lcd.blink_off();
                    lcddisplay("Top-up Initiated", "Please wait ...", "", ""); 
                    delay(3000);
                    current_state = HOME_IDLE; 
                    homescreen();
                }
                else {
                    lcd.clear();
                    lcd.blink_off();
                    lcddisplay("Top-up Failed", "Network Error", "Please try again", ""); 
                    delay(3000);
                    current_state = HOME_IDLE;
                    reset_input_buffer();
                    homescreen();
                }
            }
            
        }

    }
}
/**
 * @function to handle key press while waiting for tap or key 
 */
void handle_key_in_waiting_tap_or_key(char key){
    if(key == '*'){
        reset_input_buffer(); 
        state_machine_init(); 
        lcd.clear(); 
        lcddisplay("Transaction Cancelled", "Returning to Home", "", ""); 
        delay(3000); 
        homescreen(); 
    }
    else if(key >= '0' && key <= '9') {
        // If a number key is pressed while waiting for tap, treat it as confirmation to proceed with the transaction without tap
        current_state = ENTER_PHONE;
        state_entry_time = millis(); 
        lcd.clear(); 
        lcddisplay("Enter phone number", phone_buffer, "", "* CANCEL #OK");
        lcd.setCursor(input_index, 1); 
        lcd.blink_on(); 
        handle_phone_number_input(key); 
        
    }
}   

/**
 * @function to reset input buffer if transaction is cancelled
 */
void reset_input_buffer(){
    memset(phone_buffer, 0, sizeof(phone_buffer)); 
    input_index = 0; 
    memset(amount_buffer, 0, sizeof(amount_buffer)); 
    selected_tap_index = -1; 
    rfid_initiated = false; 
    tag_scanned = false; 


}