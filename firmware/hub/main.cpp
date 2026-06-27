#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Buzzer
const int buzzerPin = 8;

// ==========================================
// HELTEC V3 OLED CONFIGURATION
// ==========================================
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 21, /* clock=*/ 18, /* data=*/ 17);

// ==========================================
// THREAT DATABASE
// ==========================================
const int MAX_TARGETS = 4; 
const unsigned long TARGET_TIMEOUT = 10000; // Increased to 10s for stability

struct DroneTarget {
  String brand;
  String macShort;
  String channel;
  unsigned long lastSeen;
  bool active;
};

DroneTarget targets[MAX_TARGETS];

String shortenMac(String fullMac) {
  return (fullMac.length() >= 5) ? fullMac.substring(fullMac.length() - 5) : fullMac;
}

bool processThreatData(String data, unsigned long currentMillis) {
  data.trim();
  int firstComma = data.indexOf(',');
  int secondComma = data.lastIndexOf(',');
  
  if (firstComma == -1 || secondComma == -1 || firstComma == secondComma) return false;

  String brand = data.substring(0, firstComma);
  String fullMac = data.substring(firstComma + 1, secondComma);
  String channel = data.substring(secondComma + 1);
  String macShort = shortenMac(fullMac);

  // LOG FOR DEBUGGING
  Serial.println("INCOMING: " + brand + " | MAC: " + macShort);

  // Update existing
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (targets[i].active && targets[i].macShort == macShort) {
      targets[i].lastSeen = currentMillis;
      targets[i].channel = channel;
      return true;
    }
  }

  // Add new
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (!targets[i].active) {
      targets[i].brand = brand;
      targets[i].macShort = macShort;
      targets[i].channel = channel;
      targets[i].lastSeen = currentMillis;
      targets[i].active = true;
      Serial.println("DEBUG: New Target stored in slot " + String(i));
      return true;
    }
  }
  return false;
}

void drawMatrix() {
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, 128, 13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_ncenB08_tr); 
  u8g2.drawStr(2, 11, "CADRE");
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_4x6_tr); 
  u8g2.drawStr(2, 22, "TYPE");
  u8g2.drawStr(42, 22, "ID(L5)");
  u8g2.drawStr(98, 22, "CHAN");
  u8g2.drawLine(0, 25, 128, 25); 

  int yPos = 34;
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (targets[i].active) {
      u8g2.drawStr(2, yPos, targets[i].brand.c_str());
      u8g2.drawStr(42, yPos, targets[i].macShort.c_str());
      u8g2.drawStr(98, yPos, targets[i].channel.c_str());
    } else {
      u8g2.drawStr(2, yPos, "---");
      u8g2.drawStr(42, yPos, "---");
      u8g2.drawStr(98, yPos, "---");
    }
    yPos += 9; 
  }
  u8g2.sendBuffer();

  tone(buzzerPin, 1000);
  delay(250);
  noTone(buzzerPin);

  tone(buzzerPin, 1000);
  delay(250);
  noTone(buzzerPin);
}

void setup() {
  Serial.begin(115200);   
  
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW); delay(100);
  digitalWrite(21, HIGH); delay(100);
  
  // Initialize nodes: Alpha (5GHz) on 41, Bravo (2.4GHz) on 42
  Serial1.begin(4800, SERIAL_8N1, 41, -1); // Alpha (5GHz)
  Serial2.begin(4800, SERIAL_8N1, 42, -1); // Bravo (2.4GHz)

  // 1. HARDWARE POWER-ON (Vext Control)
  pinMode(36, OUTPUT);
  digitalWrite(36, LOW); 
  delay(100);

  // 2. I2C PIN INITIALIZATION
  Wire.begin(17, 18); 

  // 3. START DISPLAY
  u8g2.begin();

  // 4. MESHTASTIC LUMINANCE INJECTION
  u8g2.setContrast(255);        
  u8g2.sendF("ca", 0xD9, 0xF1); 
  u8g2.sendF("ca", 0xDB, 0x40); 
  
  for (int i = 0; i < MAX_TARGETS; i++) targets[i].active = false;
  drawMatrix();
  Serial.println("HUB READY. LISTENING ON GPIO 41 AND 42...");
}

void loop() {
  unsigned long currentMillis = millis();
  bool matrixChanged = false;

  // Process Alpha (5GHz) - Pin 41
  if (Serial1.available() > 0) {
    if (Serial1.read() == '$') {
      String data = Serial1.readStringUntil('\n');
      if (processThreatData(data, currentMillis)) {
        matrixChanged = true;
        Serial.println("DEBUG: Alpha Update");
      }
    }
  }

  // Process Bravo (2.4GHz) - Pin 42
  if (Serial2.available() > 0) {
    if (Serial2.read() == '$') {
      String data = Serial2.readStringUntil('\n');
      if (processThreatData(data, currentMillis)) {
        matrixChanged = true;
        Serial.println("DEBUG: Bravo Update");
      }
    }
  }

  // Aging cleanup
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (targets[i].active && (currentMillis - targets[i].lastSeen > TARGET_TIMEOUT)) {
      targets[i].active = false;
      matrixChanged = true; 
    }
  }

  if (matrixChanged) {
    drawMatrix();
  }
}