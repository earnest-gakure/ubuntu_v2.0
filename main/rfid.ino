/**
 * RFID.ino
 * This file contains the logic for handling RFID tag interactions, including reading tags, validating them against a list of authorized tags, and triggering tap control actions based on the tag data.
 */
#include "pinout.h"
#include <MFRC522.h>    

MFRC522 mfrc522(53, 34); // Create MFRC522 instance with SS pin 53 and RST pin 34
String scanned_tag_id = ""; // Variable to store the scanned RFID tag ID
bool tag_scanned = false; // Flag to indicate if a tag has been scanned
bool rfid_initiated = false; // Flag to indicate if RFID reader has been initialized

/**
 * @function to initialize the RFID reader          
 */
void rfid_init() {
    SPI.begin(); // Initialize SPI bus
    mfrc522.PCD_Init(); // Initialize the RFID reader
    Serial.println("RFID reader initialized");
}   

/**
 * @function to check for RFID tag presence and read its ID
 * @called from main loop to continuously check for tags    
 */
void check_for_rfid_tags() {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) { // Check if a new tag is present and can be read
        String tag_id = ""; // Initialize an empty string to store the tag ID
        for (byte i = 0; i < mfrc522.uid.size; i++) { // Loop through the UID bytes of the scanned tag
            tag_id += String(mfrc522.uid.uidByte[i], HEX); // Append each byte in hexadecimal format to the tag ID string
        }
        scanned_tag_id = tag_id; // Store the scanned tag ID in the global variable
        tag_scanned = true; // Set the flag to indicate that a tag has been scanned
        Serial.print("Scanned RFID Tag ID: ");
        Serial.println(scanned_tag_id); // Print the scanned tag ID to the serial monitor for debugging
    }
}


