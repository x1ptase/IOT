#include <Wire.h>
#include <RTClib.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// ===== LED MATRIX CONFIG =====
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 10
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// ===== BLUETOOTH CONFIG =====
#define BT_RX 2
#define BT_TX 3
SoftwareSerial BT(BT_RX, BT_TX);

// ===== RTC CONFIG =====
RTC_DS1307 rtc;

// ===== LM35 CONFIG =====
#define LM35_PIN A0

// ===== EEPROM ADDRESS =====
#define ADDR_MODE 0
#define ADDR_MSG 10
#define MSG_MAX_LEN 50

// ===== CONTROL VARIABLES =====
int mode = 1; // 1: Time | 2: Date | 3: Message | 4: Temperature
String customMessage = "HELLO FPT";
String btBuffer = "";

// ========== SETUP ==========
void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  // RTC setup
  if (!rtc.begin()) {
    Serial.println("Không tìm thấy DS1307!");
    while (1);
  }
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // LED setup
  matrix.begin();
  matrix.setIntensity(5);
  matrix.displayClear();

  // Load EEPROM
  loadEEPROM();

  Serial.println("=== SYSTEM READY ===");
  Serial.println("Bluetooth Commands:");
  Serial.println("1 → Show Time");
  Serial.println("2 → Show Date");
  Serial.println("3 + message → Show Custom Message (VD: 3HELLO)");
  Serial.println("4 → Show Temperature");
  Serial.println("S → Show EEPROM data");
}

// ========== MAIN LOOP ==========
void loop() {
  // Nhận dữ liệu Bluetooth
  while (BT.available()) {
    char c = BT.read();
    if (c == '\n' || c == '\r') continue;
    btBuffer += c;
    delay(5);
  }

  if (btBuffer.length() > 0) {
    Serial.print("Bluetooth gửi: ");
    Serial.println(btBuffer);

    char cmd = btBuffer.charAt(0);

    if (cmd == '1') {
      mode = 1;
      saveEEPROM();
    }
    else if (cmd == '2') {
      mode = 2;
      saveEEPROM();
    }
    else if (cmd == '3') {
      mode = 3;
      if (btBuffer.length() > 1) {
        customMessage = btBuffer.substring(1);
        customMessage.trim();
        customMessage.toUpperCase();
      }
      saveEEPROM();
    }
    else if (cmd == '4') {
      mode = 4;
      saveEEPROM();
    }
    else if (cmd == 'S' || cmd == 's') {
      showEEPROM();
    }

    btBuffer = "";
  }

  // Hiển thị theo mode
  switch (mode) {
    case 1: showTime(); break;
    case 2: showDate(); break;
    case 3: showMessage(customMessage); break;
    case 4: showTemperature(); break;
  }
}

// ========== EEPROM FUNCTIONS ==========
void saveEEPROM() {
  EEPROM.write(ADDR_MODE, mode);

  for (int i = 0; i < MSG_MAX_LEN; i++) {
    if (i < customMessage.length()) EEPROM.write(ADDR_MSG + i, customMessage[i]);
    else EEPROM.write(ADDR_MSG + i, 0);
  }

  Serial.println("EEPROM đã được lưu!");
}

void loadEEPROM() {
  mode = EEPROM.read(ADDR_MODE);
  if (mode < 1 || mode > 4) mode = 1;

  char msg[MSG_MAX_LEN];
  for (int i = 0; i < MSG_MAX_LEN; i++) {
    msg[i] = EEPROM.read(ADDR_MSG + i);
  }
  msg[MSG_MAX_LEN - 1] = '\0';
  customMessage = String(msg);
  customMessage.trim();

  Serial.print("Đã load EEPROM - Mode: ");
  Serial.print(mode);
  Serial.print(" | Msg: ");
  Serial.println(customMessage);
}

void showEEPROM() {
  Serial.println("==== EEPROM DATA ====");
  Serial.print("Mode: ");
  Serial.println(EEPROM.read(ADDR_MODE));
  Serial.print("Message: ");
  for (int i = 0; i < MSG_MAX_LEN; i++) {
    char c = EEPROM.read(ADDR_MSG + i);
    if (c == 0) break;
    Serial.print(c);
  }
  Serial.println();
  Serial.println("=====================");
}

// ========== HIỂN THỊ TRÁI → PHẢI ==========
void displayText(String text) {
  matrix.displayText(text.c_str(), PA_CENTER, 80, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!matrix.displayAnimate()) {}
}

// ========== HIỂN THỊ THỜI GIAN ==========
void showTime() {
  DateTime now = rtc.now();
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d", now.hour(), now.minute());
  displayText(String(timeStr));
}

// ========== HIỂN THỊ NGÀY ==========
void showDate() {
  DateTime now = rtc.now();
  char dateStr[11];
  sprintf(dateStr, "%02d/%02d/%02d", now.day(), now.month(), now.year() % 100);
  displayText(String(dateStr));
}

// ========== HIỂN THỊ THÔNG ĐIỆP ==========
void showMessage(String msg) {
  if (msg.length() > 20) msg = msg.substring(0, 20);
  displayText(msg);
}

// ========== HIỂN THỊ NHIỆT ĐỘ ==========
void showTemperature() {
  int analogValue = analogRead(LM35_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  int temperature = voltage * 100;

  char tempStr[16];
  sprintf(tempStr, "%d*C", temperature);
  displayText(String(tempStr));
}
