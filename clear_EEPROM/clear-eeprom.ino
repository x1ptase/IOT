#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  
  int bytes_cleaned = 0;
  
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    // Kiểm tra nếu ô nhớ hiện tại KHÔNG phải là 0
    if (EEPROM.read(i) != 0) {
      EEPROM.write(i, 0); // Chỉ ghi 0 nếu cần
      bytes_cleaned++;
    }
  }
  
  Serial.print("Da kiem tra EEPROM. Tong so byte da xoa (ghi de bang 0): ");
  Serial.println(bytes_cleaned);
  
  // Dừng lại sau khi xóa
  while (true); 
}

void loop() {
  // Không làm gì
}