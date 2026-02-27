/**
 * @file timer.ino      
 */
#include "pinout.h"
bool led_blink_state = false; // Global variable to track LED state for blinking

/**
 * @function to initialize timer interrupts
 */
void timer_init(){
    TCCR4A = 0;
    TCCR4B = 0;
    TCNT4  = 0;
    OCR4A  = 5000;
    TCCR4B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);
    TIMSK4 |= (1 << OCIE4A);
}
/**
 * Tmer4 ISR
 * fires every ~500ms
 * 
 */
ISR(TIMER4_COMPA_vect) {
    led_blink_state = !led_blink_state; // Toggle LED state
    uint8_t blink_val = led_blink_state ? HIGH : LOW; // Determine whether to turn LED on or off based on state
    for(uint8_t i = 0; i < NUM_OF_TAPS; i++) {
        if(taps[i].pending_open ) {
            digitalWrite(taps[i].led_pin, blink_val);
        } 
    }
}
