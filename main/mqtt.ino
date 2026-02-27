/**
 * @mqtt.ino
 */
#include "pinout.h"

char topic_publish[64];
char topic_subscribe[64];
char topic_ack_publish[64];


bool publish_flag = false;
char trxcardpay[8] = "cardpay";
char trxcardtopup[10] = "cardtopup";
char trxmpesapay[9] = "mpesapay";
char trxremotedispense[15] = "remotedispense";
String active_transaction_type[4] = {"", "", "", ""};


/**
 * @function to create topics
 */
void build_mqtt_topics() {
  strcpy(topic_publish,     "atm/transactions/");    strcat(topic_publish,     sim_imei);
  strcpy(topic_subscribe,   "atm/commands/");        strcat(topic_subscribe,   sim_imei);
  strcpy(topic_ack_publish, "atm/acknowledgement/"); strcat(topic_ack_publish, sim_imei);
  SerialMon.print(F("Topics built for IMEI: "));
  SerialMon.println(sim_imei);
}

/**
 * @function to send mqtt
 */
bool mqtt_publish(const char* topic, const char* payload) {
    if(!mqtt.connected()) {
        SerialMon.println(F("[MQTT] Not connected, cannot publish"));
        return false;
    }
    SerialMon.print(F("PUBLISH topic: "));
    SerialMon.print(topic);
    SerialMon.print(F(" : "));
    SerialMon.println(payload);
    return mqtt.publish(topic, payload);    
}

/**
 * @function to send MPESA pay mqtt
 */
void mqtt_publish_mpesa_pay(const char* phone,uint8_t tap_index , const char* amount) {
    char tap_index_str[3];
    char mqttmessage[BUFFER_SIZE];
    sprintf(tap_index_str, "%d", tap_index); // Convert tap index to string
    //construct the MQTT topic and message payload
    snprintf(mqttmessage, BUFFER_SIZE, 
        "%s%s%s%s%s%s%s%s%s",
        "mpesapay",delim, sim_imei, delim, phone, delim, amount, delim, tap_index_str);
    publish_flag = mqtt_publish(topic_publish, mqttmessage);   
    
}
/**
 * @function to send card pay mqtt
 */
void mqtt_publish_card_pay(const char* card_id, uint8_t tap_index , const char* amount) {
    char tap_index_str[3];
    char mqttmessage[BUFFER_SIZE];
    sprintf(tap_index_str, "%d", tap_index); // Convert tap index to string
    //construct the MQTT topic and message payload
    snprintf(mqttmessage, BUFFER_SIZE, 
        "%s%s%s%s%s%s%s%s%s",
        "cardpay",delim,sim_imei,delim,card_id,delim,tap_index_str,delim, amount);
    publish_flag = mqtt_publish(topic_publish, mqttmessage);   
}
/**
 * @function to send card topup mqtt
 */
void mqtt_publish_card_topup(const char* card_id, const char* phone, const char* amount) {
    char mqttmessage[BUFFER_SIZE];
    //construct the MQTT topic and message payload
    snprintf(mqttmessage, BUFFER_SIZE, 
        "%s%s%s%s%s%s%s%s%s",
        "cardtopup",delim,sim_imei,delim, card_id,delim, phone,delim, amount);
    publish_flag = mqtt_publish(topic_publish, mqttmessage);   
}

/**
 * @callback function
 */

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    SerialMon.print(F("[MQTT] Received on "));
    SerialMon.print(topic);
    SerialMon.print(F(": "));
    SerialMon.println(message);

    // TODO: parse message and open/close taps accordingly
    // e.g. "open_1_100" → tap_open(1), set target_pulses
}