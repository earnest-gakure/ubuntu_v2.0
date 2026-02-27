/**
 * @file network.ino
 * @brief Non-blocking GSM/GPRS connection manager.
 *
 * Design goals
 * ─────────────
 * • Zero blocking delays in the main loop — taps, keypad, LCD and RFID keep
 *   running even while the modem is (re)connecting.
 * • A small state machine drives every phase of bring-up and reconnection.
 * • Hard-reset is tried only after all soft retries are exhausted.
 * • A periodic heartbeat checks link health without stalling anything.
 * • All timing uses millis() comparisons.
 */

#include "pinout.h"

// ═══════════════════════════════════════════════════════════════
//  Internal state machine
// ═══════════════════════════════════════════════════════════════

enum GsmState : uint8_t {
    GSM_IDLE,               // Not yet started / fully connected
    GSM_MODEM_INIT,         // Sending AT, waiting for modem to wake up
    GSM_WAIT_NETWORK,       // Polling for network registration
    GSM_CONNECT_GPRS,       // Attempting GPRS attach
    GSM_CONNECTED,          // Fully up – heartbeat polls from here
    GSM_HARD_RESET_WAIT,    // Holding reset pin LOW
    GSM_POST_RESET_WAIT,    // Waiting after releasing reset before re-init
    GSM_RETRY_DELAY,        // Soft back-off between attempts
};

static GsmState  gsm_state         = GSM_IDLE;
static uint32_t  gsm_timer         = 0;   // General-purpose non-blocking timer
static uint8_t   retry_count       = 0;
static uint32_t  last_heartbeat    = 0;

// How long to hold the modem's RST pin low during hard-reset
static const uint32_t HARD_RESET_HOLD_MS   = 200UL;
// How long to wait after releasing RST before talking to the modem again
static const uint32_t POST_RESET_WAIT_MS   = 3000UL;
// How often to poll waitForNetwork() — keeps loop() responsive
static const uint32_t NET_POLL_INTERVAL_MS = 500UL;
// Maximum time we'll spend waiting for network registration per attempt
static const uint32_t NET_WAIT_LIMIT_MS    = 15000UL;
// Maximum time we'll spend waiting for GPRS attach per attempt
static const uint32_t GPRS_WAIT_LIMIT_MS   = 30000UL;
// Back-off between soft retries (non-blocking – other tasks run during this)
static const uint32_t SOFT_RETRY_DELAY_MS  = 10000UL;

// Per-phase start time (used to enforce timeouts within a state)
static uint32_t phase_start = 0;

char sim_imei[17] = "";

// ═══════════════════════════════════════════════════════════════
//  Forward declarations of internal helpers
// ═══════════════════════════════════════════════════════════════
static void _enter_state(GsmState next);
static void _begin_hard_reset();
static void _get_imei();

// ═══════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Call once from setup().  Does NOT block — only sets baud rates,
 *        configures the reset pin, and kicks off the state machine.
 */
void gsm_init() {
    SerialMon.begin(115200);
    SerialAT.begin(115200);

    pinMode(gsm_reset, OUTPUT);
    digitalWrite(gsm_reset, HIGH);   // Keep modem out of reset

    SerialMon.println(F("[GSM] Initialising (non-blocking)..."));
    _enter_state(GSM_MODEM_INIT);
}

/**
 * @brief Call every loop() iteration.  Drives the connection state machine
 *        and the periodic heartbeat.  Returns immediately if nothing to do.
 */
void gsm_update() {
    uint32_t now = millis();

    switch (gsm_state) {

    // ── Phase 1: wake the modem ───────────────────────────────
    case GSM_MODEM_INIT:
        // modem.init() is itself quick (just sends AT commands synchronously),
        // but we only call it once and then move on.  If your TinyGSM version
        // makes it async you can split it further; for SIM800 it is fast enough.
        if (modem.init()) {
            SerialMon.println(F("[GSM] Modem OK → waiting for network"));
            String info = modem.getModemInfo();
            SerialMon.print(F("[GSM] Modem info: "));
            SerialMon.println(info);
            _enter_state(GSM_WAIT_NETWORK);
        } else {
            SerialMon.println(F("[GSM] Modem init failed"));
            retry_count++;
            if (retry_count >= MAX_RETRIES) {
                SerialMon.println(F("[GSM] Max retries – hard reset"));
                _begin_hard_reset();
            } else {
                _enter_state(GSM_RETRY_DELAY);
            }
        }
        break;

    // ── Phase 2: wait for GSM/LTE registration ───────────────
    case GSM_WAIT_NETWORK:
        // Poll every NET_POLL_INTERVAL_MS so the loop stays responsive
        if (now - gsm_timer >= NET_POLL_INTERVAL_MS) {
            gsm_timer = now;

            if (modem.isNetworkConnected()) {
                SerialMon.println(F("[GSM] Network registered ✓"));
                GSM_Net_Connect_Buzz();
                _get_imei();
                _enter_state(GSM_CONNECT_GPRS);

            } else if (now - phase_start >= NET_WAIT_LIMIT_MS) {
                // Timed out waiting for registration
                SerialMon.println(F("[GSM] Network registration timeout"));
                retry_count++;
                if (retry_count >= MAX_RETRIES) {
                    _begin_hard_reset();
                } else {
                    _enter_state(GSM_RETRY_DELAY);
                }
            }
            // else: still waiting, return and let loop() do other work
        }
        break;

    // ── Phase 3: GPRS attach ──────────────────────────────────
    case GSM_CONNECT_GPRS:
        // gprsConnect() can itself block internally in TinyGSM for SIM800.
        // We wrap it in a timeout guard so at worst we stall for
        // GPRS_WAIT_LIMIT_MS once, then give up gracefully.
        if (now - phase_start < GPRS_WAIT_LIMIT_MS) {

            if (modem.gprsConnect(apn, gprsUser, gprsPass)) {
                if (modem.isGprsConnected()) {
                    SerialMon.println(F("[GSM] GPRS connected ✓"));
                    Mqtt_Broker_Connect_Buzz();   // re-using existing 3-beep signal
                    mqtt.setServer(broker, mqtt_port);
                    mqtt.setCallback(mqtt_callback);
                    retry_count   = 0;
                    last_heartbeat = now;
                    _enter_state(GSM_CONNECTED);
                } else {
                    // gprsConnect returned true but isGprsConnected is false —
                    // unusual; fall through to timeout path below
                    SerialMon.println(F("[GSM] GPRS connect anomaly"));
                }
            }
            // gprsConnect() returned false — keep retrying until timeout

        } else {
            SerialMon.println(F("[GSM] GPRS attach timeout"));
            retry_count++;
            if (retry_count >= MAX_RETRIES) {
                _begin_hard_reset();
            } else {
                _enter_state(GSM_RETRY_DELAY);
            }
        }
        break;
    case GSM_CONNECTED:
        if (now - last_heartbeat >= HEARTBEAT_INTERVAL) {
            last_heartbeat = now;
            bool net  = modem.isNetworkConnected();
            bool gprs = modem.isGprsConnected();
            SerialMon.print(F("[GSM] Heartbeat – Network: "));
            SerialMon.print(net  ? F("OK") : F("DOWN"));
            SerialMon.print(F("  GPRS: "));
            SerialMon.println(gprs ? F("OK") : F("DOWN"));

            if (!net || !gprs) {
                SerialMon.println(F("[GSM] Link lost → reconnecting"));
                retry_count = 0;
                _enter_state(gprs ? GSM_WAIT_NETWORK : GSM_CONNECT_GPRS);
            }
        }

        // ── MQTT connection management ────────────────────────────
        if (!mqtt.connected()) {
            SerialMon.println(F("[MQTT] Connecting to broker..."));
            String clientId = "waterhub_";
            clientId += sim_imei;
            if (mqtt.connect(clientId.c_str())) {
                SerialMon.println(F("[MQTT] Connected ✓"));
                mqtt.subscribe(topic_subscribe);   // Subscribe to command topic
                Mqtt_Broker_Connect_Buzz();
            } else {
                SerialMon.print(F("[MQTT] Connect failed, rc="));
                SerialMon.println(mqtt.state());
            }
        } else {
            mqtt.loop();  // Keep MQTT client responsive
        }
        break;

    // ── Non-blocking back-off between soft retries ────────────
    case GSM_RETRY_DELAY:
        if (now - gsm_timer >= SOFT_RETRY_DELAY_MS) {
            SerialMon.print(F("[GSM] Retry attempt "));
            SerialMon.println(retry_count);
            _enter_state(GSM_MODEM_INIT);
        }
        // else: waiting — loop() continues normally
        break;

    // ── Hard reset: hold RST low ──────────────────────────────
    case GSM_HARD_RESET_WAIT:
        if (now - gsm_timer >= HARD_RESET_HOLD_MS) {
            digitalWrite(gsm_reset, HIGH);   // Release reset
            SerialMon.println(F("[GSM] RST released, waiting for boot"));
            _enter_state(GSM_POST_RESET_WAIT);
        }
        break;

    // ── Hard reset: wait for modem to boot ───────────────────
    case GSM_POST_RESET_WAIT:
        if (now - gsm_timer >= POST_RESET_WAIT_MS) {
            SerialAT.begin(115200);
            retry_count = 0;
            SerialMon.println(F("[GSM] Post-reset boot complete → re-init"));
            _enter_state(GSM_MODEM_INIT);
        }
        break;

    case GSM_IDLE:
    default:
        break;
    }
}

/**
 * @brief Returns true when the modem is registered and GPRS is up.
 *        Safe to call from anywhere (no side-effects).
 */
bool gsm_is_connected() {
    return gsm_state == GSM_CONNECTED;
}

// ═══════════════════════════════════════════════════════════════
//  Internal helpers
// ═══════════════════════════════════════════════════════════════

static void _enter_state(GsmState next) {
    gsm_state  = next;
    gsm_timer  = millis();
    phase_start = gsm_timer;
}

static void _begin_hard_reset() {
    SerialMon.println(F("[GSM] Hard reset triggered"));
    retry_count = 0;
    digitalWrite(gsm_reset, LOW);
    _enter_state(GSM_HARD_RESET_WAIT);
}

static void _get_imei() {
    SerialAT.println(F("AT+GSN"));
    delay(150);   // One-off, only called after successful registration
    uint8_t i = 0;
    while (SerialAT.available() && i < 16) {
        char c = SerialAT.read();
        if (c >= '0' && c <= '9') {
            sim_imei[i++] = c;
        }
    }
    sim_imei[i] = '\0';
    SerialMon.print(F("[GSM] IMEI: "));
    SerialMon.println(sim_imei);
    build_mqtt_topics(); // Now that we have the IMEI, build the MQTT topics
}
