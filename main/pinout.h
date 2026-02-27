/**
 * @file pinout.h
 * @brief Pin definitions and global variables
 */

#ifndef PINOUT_H
#define PINOUT_H

// ── Modem selection ───────────────────────────────────────────
#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT  Serial2


#include <stdio.h>
#include <stdint.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>
#include <MFRC522.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>

// ── Modem selection ───────────────────────────────────────────
#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT  Serial2

// ── APN / credentials ─────────────────────────────────────────
const char apn[]      = "iot.safaricom.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char *broker  = "167.99.196.28";
const int   mqtt_port = 1883;



TinyGsm         modem(SerialAT);
TinyGsmClient   client(modem);
PubSubClient    mqttClient(client);

// ── GSM connection manager settings ──────────────────────────
// Maximum number of soft retries before a hard (pin) reset is attempted
static const uint8_t  MAX_RETRIES        = 3;

// How often the heartbeat checks link health (ms)
static const uint32_t HEARTBEAT_INTERVAL = 60000UL;   // 60 s

// ── Hardware ──────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 20, 4);

#define PAUSE_TIMEOUT        180000UL  // 3 min – tap auto-close when paused
#define STATEMACHINE_TIMEOUT  45000UL  // 45 s  – UI input timeout

#define NUM_OF_TAPS 4

// Motor pins
#define motor1_close  23
#define motor1_open   25
#define motor2_close  29
#define motor2_open   27
#define motor3_close  33
#define motor3_open   31
#define motor4_close  37
#define motor4_open   35

// Flowmeter interrupt pins
#define flowmeter_1   19
#define flowmeter_2   18
#define flowmeter_3    2
#define flowmeter_4    3

// GSM reset pin
#define gsm_reset     13

// Buzzer
#define buzzer_pin    28

// Indicator LEDs
#define tap_1_led     39
#define tap_2_led     41
#define tap_3_led     43
#define tap_4_led     45

// ── Tap data structure ────────────────────────────────────────
typedef struct {
    uint8_t motor_open_pin;
    uint8_t motor_close_pin;
    uint8_t flowmeter_pin;
    uint8_t led_pin;
    uint8_t button_pin;

    bool pending_open;
    bool running;
    bool paused;
    bool button_pressed;

    volatile uint32_t pulse_count;
    uint32_t          target_pulses;
    unsigned long     pause_start_time;
} Tap;

extern Tap taps[NUM_OF_TAPS];

// ── Keypad / input buffers ────────────────────────────────────
extern char    phone_buffer[11];
extern char    amount_buffer[5];
extern uint8_t input_index;

// ── State machine ─────────────────────────────────────────────
enum systenstates { HOME_IDLE, ENTER_PHONE, ENTER_AMOUNT, ENTER_TAP, WAITING_FOR_TAP_OR_KEY };
extern systenstates  current_state;
extern unsigned long state_entry_time;

// ── M-Pesa ────────────────────────────────────────────────────
#define BUFFER_SIZE 256
extern char    mpesa_phone_number[11];
extern char    mpesa_amount[5];
extern uint8_t selected_tap_index;

// ── RFID ──────────────────────────────────────────────────────
extern MFRC522 mfrc522;
extern String  scanned_tag_id;
extern bool    tag_scanned;
extern bool    rfid_initiated;

// ── GSM ───────────────────────────────────────────────────────
extern char sim_imei[17];

// Public GSM API (implemented in network.ino)
void gsm_init();        // Call once from setup()
void gsm_update();      // Call every loop() — drives non-blocking reconnect
bool gsm_is_connected(); // True when network + GPRS are both up

//MQTT

char delim[] = "_";
extern bool publish_flag;

char trxcardpay[8];
char trxcardtopup[10];
char trxmpesapay[9];
char trxremotedispense[15];

extern String active_transaction_type[4];



#endif
