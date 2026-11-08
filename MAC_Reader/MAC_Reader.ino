// Optional helper; the receiver sketch also prints this address.
#include <WiFi.h>
void setup() {
  Serial.begin(115200);
  delay(1500);
  WiFi.mode(WIFI_STA);
  delay(100);
}
void loop() {
  Serial.print("Receiver station MAC: "); Serial.println(WiFi.macAddress());
  delay(2000);
}
