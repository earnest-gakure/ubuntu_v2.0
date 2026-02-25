#include "pinout.h"

/**
 * @file flowmeters.ino
 * @brief Flowmeter interrupt service routines
 * Simple wrappers that increment counters and check dispense completion
 */
// Flowmeter ISRs for each tap
void flowmeter_ISR_1() {
    taps[0].pulse_count++;
}   
void flowmeter_ISR_2() {
    taps[1].pulse_count++;
}
void flowmeter_ISR_3() {
    taps[2].pulse_count++;
}
void flowmeter_ISR_4() {
    taps[3].pulse_count++;
}  
// Array of function pointers for flowmeter ISRs
void (*flowmeter_ISRs[NUM_OF_TAPS])() = {
    flowmeter_ISR_1,
    flowmeter_ISR_2,
    flowmeter_ISR_3,
    flowmeter_ISR_4
};  
      