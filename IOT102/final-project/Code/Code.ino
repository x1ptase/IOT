#include<Wire.h>
#include<RTClib.h>
#include<MD_Parola.h>
#include<MD_MAX72XX.h>
#include<SPI.h>
#include<SoftwareSerial.h>
#include<EEPROM.h>  // Include EEPROM library

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 10
#define LM35_PIN A0

#define BT_TX 3
#define BT_RX 2
SoftwareSerial bluetooth(BT_RX, BT_TX);

RTC_DS1307 rtc;
MD_Parola matrix=MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

String receivedData="";
bool showTime=true, showTemperature=false, showDate=false;
String customMessage="";
int directionRun=1;

// EEPROM addresses
#define EEPROM_MODE_ADDR 0       // EEPROM Address to store mode
#define EEPROM_MSG_ADDR 1        // EEPROM Address for custom message
#define EEPROM_DIR_ADDR 2
#define MAX_MSG_LENGTH 50        // Max length of message

void setup(){
  Serial.begin(9600);
  bluetooth.begin(9600);

  if(!rtc.begin())
    if(!rtc.isrunning()){
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

  matrix.begin();
  matrix.setIntensity(5);
  matrix.displayClear();

  // Load mode and message from EEPROM
  loadModeFromEEPROM();
}

void loop(){
  while(bluetooth.available()){
    char c=bluetooth.read();
    if(c == '\n'){
      receivedData="";
    } else{
      receivedData += c;
      processCommand(receivedData);
    }
  }

  receivedData="";

  if(showTime) displayTime();
  if(showTemperature) displayTemperature();
  if(showDate) displayDate();
  if(customMessage != "") displayCustomMessage();
}

// ===========================
// Function to process commands
// ===========================
void processCommand(String command) {
  if (command == "MODE1") {
    showTime = true;
    showTemperature = false;
    showDate = false;
    customMessage = "";
    saveModeToEEPROM(1, customMessage);
  } else if (command == "MODE2") {
    showTime = false;
    showTemperature = true;
    showDate = false;
    customMessage = "";
    saveModeToEEPROM(2, customMessage);
  } else if (command.indexOf("MODE3") != -1) {
    showTime = false;
    showTemperature = false;
    showDate = false;
    int index = command.indexOf("MODE3") + 5;
    if (index < command.length()) {
      customMessage = command.substring(index);
      customMessage.trim();
      saveModeToEEPROM(3, customMessage);
    }
  } else if (command.indexOf("MODE4") != -1) {
    showTime = true;
    showTemperature = false;
    showDate = false;
    int index = command.indexOf("MODE4") + 5;

    String hour = command.substring(index, index + 2);
    String min = command.substring(index + 3, index + 5);

    DateTime now = rtc.now();
    int receivedHour = hour.toInt();
    int receivedMin = min.toInt();

    // Check if the received time is different from the current RTC time
    if (now.hour() != receivedHour || now.minute() != receivedMin) {
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), receivedHour, receivedMin, 0));
      Serial.println("RTC Updated!");
      bluetooth.println("RTC Updated!");
    } else {
      Serial.println("RTC Already Correct!");
      bluetooth.println("RTC Already Correct!");
    }

    customMessage = "";
    saveModeToEEPROM(4, customMessage);
  } else if (command == "MODE5") {
    showTime = false;
    showTemperature = false;
    showDate = true;
    customMessage = "";
    saveModeToEEPROM(5, customMessage);
  }
  else if (command.indexOf("MODES") != -1) {
    int index = command.indexOf("MODES") + 5;
    directionRun = command.substring(index, index + 1).toInt();
  }
}

// ===========================
// Function to display time
// ===========================
void displayTime() {
  static unsigned long lastSent = 0;
  unsigned long nowMillis = millis();

  if (nowMillis - lastSent >= 1000) {
    lastSent = nowMillis;

    DateTime now = rtc.now();
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d", now.hour(), now.minute());

    displayText(timeStr);
    while (!matrix.displayAnimate()) {}
  }
}

// ===========================
// Function to display temperature
// ===========================
void displayTemperature() {
  int analogValue = analogRead(LM35_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  int temp = voltage * 100;

  char temperatureStr[16];
  snprintf(temperatureStr, sizeof(temperatureStr), "T:%dC", temp);

  displayText(temperatureStr);
  while (!matrix.displayAnimate()) {}
  delay(3000);
}

// ===========================
// Function to display message
// ===========================
void displayCustomMessage() {
  matrix.displayClear();

  if (directionRun == 5) {
    matrix.setTextAlignment(PA_CENTER);
    matrix.print(customMessage.c_str());
  } else {
    matrix.displayText(customMessage.c_str(), PA_CENTER, 80, 0, getScrollEffect(), getScrollEffect());
    while (!matrix.displayAnimate()) {}
  }
  delay(3000);
}

// ===========================
// Function to display date
// ===========================
void displayDate() {
  static unsigned long lastSent = 0;
  unsigned long nowMillis = millis();

  if (nowMillis - lastSent >= 1000) {
    lastSent = nowMillis;

    DateTime now = rtc.now();
    char dateStr[16];
    sprintf(dateStr, "%02d/%02d", now.day(), now.month());

    displayText(dateStr);
    while (!matrix.displayAnimate()) {}
  }
  delay(3000);
}

textEffect_t getScrollEffect() {
  switch (directionRun) {
    case 1: return PA_SCROLL_LEFT;
    case 2: return PA_SCROLL_RIGHT;
    case 3: return PA_SCROLL_UP;
    case 4: return PA_SCROLL_DOWN;
    case 5: return PA_PRINT;
    default: return PA_SCROLL_LEFT;
  }
}

void displayText(String text) {
  matrix.displayClear();

  if (directionRun == 5) {
    matrix.setTextAlignment(PA_CENTER);
    matrix.print(text.c_str());
  } else {
    matrix.displayText(text.c_str(), PA_CENTER, 80, 0, getScrollEffect(), getScrollEffect());
    while (!matrix.displayAnimate()) {}
  }
}

// ===========================
// Function to save mode & message to EEPROM
// ===========================
void saveModeToEEPROM(int mode, String message) {
  EEPROM.write(EEPROM_MODE_ADDR, mode);
  for (int i = 0; i < MAX_MSG_LENGTH; i++) {
    EEPROM.write(EEPROM_MSG_ADDR + i, i < message.length() ? message[i] : '\0');
  }
  EEPROM.write(EEPROM_DIR_ADDR, directionRun);
}

// ===========================
// Function to load mode & message from EEPROM
// ===========================
void loadModeFromEEPROM() {
  int mode = EEPROM.read(EEPROM_MODE_ADDR);
  char message[MAX_MSG_LENGTH];
  for (int i = 0; i < MAX_MSG_LENGTH; i++) {
    message[i] = EEPROM.read(EEPROM_MSG_ADDR + i);
  }
  message[MAX_MSG_LENGTH - 1] = '\0';
  customMessage = String(message);

  directionRun = EEPROM.read(EEPROM_DIR_ADDR);

  showTime = (mode == 1 || mode == 4);
  showTemperature = (mode == 2);
  showDate = (mode == 5);
}
