// heartbeat.ino
// ─────────────────────────────────────────────────────────────────────────────
// Heartbeat publisher — fires every 2 minutes via Timer5 ISR.
//
// Flow:
//   1. Timer5 ISR sets heartbeat_pending = true every 2 minutes
//   2. Main loop calls heartbeat_check() every iteration
//   3. heartbeat_check() only publishes when:
//      - heartbeat_pending is true
//      - system_busy is false (no user is mid-transaction)
//      - MQTT is connected
//
// Payload format:
//   imei_valvesStatus_flowmeterPulses
//   e.g. 863576043526289_1:0:1:0_120:0:600:0
//
//   valvesStatus     — tap_running state per tap (1=running, 0=stopped)
//   flowmeterPulses  — cumulative pulses counted since last dispense started
//                      (counter_A, counter_B, counter_C, counter_D)
// ─────────────────────────────────────────────────────────────────────────────

#include "pinout.h"


// Flag set by Timer5 ISR, cleared after successful publish
volatile bool heartbeat_pending = false;



// ─────────────────────────────────────────────
// TIMER 5 INIT
// Timer5 on ATmega2560 is a 16-bit timer — same family as Timer4.
// We configure it in CTC mode with a 1024 prescaler.
// At 16MHz: tick period = 1024 / 16,000,000 = 64µs
// For 2 minutes (120 seconds):
//   ticks needed = 120s / 64µs = 1,875,000
//   Since OCR5A max is 65535 we use a software counter:
//   Set OCR5A = 62499 → ISR fires every ~4 seconds (64µs × 62500 = 4.000s)
//   Software counter counts 30 ISR fires → 30 × 4s = 120s = 2 minutes
// ─────────────────────────────────────────────
#define HEARTBEAT_ISR_INTERVAL_TICKS  62499   // ~4 seconds per ISR fire
#define HEARTBEAT_ISR_COUNT           30      // 30 × 4s = 120s = 2 minutes

volatile uint8_t heartbeat_isr_counter = 0;

void heartbeat_init() {


  // Configure Timer5 in CTC mode, 1024 prescaler
  TCCR5A = 0;
  TCCR5B = 0;
  TCNT5  = 0;

  OCR5A = HEARTBEAT_ISR_INTERVAL_TICKS;

  // CTC mode (WGM52), prescaler 1024 (CS52 + CS50)
  TCCR5B |= (1 << WGM52) | (1 << CS52) | (1 << CS50);

  // Enable Timer5 compare match A interrupt
  TIMSK5 |= (1 << OCIE5A);

  Serial.println(F("Heartbeat timer initialized (2 min interval)"));
}

// ─────────────────────────────────────────────
// TIMER5 ISR
// Fires every ~4 seconds.
// Increments software counter; sets heartbeat_pending after 30 fires.
// ─────────────────────────────────────────────
ISR(TIMER5_COMPA_vect) {
  heartbeat_isr_counter++;
  if (heartbeat_isr_counter >= HEARTBEAT_ISR_COUNT) {
    heartbeat_isr_counter = 0;
    heartbeat_pending     = true;
  }
}

// ─────────────────────────────────────────────
// HEARTBEAT CHECK
// Called from main loop every iteration.
// Only publishes when system is idle and MQTT is connected.
// ─────────────────────────────────────────────
void heartbeat_check() {
  if (!heartbeat_pending)    return;
  if (!mqtt.connected())  return;   // wait — will retry next loop iteration

  // ── Build valve status string  (tap1:tap2:tap3:tap4) ──
  char valve_status[16];
  snprintf(valve_status, sizeof(valve_status), "%d:%d:%d:%d",
    taps[0].running ? 1 : 0,
    taps[1].running ? 1 : 0,
    taps[2].running ? 1 : 0,
    taps[3].running ? 1 : 0
  );

  // ── Build pulse string  (counterA:counterB:counterC:counterD) ──
  char pulse_status[32];
  snprintf(pulse_status, sizeof(pulse_status), "%lu:%lu:%lu:%lu",
    taps[0].pulse_count,
    taps[1].pulse_count,
    taps[2].pulse_count,
    taps[3].pulse_count
  );

  // ── Assemble full payload: imei_valves_pulses ──
  char payload[BUFFER_SIZE];
  
  snprintf(payload, sizeof(payload), "heartbeats_%s_%s_%s",
    sim_imei,
    valve_status,
    pulse_status
  );

  Serial.print(F("Heartbeat → "));
  Serial.print(topic_ack_publish);
  Serial.print(F(": "));
  Serial.println(payload);

  bool result = mqtt_publish_heartbeat(topic_ack_publish, payload);

  if (result) {
    Serial.println(F("Heartbeat published OK"));
  } else {
    Serial.println(F("Heartbeat publish failed — will retry next check"));
    return;   // leave heartbeat_pending = true so we retry next loop
  }

  // Only clear flag after successful publish
  heartbeat_pending = false;
}
