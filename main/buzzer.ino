/**
 * @file buzzer.ino
 * @brief This file contains the logic for controlling the buzzer, including functions to play different tones for various events such as tap opening, closing, and errors.
 */
#include "pinout.h"

void buzzerinit() {
  pinMode(buzzer_pin, OUTPUT);
}


void keypadbuzz() {
  digitalWrite(buzzer_pin, HIGH);
  delay(3);
  digitalWrite(buzzer_pin, LOW);
  delay(3);
}

void GSM_Net_Connect_Buzz() {  //Indicate that a connection to the GSM network has been established
  digitalWrite(buzzer_pin, HIGH);    // turn the LED on (HIGH is the voltage level)
  delay(50);                   // wait for a second
  digitalWrite(buzzer_pin, LOW);     // turn the LED off by making the voltage LOW
  delay(50);
}
// Double beep — GPRS attached
void GSM_GPRS_Connect_Buzz() {
        digitalWrite(buzzer_pin, HIGH); delay(50);
        digitalWrite(buzzer_pin, LOW);  delay(50);

}
void Mqtt_Broker_Connect_Buzz() {  //Indicate that a connection to the broker has been established
  for (int x = 0; x < 3; x++) {
    digitalWrite(buzzer_pin, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(50);                 // wait for a second
    digitalWrite(buzzer_pin, LOW);   // turn the LED off by making the voltage LOW
    delay(50);
  }
}

void successbeep() {
  for (int x = 0; x < 2; x++) {
    digitalWrite(buzzer_pin, HIGH);
    delay(40);
    digitalWrite(buzzer_pin, LOW);
    delay(20);
  }
}
void errorbeep() {
  for (int x = 0; x < 3; x++) {
    digitalWrite(buzzer_pin, HIGH);
    delay(40);
    digitalWrite(buzzer_pin, LOW);
    delay(20);
  }
}
