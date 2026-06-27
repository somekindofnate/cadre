#include <Arduino.h>
#include <WiFi.h>
#include "diag.h"

// ==========================================
// REALTEK C-HEADERS
// ==========================================
#ifdef __cplusplus
extern "C" {
#endif
  #include "wifi_conf.h"
  #include "wifi_util.h"
  #include "wifi_structures.h"
#ifdef __cplusplus
}
#endif

// --- TARGET OUI DATA ---
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

// --- 5GHZ WI-FI CONFIGURATION ---
const int channels5GHz[] = {36, 40, 44, 48, 149, 153, 157, 161};
const int numChannels = 8;
int currentChannelIndex = 0;
unsigned long lastChannelSwitch = 0;
const unsigned long CHANNEL_HOP_INTERVAL = 400; 

// --- PACKET STRUCT ---
struct RawPacketHeader {
  uint16_t frameControl;
  uint16_t duration;
  uint8_t dstMac[6];
  uint8_t srcMac[6];
  uint8_t bssId[6];
  uint16_t seqControl;
};

void rtkSnifferCallback(unsigned char* buf, unsigned int len, void* userdata) {
  (void)userdata; 
  if (len < 24 + 28) return; 
  unsigned char* framePayload = buf + 28; 
  RawPacketHeader* header = (RawPacketHeader*)framePayload;
  uint8_t* src = header->srcMac;

  for (int i = 0; i < NUM_OUIs; i++) {
    if (src[0] == KNOWN_DRONES[i].oui[0] && 
        src[1] == KNOWN_DRONES[i].oui[1] && 
        src[2] == KNOWN_DRONES[i].oui[2]) {
      
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
               src[0], src[1], src[2], src[3], src[4], src[5]);
      
      // SYNC ANCHOR: Must include $ for Hub parsing
      Serial1.print("$"); 
      Serial1.print(KNOWN_DRONES[i].brand);
      Serial1.print(",");
      Serial1.print(macStr);
      Serial1.print(",CH");
      Serial1.println(channels5GHz[currentChannelIndex]);
      break; 
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(4800); 

  wifi_off();
  delay(500);
  wifi_on(1);
  
  wifi_set_promisc(0, NULL, 0); 
  wifi_enter_promisc_mode();
  wifi_set_promisc(3, rtkSnifferCallback, 1); 
  wifi_set_channel(channels5GHz[currentChannelIndex]);
  
  Serial.println("BW16 5GHz Node Active.");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastChannelSwitch >= CHANNEL_HOP_INTERVAL) {
    lastChannelSwitch = currentMillis;
    currentChannelIndex = (currentChannelIndex + 1) % numChannels;
    wifi_set_channel(channels5GHz[currentChannelIndex]);
  }
}