/**
 * @file queue.ino
 * @brief transactions queue management
 * each trx has an id,
 * flow 
 * 1. gererate trx id
 * 2. publish trx to mqtt
 * 3. queue trx
 * 4. mqttcallback 
 * 5. queue_store_response
 * 6. process queue
 */
#include "pinout.h"
TransactionQueueEntry trx_queue[QUEUE_SIZE];
uint8_t queue_head = 0;
uint8_t queue_tail = 0;
uint8_t queue_count = 0;
String active_transaction_id[NUM_OF_TAPS] = {"", "", "", ""}; 


/**
 * @function to generate unique transaction ID
 * 
 */
void generate_txid(char *bu){
    
    //3upper case letters
    for(int i=0; i<3; i++){
        bu[i] = 'A' + random(0, 26);
    }
    //3 digits
    for(int i=3; i<6; i++){
        bu[i] = '0' + random(0, 10);
    }
    bu[6] = '\0'; // Null-terminate the string
}

/**
 * queue helpers
 */
bool queue_is_full() {
    return queue_count >= QUEUE_SIZE;
}

bool queue_is_empty() {
    return queue_count == 0;
}
/**
 * @function to add new pendoing transaction to the queue
 * returns true if added successfully, false if queue is full
 */
bool queue_enqueue(const char* trx_type, uint8_t tap_num, const char* txid) {
    if (queue_is_full()) {
        return false; // Queue is full
    }
    
    TransactionQueueEntry &entry = trx_queue[queue_tail];
    entry.state      = QSTATE_PENDING;
    entry.tap_number = tap_num;
    entry.timestamp  = millis();
    entry.timeout_ms = trxn_wait_time ;

    memset(entry.trx_type, 0, TRX_TYPE_LEN);
    strncpy(entry.trx_type, trx_type, TRX_TYPE_LEN - 1);

    memset(entry.txid, 0, TRXID_LEN);
    strncpy(entry.txid, txid, TRXID_LEN - 1);   // store the txid for matching

    memset(entry.payload, 0, BUFFER_SIZE);      // payload arrives later

    queue_tail  = (queue_tail + 1) % QUEUE_SIZE; // wrap back to 0
    queue_count++;

    Serial.print(F("Enqueued: "));
    Serial.print(entry.trx_type);
    Serial.print(F(" tap="));
    Serial.print(entry.tap_number);
    Serial.print(F(" txid="));
    Serial.println(entry.txid);

  return true;

}
/**
 * @function to store response
 * called from mqtt callback when server response arrives
 * finds the PENDING queue slot whose txid matches the txid extracted from payload
 * returns true if matching slot found and updated
 */
bool queue_store_response(const char* txid, const char* payload){
    for(uint8_t i=0; i < QUEUE_SIZE ; i++){
        TransactionQueueEntry &entry = trx_queue[i];
        if(entry.state == QSTATE_PENDING && strncmp(entry.txid, txid, TRXID_LEN) == 0){
            entry.state = QSTATE_RECEIVED;
            strncpy(entry.payload, payload, BUFFER_SIZE - 1);
            Serial.print(F("Response stored: txid="));
            Serial.print(txid);
            Serial.print(F(" tap="));
            Serial.println(entry.tap_number);

            return true;
        }
    }
    Serial.print(F("No matching queue entry for txid="));
    Serial.println(txid);
    return false;
}
/**
 * @function Removes ANY slot by index — not just the head.
 * called in queue_check_timeouts() and process_queue()
 */
void queue_dequeue_slot(uint8_t index) {
  if (trx_queue[index].state == QSTATE_EMPTY) return;

  memset(&trx_queue[index], 0, sizeof(TransactionQueueEntry));
  trx_queue[index].state = QSTATE_EMPTY;
  queue_count--;

  // If we just cleared the head, advance it
  if (index == queue_head) {
    queue_head = (queue_head + 1) % QUEUE_SIZE;
  }

  Serial.print(F("Slot "));
  Serial.print(index);
  Serial.println(F(" dequeued"));
}
/**
 * @function to qequeue expired slots and set tap free
 * called in loop  
 */
void queue_check_timeout(){
    for(uint8_t i=0; i < QUEUE_SIZE ; i++){
        TransactionQueueEntry &entry =trx_queue[i];
        if(entry.state == QSTATE_PENDING){
            if(millis() - entry.timestamp >= entry.timeout_ms){
                Serial.print(F("Timeout: "));
                Serial.print(entry.trx_type);
                Serial.print(F(" tap="));
                Serial.print(entry.tap_number);
                Serial.print(F(" txid="));
                Serial.println(entry.txid);

                uint8_t tap_num = entry.tap_number;
                taps[tap_num].pending_open = false;
                digitalWrite(taps[tap_num].led_pin, LOW);
                queue_dequeue_slot(i);
            }
        }
    }
}

/**
 * function to process responses
 * called in loop or STATE IDLE
 */
void process_queue(){
    for(uint8_t i = 0; i < QUEUE_SIZE ; i++){
        TransactionQueueEntry &entry = trx_queue[i];
        if(entry.state != QSTATE_RECEIVED)continue; //skip current slot

        Serial.print(F("Processing: "));
        Serial.print(entry.trx_type);
        Serial.print(F(" tap="));
        Serial.print(entry.tap_number);
        Serial.print(F(" txid="));
        Serial.println(entry.txid);

        String trx_type = String(entry.trx_type);
        uint8_t tap_num = entry.tap_number;
        char saved_txid[TRXID_LEN];
        strncpy(saved_txid, entry.txid, TRXID_LEN); // save before clearing slot

        //tokenise the stored payload
        char payload_copy[BUFFER_SIZE];
        strncpy(payload_copy, entry.payload, BUFFER_SIZE -1);

        char *token_ptrs[10];
        memset(token_ptrs, 0, sizeof(token_ptrs));
        uint8_t tc = 0;

        char *msg = strtok(payload_copy, "@");
        if(msg != NULL){
            char *tok = strtok(msg, "_");
            while (tok != NULL && tc < 10) {
                token_ptrs[tc++] = tok;
                tok = strtok(NULL, "_");
            }
        }

        //free the slot before acting
        queue_dequeue_slot(i);

        // ── MPESA PAY response ──
    // Expected tokens: mpesapay_IMEI_phone_amount_tap_txid_status_pulses
    //   [0]=mpesapay [1]=IMEI [2]=phone [3]=amount
    //   [4]=tap      [5]=txid [6]=status [7]=pulses
    if(trx_type == trxmpesapay){
        String trxstatus = token_ptrs[6];
        String checkpulses = token_ptrs[7];
        if(trxstatus == "1"){
            taps[tap_num].target_pulses = checkpulses.toInt();
            status_1_screen();
            active_transaction_id[tap_num]   = token_ptrs[5];
            active_transaction_type[tap_num] = token_ptrs[0];

            tap_open(tap_num);
            mqtt_dispense_start_ack(
                active_transaction_type[tap_num],   // "mpesapay"
                token_ptrs[4],                      // tap number string from payload
                taps[tap_num].target_pulses,        // uint32_t pulses
                active_transaction_id[tap_num]      // txid
            );
        }else{
            taps[tap_num].pending_open = false;
            if      (trxstatus == "2") { errorbeep(); status_2_screen(); }
            else if (trxstatus == "3") { errorbeep(); status_3_screen(); }
            else if (trxstatus == "4") { errorbeep(); status_4_screen(); }
            else if (trxstatus == "5") { errorbeep(); status_5_screen(); }
            else if (trxstatus == "6") { errorbeep(); status_6_screen(); }
            else if (trxstatus == "7") { errorbeep(); status_7_screen(); }
        }
    }
    // ── CARD PAY response ──
    // Expected tokens: cardpay_IMEI_cardID_tap_txid_balance_status_pulses
    //   [0]=cardpay [1]=IMEI [2]=cardID [3]=tap
    //   [4]=txid    [5]=balance [6]=status [7]=pulses
    else if(trx_type == trxcardpay){
        String cardbalance = token_ptrs[5];
        String card_status = token_ptrs[6];
        String checkpulses = token_ptrs[7];
        if(card_status == "1"){
            taps[tap_num].target_pulses = checkpulses.toInt();
            card_pay_success(cardbalance);
            active_transaction_id[tap_num]   = token_ptrs[4];
            active_transaction_type[tap_num] = token_ptrs[0];

            tap_open(tap_num);

            //Payload: "ack_IMEI_type_TXID_tap_start_targetPulses"
            //(String transactionType, String tapNumber,uint32_t targetPulses, String txid)
            mqtt_dispense_start_ack(
                    active_transaction_type[tap_num],
                    token_ptrs[3],
                    taps[tap_num].target_pulses,
                    active_transaction_id[tap_num]
                );
            tap_open(i);

        }
    }
    homescreen();
    //LOOK AT THIS LATER
   //************************************************* */
    return;
    }
}