#include   "pinout.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

void lcd_init() {
    // Initialize the LCD with the I2C address and dimensions
    lcd.init();
    lcd.backlight(); // Turn on the backlight
}

void lcddisplay(String firstline, String secondline, String thirdline, String fourthline) {
  //This is a generic function that allows you to print messages on the screen on a respective line
  lcd.setCursor(0, 0);
  //lcd.clear();
  if (firstline == "") {
    lcd.print(" ");
  } else {
    lcd.print(firstline);
  }

  lcd.setCursor(0, 1);
  if (secondline == "") {
    lcd.print("");
  } else {
    lcd.print(secondline);
  }

  lcd.setCursor(0, 2);
  if (thirdline == "") {
    lcd.print("");
  } else {
    lcd.print(thirdline);
  }

  lcd.setCursor(0, 3);
  if (fourthline == "") {
    lcd.print("");
  } else {
    lcd.print(fourthline);
  }
}

/**
 * welcome screen animation
 */
void welcome_screen_animation() {
  int i=0;
  int j=0;
  int randomnum;
  int z=0;
  int k=0;
  int array1[] = { 5,  5,  3,  4,  1,  2,  5,  5, 5,  5,  3,  4,  1,  2,  5,  5,  5,  5,  3,  4,  1,  2,  5,  5, 5,  5,  3,  4,  1,  2,  5,  5 };
  int array2[] = { 3,  4,  0,  0,  0,  0,  1,  2, 3,  4,  0,  0,  0,  0,  1,  2,  3,  4,  0,  0,  0,  0,  1,  2, 3,  4,  0,  0,  0,  0,  1,  2 };
  byte customChar1[] = {
    0x1B,
    0x1B,
    0x1B,
    0x1B,
    0x1B,
    0x1B,
    0x1B,
    0x1B
  };

  byte customChar2[] = {
    0x18,
    0x18,
    0x1B,
    0x1B,
    0x1B,
    0x1B,
    0x1B,
    0x1B
  };
  byte customChar3[] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x18,
    0x18,
    0x1B,
    0x1B
  };

  byte customChar4[] = {
  0x00,
    0x00,
    0x00,
    0x00,
    0x03,
    0x03,
    0x1B,
    0x1B
  };

  byte customChar5[] = {
  0x03,
    0x03,
    0x1B,
    0x1B,
    0x1B,
    0x1B,
    0x1B,
    0x1B
  };

    byte customChar6[] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
  };

  lcd.createChar(0, customChar1);
  lcd.createChar(1, customChar2);
  lcd.createChar(2, customChar3);
  lcd.createChar(3, customChar4);
  lcd.createChar(4, customChar5);
  lcd.createChar(5, customChar6);

  lcd.clear();


  lcd.setCursor(0,2);
  lcd.print("  UBUNTU WATERHUB");
  lcd.setCursor(0,3);
  lcd.print("        AFRICA");
  delay(200);



  for (z=7 ; z>=0 ;z--) { 
    i=z;
    for(k=0 ;k<20;k++) {
      lcd.setCursor(k,0);
      lcd.write(array1[i]);
      lcd.setCursor(k,1);
      lcd.write(array2[i]);
      i++;
    }
    delay(10);
  }

  delay(2000);


}

void loading_screen() {
  lcd.clear();
  lcddisplay("System Loading", "", "Please Wait... ", "");

  lcd.setCursor(14, 2);
  lcd.blink_on();
}

void homescreen() {
  lcd.blink_off();

  lcd.clear();
  lcddisplay("Tap Tag", "", "Enter Phone Number", "");
}


//  //error specific display 
// ===== TRANSACTION STATUS SCREENS =====

void status_1_screen() {
  lcd.clear();
  lcddisplay("", "", "SUCCESSFUL", "");
  delay(2000);
}

void status_2_screen() {
  lcd.clear();
  lcddisplay("", "STK REQUEST FAILED", "TRY AGAIN...", "");
  delay(5000);
}

void status_3_screen() {
  lcd.clear();
  lcddisplay("", "TRANSACTION HAS BEEN", "  CANCELLED", "");
  delay(5000);
}

void status_4_screen() {
  lcd.clear();
  lcddisplay("", "WRONG AMOUNT", "PLEASE TRY AGAIN...", "");
  delay(5000);
}

void status_5_screen() {
  lcd.clear();
  lcddisplay("", "LOW MPESA BALANCE", "PLEASE TOP UP !", "");
  delay(5000);
}

void status_6_screen() {
  lcd.clear();
  lcddisplay("", "SYSTEM OFFLINE", "TRY AGAIN....", "");
  delay(5000);
}

void status_7_screen() {
  lcd.clear();
  lcddisplay("", "WRONG MPESA PIN", "PLEASE TRY AGAIN...", "");
  delay(5000);
}

void status_8_screen() {
  lcd.clear();
  lcddisplay("", "ENTER CORRECT AMOUNT", "PLEASE TRY AGAIN...", "");
  delay(5000);
}

// //------------**********----------------//
// //------------ TAG SCREENS ---------------//
// //------------**********----------------//

void tagID_display_screen() {
  lcd.clear();
  //lcddisplay("ID: " + get_cardid(), "", "", "");
  delay(2500);
}
void card_pay_success(String cardbalance) {
  lcd.clear();
  lcddisplay("", "SUCCESSFUL", "TOKENS BAL: " + cardbalance, "");
  delay(3000);
}
void card_topup_success(String cardbalance) {
  lcd.clear();
  lcddisplay("", "TOP UP SUCCESS", "TOKENS BAL: " + cardbalance, "");
  delay(3000);
}
