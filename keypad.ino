/**
 * @file keypad.ino
 * @brief This file contains the logic for handling the keypad input, including debouncing and mapping button presses to tap control actions
 */
#include "pinout.h"
#include <Keypad.h>

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
void keyapad_init() {
    customKeypad.setDebounceTime(50); // Set debounce time to 50ms
    customKeypad.setHoldTime(500); // Set hold time to 500

    for(byte i = 0; i < ROWS; i++) {
        pinMode(rowPins[i], INPUT_PULLUP); // Set row pins as input with pull-up resistors
    }
}

