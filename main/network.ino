/**
 * 
 * @file network.ino
 * this file controls gsm/gprs connection 
 * the reconnection attempts should be non-blocking to prevent 
 * system freezing
 */

#include "pinout.h"
#define NUM_NETWORK_RETRIES 3

/**
 * function to get sim imei
 */
static void get_imei() {
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
/**
 * @brief Takes raw RSSI value, converts to bars, and updates LCD display
 * @param rssi Raw RSSI value from modem.getSignalQuality()
 */
void displaySignalBars(int rssi) {
    if(rssi == 99 || rssi <= 0){
        signal_no_reading = true;
        current_signal_bars = 0;
    }else{
        signal_no_reading = false;
        // Convert RSSI to bar count
        if      (rssi >= 20 && rssi <= 31) current_signal_bars = 4;  // Excellent
        else if (rssi >= 15 && rssi <= 19) current_signal_bars = 3;  // Good
        else if (rssi >= 10 && rssi <= 14) current_signal_bars = 2;  // Fair
        else if (rssi >= 2  && rssi <= 9 ) current_signal_bars = 1;  // Weak
        else                               current_signal_bars = 0;  // No signal (covers 99, 0, -1)

    }

    //show network bars
  lcd.setCursor(19,0);
  if(signal_no_reading){
    lcd.write('X');

  }else{
    lcd.write(byte(current_signal_bars));
  }

}
/**
 * @function to create network bars
 * called in set up
 * 
 */
void initBarChars() {
  byte barGraph[5][8] = {
    {B01100, B01100, B01100, B01100, B01100, B00000, B01100, B01100},
    {B00000, B00000, B00000, B00000, B11000, B11000, B11000, B11000},
    {B00000, B00000, B00110, B00110, B00110, B11110, B11110, B11110},
    {B00000, B00000, B00011, B00011, B01111, B01111, B11111, B11111},
    {B00011, B00011, B00111, B00111, B01111, B01111, B11111, B11111}
  };
  
  for (int i = 0; i < 5; i++) {
    lcd.createChar(i, barGraph[i]);
  }
}
void reset_gsm(){
    digitalWrite(gsm_reset, LOW);
    delay(200);
    digitalWrite(gsm_reset, HIGH);
}
void gsm_init(){
    SerialMon.begin(115200);
    SerialAT.begin(115200);

    pinMode(gsm_reset, OUTPUT);
    digitalWrite(gsm_reset, HIGH);   // Keep modem out of reset

    SerialMon.println(F("Initialising..."));

    Serial.print(F("Waiting for network...."));
    if(!modem.waitForNetwork){
        Serial.println(F("fail"));

        reset_gsm();
        return;
    }
    Serial.println(F("succes"));
    if(modem.isNetworkConected){
        Serial.println(F("Network connected"));
        GSM_Net_Connect_Buzz();
        get_imei();
    }

    //connect to GPRS
    Serial.print(F("Connecting to "));
    Serial.print(apn);
    bool result = modem.gprsConnect(apn, gprsUser, gprsPass);
    if(result && modem.isGprsConnected()){
        Serial.println(F("Success"));
        int rssi = modem.getSignalQuality();
        displaySignalBars(rssi);
        GSM_GPRS_Connect_Buzz();
        mqtt.setServer(broker, mqtt_port);
        mqtt.setCallback(mqtt_callback);

        //connect to mqtt broker
        if (!mqtt.connected()) {
            SerialMon.println(F("[MQTT] Connecting to broker..."));
            String clientId = "waterhub_";
            clientId += sim_imei;
            if (mqtt.connect(clientId.c_str())) {
                SerialMon.println(F("[MQTT] Connected ✓"));
                mqtt.subscribe(topic_subscribe);   // Subscribe to command topic
                //Mqtt_Broker_Connect_Buzz();
            } else {
                SerialMon.print(F("[MQTT] Connect failed, rc="));
                SerialMon.println(mqtt.state());
            }
        } else {
            mqtt.loop();  // Keep MQTT client responsive
        }
    }else{
        Serial.println(F("Fail"));
        reset_gsm();
    }        

}

/**
 * function to check network connection with retries
 * checks for network connection after every 5 mins. if not connected, have three retries
 * and if still fails, wait for the next round
 */

bool network check(){
    Serial.println(F("Checking network connection"));
    //check if registered to network
    if(!modem.isNetworkConnected)
    {
        //counter for retries
        uint8_t net_retries = 0;
        while(net_retries <= NUM_NETWORK_RETRIES){
            if(modem.waitForNetwork(4000L, true)){
                Serial.println(F("Network re-connected"));
                if(!modem.isGprsConnected()){
                    if(!modem.gprsConnect(apn, gprsUser, gprsPass)){
                        Serial.println(F("GPRS connection failed"));
                        reset_gsm();
                    }
                }
                if(modem.isGprsConnected){
                    break;
                }
            }else{
                // Failed to reconnect to the network
                SerialMon.print(F("Failed to reconnect to the network at retry: "));
                SerialMon.println(net_retries);

                // Handle specific retry cases, e.g., reset GSM after 2 retries
                if (net_retries == 2)
                {
                SerialMon.println(F("At retry 2. resetting GSM..."));
                reset_gsm();
                }
            }
        }
    }
 }


