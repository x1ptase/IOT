#include <Wire.h>
#include <RTClib.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <SoftwareSerial.h>

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
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

String receivedData = "";
bool showTime = true;
bool showTemperature = false;
bool showText = false;
String customMessage = "";

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);

  // RTC setup
  if (!rtc.begin()) {
    Serial.println(F("RTC not found!"));
  }
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Matrix setup
  matrix.begin();
  matrix.setIntensity(5);
  matrix.displayClear();

  Serial.println(F("Ready..."));
  bluetooth.println(F("Ready..."));
}

void loop() {
  // Nhận lệnh từ Bluetooth
  while (bluetooth.available()) {
    char c = bluetooth.read();
    receivedData += c;
    delay(2);
  }

  if (receivedData.length() > 0) {
    processCommand(receivedData);
    receivedData = "";
  }

  if (showTime) displayTime();
  if (showTemperature) displayTemperature();
  if (showText) displayCustomMessage();
}

// =======================
// Xử lý lệnh Bluetooth
// =======================
void processCommand(String command) {
  if (command.startsWith("MODE1")) {
    showTime = true;
    showTemperature = false;
    showText = false;
  }
  else if (command.startsWith("MODE2")) {
    showTime = false;
    showTemperature = true;
    showText = false;
  }
  else if (command.startsWith("MODE3")) {
    customMessage = command.substring(5);
    customMessage.trim();
    showTime = false;
    showTemperature = false;
    showText = true;
  }
}

// =======================
// Hiển thị thời gian
// =======================
void displayTime() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    DateTime now = rtc.now();
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d", now.hour(), now.minute());

    matrix.displayText(timeStr, PA_CENTER, 80, 0, PA_PRINT, PA_NO_EFFECT);
    while (!matrix.displayAnimate()) {}
  }
}

// =======================
// Hiển thị nhiệt độ LM35
// =======================
void displayTemperature() {
  int analogValue = analogRead(LM35_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  int tempC = voltage * 100;

  char tempStr[10];
  sprintf(tempStr, "T:%dC", tempC);

  matrix.displayText(tempStr, PA_CENTER, 80, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!matrix.displayAnimate()) {}
  delay(1000);
}

// =======================
// Hiển thị text tùy chỉnh
// =======================
void displayCustomMessage() {
  matrix.displayText(customMessage.c_str(), PA_CENTER, 80, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!matrix.displayAnimate()) {}
  delay(1000);
}
