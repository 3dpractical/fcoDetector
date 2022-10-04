#include <Wire.h>
#include "Adafruit_FONA.h"

#define SIM800L_RX     27
#define SIM800L_TX     26
#define SIM800L_PWRKEY 4
#define SIM800L_RST    5
#define SIM800L_POWER  23

char replybuffer[255];

HardwareSerial *sim800lSerial = &Serial1;
Adafruit_FONA sim800l = Adafruit_FONA(SIM800L_PWRKEY);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

#define LED_BLUE  13

String smsString = "";

#define target_message  "+6283821919530"

void setup(){
  Wire.begin()
  pinMode(LED_BLUE, OUTPUT);
  pinMode(SIM800L_POWER, OUTPUT);

  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(SIM800L_POWER, HIGH);

  Serial.begin(115200);
  Serial.println(F("ESP32 with GSM SIM800L"));
  Serial.println(F("Initializing….(May take more than 10 seconds)"));
  
  delay(10000);

  // Make it slow so its easy to read!
  sim800lSerial->begin(4800, SERIAL_8N1, SIM800L_TX, SIM800L_RX);
  if (!sim800l.begin(*sim800lSerial)) {
    Serial.println(F("Couldn't find GSM SIM800L"));
    while (1);
  }
  Serial.println(F("GSM SIM800L is OK"));

  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = sim800l.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("SIM card IMEI: "); Serial.println(imei);
  }

  Serial.println("GSM SIM800L Ready");

  delay(1000);
  if (!sim800l.sendSMS(target_message, "Hello World!")) {
    Serial.println(F("Sent!"));
  } else {
    Serial.println(F("Failed!"));
  }  
}

char sim800lNotificationBuffer[64];          //for notifications from the FONA
char smsBuffer[250];

void loop(){
  char* bufPtr = sim800lNotificationBuffer;    //handy buffer pointer

  if (sim800l.available()) {
    int slot = 0; // this will be the slot number of the SMS
    int charCount = 0;

    // Read the notification into fonaInBuffer
    do {
      *bufPtr = sim800l.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (sim800l.available()) && (++charCount < (sizeof(sim800lNotificationBuffer)-1)));
    
    //Add a terminal NULL to the notification string
    *bufPtr = 0;

    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(sim800lNotificationBuffer, "+CMTI: \"SM\",%d", &slot)) {
      Serial.print("slot: "); Serial.println(slot);
      
      char callerIDbuffer[32];  //we'll store the SMS sender number in here
      
      // Retrieve SMS sender address/phone number.
      if (!sim800l.getSMSSender(slot, callerIDbuffer, 31)) {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);

      // Retrieve SMS value.
      uint16_t smslen;
      // Pass in buffer and max len!
      if (sim800l.readSMS(slot, smsBuffer, 250, &smslen)) {
        smsString = String(smsBuffer);
        Serial.println(smsString);
      }

      if (smsString == "ON") {
        Serial.println("Relay is activated.");
        digitalWrite(LED_BLUE, HIGH);
        delay(100);
      }
      else if (smsString == "OFF") {
        Serial.println("Relay is deactivated.");
        digitalWrite(LED_BLUE, LOW);
        delay(100);
      }

      if (sim800l.deleteSMS(slot)) {
        Serial.println(F("OK!"));
      }
      else {
        Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(slot);
        sim800l.print(F("AT+CMGD=?\r\n"));
      }
    }
  }
}
