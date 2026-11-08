// Gelmar Tech Lab - ESP32-WROOM-32 receiver (ESP32 Dev Module)
// GPIO23 -> 330 ohm resistor -> LED anode; LED cathode -> GND.
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

constexpr uint8_t LED_PIN = 23;
constexpr uint32_t TIMEOUT_MS = 1500;
portMUX_TYPE packetMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint8_t receivedState = 0;
volatile uint32_t receivedAt = 0;
volatile bool havePacket = false;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
#else
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
#endif
  if (len != 1 || data[0] > 1) return;
  portENTER_CRITICAL(&packetMux);
  receivedState = data[0];
  receivedAt = millis();
  havePacket = true;
  portEXIT_CRITICAL(&packetMux);
}
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  startRadio();
  if (esp_now_register_recv_cb(onReceive) != ESP_OK)
    stopWithError("Cannot register receive callback");
  Serial.println("Copy the Station MAC above into the transmitter sketch.");
}
void loop() {
  portENTER_CRITICAL(&packetMux);
  uint8_t state = receivedState;
  uint32_t at = receivedAt;
  bool valid = havePacket;
  portEXIT_CRITICAL(&packetMux);
  bool on = valid && (uint32_t)(millis() - at) < TIMEOUT_MS && state == 1;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  static int previous = -1;
  if (previous != (int)on) {
    Serial.println(on ? "LED ON" : "LED OFF");
    previous = on;
  }
  delay(1);
}
