#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ==========================================
// TARGET OUI DATA
// ==========================================
struct DroneOUI {
  uint8_t oui[3];
  const char* brand;
};

const DroneOUI KNOWN_DRONES[] = {
  // --- DJI (Massive Allocation Block) ---
  {{0x04, 0xA8, 0x5A}, "DJI"},
  {{0x0C, 0x9A, 0xE6}, "DJI"},
  {{0x34, 0xD2, 0x62}, "DJI"},
  {{0x48, 0x1C, 0xB9}, "DJI"},
  {{0x4C, 0x43, 0xF6}, "DJI"},
  {{0x58, 0xB8, 0x58}, "DJI"},
  {{0x60, 0x60, 0x1F}, "DJI"},
  {{0x88, 0x29, 0x85}, "DJI"},
  {{0x8C, 0x58, 0x23}, "DJI"},
  {{0xE4, 0x7A, 0x2C}, "DJI"},
  {{0xDC, 0x0E, 0xA1}, "DJI (Compal)"},
  
  // --- Autel Robotics ---
  {{0xEC, 0x5B, 0xCD}, "Autel"},
  
  // --- Parrot ---
  {{0x90, 0x03, 0xB7}, "Parrot"},
  {{0x00, 0x26, 0x7E}, "Parrot"},
  {{0x00, 0x12, 0x1C}, "Parrot"},
  {{0xA0, 0x14, 0x3D}, "Parrot"},
  
  // --- Skydio ---
  {{0x38, 0x1D, 0x14}, "Skydio"}, 

  // --- Common FPV / Cheap Toy Drones ---
  {{0x4C, 0x0F, 0xC7}, "Sky Rider"},
  {{0x08, 0xEA, 0x40}, "Generic FPV"}, 
  {{0xE0, 0xB9, 0x4D}, "Generic FPV"}  
};
const int NUM_OUIs = sizeof(KNOWN_DRONES) / sizeof(KNOWN_DRONES[0]);

// ==========================================
// UART CONFIGURATION
// ==========================================
HardwareSerial CADRE_Serial(1); // Use Serial1
#define TX_PIN D7               // Ensure this matches your wiring

void sniffCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* frame = pkt->payload;
  
  // 802.11 Management frames (Beacons) are usually at offset 0 in the payload
  // MAC Source is at offset 10 of the frame
  uint8_t* src = frame + 10;

  for (int i = 0; i < NUM_OUIs; i++) {
    if (src[0] == KNOWN_DRONES[i].oui[0] && 
        src[1] == KNOWN_DRONES[i].oui[1] && 
        src[2] == KNOWN_DRONES[i].oui[2]) {
      
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
               src[0], src[1], src[2], src[3], src[4], src[5]);

      // Hub Parsing Format: $Brand,MAC,Channel
      CADRE_Serial.print("$");
      CADRE_Serial.print(KNOWN_DRONES[i].brand);
      CADRE_Serial.print(",");
      CADRE_Serial.print(macStr); // Can be shortened in Hub
      CADRE_Serial.print(",CH");
      CADRE_Serial.println(WiFi.channel());
      
      // Debounce: Prevent the Hub from locking up
      delay(300); 
      break; 
    }
  }
}

void setup() {
  Serial.begin(115200);
  CADRE_Serial.begin(4800, SERIAL_8N1, -1, TX_PIN);

  // Initialize Wi-Fi
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();

  // Use the native ESP-IDF function
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffCallback);

  wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
  };
  esp_wifi_set_promiscuous_filter(&filter);
  
  Serial.println("XIAO 2.4GHz Node Active.");
}

void loop() {
  // Hopping 1, 6, 11 (Standard Drone Control Channels)
  static int channels[] = {1, 6, 11};
  static int i = 0;
  
  esp_wifi_set_channel(channels[i], WIFI_SECOND_CHAN_NONE);
  i = (i + 1) % 3;
  
  delay(1000); // Dwell time per channel
}