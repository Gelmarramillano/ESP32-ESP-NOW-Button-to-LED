// Gelmar Tech Lab - ESP32-S3 transmitter
// Button: GPIO4 to GND; INPUT_PULLUP. Press = LED ON, release = OFF.
// Replace all six zero bytes with the receiver's STATION MAC before uploading.
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_arduino_version.h>

constexpr uint8_t CHANNEL = 1;
void stopWithError(const char *message) {
  Serial.println(message);
  while (true) delay(1000);
}
void startRadio() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  if (esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK)
    stopWithError("Cannot set channel");
  Serial.print("Station MAC: "); Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) stopWithError("ESP-NOW init failed");
}

const uint8_t RECEIVER_MAC[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
constexpr uint8_t BUTTON_PIN = 4;
bool stablePressed = false;
bool lastRaw = false;
uint32_t changedAt = 0;
uint32_t lastSend = 0;
bool sendNow = true;

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  startRadio();
  bool empty = true;
  for (uint8_t b : RECEIVER_MAC) if (b != 0) empty = false;
  if (empty) stopWithError("Enter your receiver MAC in RECEIVER_MAC first.");
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, RECEIVER_MAC, 6);
  peer.channel = CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) stopWithError("Cannot add receiver peer");
  Serial.println("Ready: press GPIO4 button.");
}
void loop() {
  uint32_t now = millis();
  bool raw = digitalRead(BUTTON_PIN) == LOW;
  if (raw != lastRaw) { lastRaw = raw; changedAt = now; }
  if (now - changedAt >= 30 && stablePressed != raw) {
    stablePressed = raw;
    sendNow = true;
  }
  // Repeat the state every 200 ms to recover from lost packets.
  if (sendNow || now - lastSend >= 200) {
    uint8_t state = stablePressed ? 1 : 0;
    esp_err_t result = esp_now_send(RECEIVER_MAC, &state, sizeof(state));
    Serial.printf("Button %s | queue: %s\n", state ? "PRESSED" : "RELEASED",
                  result == ESP_OK ? "OK" : "ERROR");
    // Queue OK means accepted for sending, not confirmation of reception.
    lastSend = now; sendNow = false;
  }
  delay(1);
}
