/**
 * @file taps.ino
 * @brief this file contains all pin definitions and global variables related to the pins used in the project
 */

#ifndef PIOUT_H
#define PIOUT_H

#include <stdio.h>
#include <stdint.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);



#define NUM_OF_TAPS 4
//pins 
#define  motor1_close   23
#define  motor1_open    25
#define  motor2_close   29
#define  motor2_open    27
#define  motor3_close   33
#define  motor3_open    31
#define  motor4_close   37
#define  motor4_open    35



// /**Flowmeter pins**/ ORIGINAL
#define  flowmeter_1    19
#define  flowmeter_2    18
#define  flowmeter_3    2
#define  flowmeter_4    3

/**GSM Reset Pin**/
#define  gsm_reset      13
/**Buzzer Pin**/
#define buzzer_pin      28


/*Indicator LEDs*/
#define  tap_1_led      39
#define  tap_2_led      41
#define  tap_3_led      43
#define  tap_4_led      45

typedef struct {
    //hardware pins
    uint8_t motor_open_pin;
    uint8_t motor_close_pin;
    uint8_t flowmeter_pin;
    uint8_t led_pin;
    uint8_t button_pin ;
    

    //state flags
    bool pending_open;
    bool running;
    bool paused;
    bool button_pressed;

    //counters and timers
    volatile uint32_t pulse_count;
    uint32_t target_pulses; 
    unsigned long pause_start_time;

} Tap;

extern Tap taps[NUM_OF_TAPS];
#define PAUSE_TIMEOUT 180000 //3 minutes in milliseconds

//keypad pin definitions and global variables
extern char phone_buffer[11]; 
extern char amount_buffer[5];
extern uint8_t input_index;

//state machine global variables
enum systenstates {HOME_IDLE, ENTER_PHONE, ENTER_AMOUNT, ENTER_TAP};
extern systenstates current_state;

//mpesa global variables
#define BUFFER_SIZE 256
#define STATEMACHINE_TIMEOUT 120000 // 60 seconds timeout for user input in state machine
extern char mpesa_phone_number[11];
extern char mpesa_amount[5];
extern uint8_t selected_tap_index;
extern uint8_t input_index;
extern unsigned long state_entry_time;

#endif


