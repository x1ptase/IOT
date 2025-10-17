#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW   // Loại module LED
#define MAX_DEVICES 4                       // 4 module = 8x32
#define CS_PIN 10
#define DATA_PIN 11
#define CLK_PIN 13

// Khởi tạo đối tượng matrix
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Nội dung hiển thị
const char *text = "HELLO TUAN KIET 123 ";  // <-- đổi chữ ở đây

void setup() {
  matrix.begin();
  matrix.setIntensity(5);  // Độ sáng (0–15)
  matrix.displayClear();

  // Cấu hình hiệu ứng chạy chữ
  matrix.displayText(text, PA_CENTER, 80, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void loop() {
  if (matrix.displayAnimate()) {
    matrix.displayReset();  // Lặp lại hiệu ứng
  }
}