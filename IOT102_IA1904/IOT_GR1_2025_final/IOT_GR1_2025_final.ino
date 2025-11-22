#include <Wire.h>
#include <RTClib.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 10
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

#define BT_RX 2
#define BT_TX 3
SoftwareSerial BT(BT_RX, BT_TX);

RTC_DS1307 rtc;
#define LM35_PIN A0

#define ADDR_MODE 0
#define ADDR_EFFECT 1
#define ADDR_MSG 10
#define MSG_MAX_LEN 50

int mode = 1;
int effectMode = 0;
String customMessage = "HELLO FPT";
String btBuffer = "";
unsigned long lastUpdate = 0;

int manualHour = 0;
int manualMinute = 0;

void setup() {
  Serial.begin(9600);
  BT.begin(9600);
  if (!rtc.begin()) while (1);
  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  matrix.begin();
  matrix.setIntensity(5);
  matrix.displayClear();
  matrix.setFont(nullptr);
  loadEEPROM();
}

void loop() {
  DateTime now = rtc.now();
  int analogValue = analogRead(LM35_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  int temperature = voltage * 100;

  while (BT.available()) {
    char c = BT.read();
    if (c != '\n' && c != '\r') btBuffer += c;
  }

  if (btBuffer.length() > 0) {
    char cmd = btBuffer.charAt(0);
    if (cmd == 'T') {
      int year = btBuffer.substring(1, 5).toInt();
      int month = btBuffer.substring(6, 8).toInt();
      int day = btBuffer.substring(9, 11).toInt();
      int hour = btBuffer.substring(12, 14).toInt();
      int minute = btBuffer.substring(15, 17).toInt();
      rtc.adjust(DateTime(year, month, day, hour, minute, 0));
    } else {
      switch (cmd) {
        case '1': mode = 1; saveEEPROM(); break;
        case '2': mode = 2; saveEEPROM(); break;
        case '3': mode = 3; customMessage = btBuffer.substring(1); customMessage.trim(); customMessage.toUpperCase(); saveEEPROM(); break;
        case '4': mode = 4; saveEEPROM(); break;
        case '5': mode = 5; { 
          DateTime n = rtc.now();
          manualHour = n.hour();
          manualMinute = n.minute();
          saveEEPROM();
        } break;
        case 'L': effectMode = 0; saveEEPROM(); break;
        case 'R': effectMode = 1; saveEEPROM(); break;
        case 'U': effectMode = 2; saveEEPROM(); break;
        case 'D': effectMode = 3; saveEEPROM(); break;
        case '+': if (mode == 5) manualHour = (manualHour + 1) % 24; break;
        case '-': if (mode == 5) manualHour = (manualHour + 23) % 24; break;
        case '>': if (mode == 5) manualMinute = (manualMinute + 1) % 60; break;
        case '<': if (mode == 5) manualMinute = (manualMinute + 59) % 60; break;
        case 'O': if (mode == 5) {  
          DateTime n = rtc.now();
          rtc.adjust(DateTime(n.year(), n.month(), n.day(), manualHour, manualMinute, 0));
          mode = 1;
          saveEEPROM();
        } break;
      }
    }
    btBuffer = "";
  }

  unsigned long nowMs = millis();
  switch (mode) {
    case 1: if (nowMs - lastUpdate > 1000) { showTime(); lastUpdate = nowMs; } break;
    case 2: if (nowMs - lastUpdate > 2000) { showDate(); lastUpdate = nowMs; } break;
    case 3: showMessage(customMessage); break;
    case 4: if (nowMs - lastUpdate > 2000) { showTemperature(); lastUpdate = nowMs; } break;
    case 5: showManualAdjust(); delay(400); break;
  }
}

void saveEEPROM() {
  EEPROM.write(ADDR_MODE, mode);
  EEPROM.write(ADDR_EFFECT, effectMode);
  for (int i = 0; i < MSG_MAX_LEN; i++)
    EEPROM.write(ADDR_MSG + i, (i < customMessage.length()) ? customMessage[i] : 0);
}

void loadEEPROM() {
  mode = EEPROM.read(ADDR_MODE);
  effectMode = EEPROM.read(ADDR_EFFECT);
  char msg[MSG_MAX_LEN];
  for (int i = 0; i < MSG_MAX_LEN; i++) msg[i] = EEPROM.read(ADDR_MSG + i);
  msg[MSG_MAX_LEN - 1] = '\0';
  customMessage = String(msg);
  customMessage.trim();
}

void displayText(String text) {
  textEffect_t effect = PA_SCROLL_LEFT;
  int speed = 80, pause = 0;
  switch (effectMode) {
    case 0: effect = PA_SCROLL_RIGHT; break;
    case 1: effect = PA_SCROLL_LEFT; break;
    case 2: effect = PA_SCROLL_UP; break;
    case 3: effect = PA_SCROLL_DOWN; break;
  }
  matrix.displayText(text.c_str(), PA_CENTER, speed, pause, effect, effect);
  while (!matrix.displayAnimate());
}

void showTime() {
  DateTime now = rtc.now();
  char buffer[6];
  sprintf(buffer, "%02d:%02d", now.hour(), now.minute());
  displayText(buffer);
}

void showDate() {
  DateTime now = rtc.now();
  char buffer[11];
  sprintf(buffer, "%02d/%02d/%02d", now.day(), now.month(), now.year() % 100);
  displayText(buffer);
}

void showMessage(String msg) {
  msg.trim();
  msg.toUpperCase();
  if (effectMode == 2 || effectMode == 3) {
    int totalCols = MAX_DEVICES * 8;
    const int approxCharWidth = 6;
    int maxCharsPerLine = totalCols / approxCharWidth;
    if (maxCharsPerLine < 1) maxCharsPerLine = 1;
    const int padSpaces = 2;
    msg += " ";
    int n = msg.length();
    int pos = 0;
    while (pos < n) {
      while (pos < n && msg.charAt(pos) == ' ') pos++;
      if (pos >= n) break;
      int end = pos + maxCharsPerLine;
      if (end > n) end = n;
      int lastSpace = -1;
      for (int i = pos; i < end; i++) if (msg.charAt(i) == ' ') lastSpace = i;
      String part;
      if (end >= n) {
        part = msg.substring(pos, n);
        pos = n;
      } else if (msg.charAt(end) == ' ') {
        part = msg.substring(pos, end);
        pos = end + 1;
      } else if (lastSpace != -1) {
        part = msg.substring(pos, lastSpace);
        pos = lastSpace + 1;
      } else {
        part = msg.substring(pos, end);
        pos = end;
      }
      part.trim();
      if (pos < n) for (int s = 0; s < padSpaces; s++) part += " ";
      matrix.displayClear();
      matrix.displayReset();
      textEffect_t eff = (effectMode == 2) ? PA_SCROLL_UP : PA_SCROLL_DOWN;
      matrix.displayText(part.c_str(), PA_LEFT, 150, 0, eff, eff);
      while (!matrix.displayAnimate());
      delay(220);
    }
  } else {
    displayText(msg);
  }
}

void showTemperature() {
  int analogValue = analogRead(LM35_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  int temperature = voltage * 100;
  char buffer[10];
  sprintf(buffer, "%d*C", temperature);
  displayText(buffer);
}

void showManualAdjust() {
  char buffer[10];
  sprintf(buffer, "%02d:%02d", manualHour, manualMinute);
  displayText(buffer);
}

