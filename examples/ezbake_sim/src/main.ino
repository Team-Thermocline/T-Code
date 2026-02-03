#include <Arduino.h>

void setup() {
  delay(1500);

  // Configure Pins
  pinMode(LED_BUILTIN, OUTPUT);


  // Begin Serial
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  digitalWrite(LED_BUILTIN, HIGH);

  while (!Serial) {
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
  }
  
  Serial.flush();
}

void loop() {
  if (Serial.available()) {
    int c = Serial.read();
    // Test: send ? and we reply "OK" (no echo). If "OK" is clean, the bug is in webserial.io send/echo path.
    if (c == '?' || c == 'Q') {
      Serial.println(F("OK"));
    } else {
      Serial.write((uint8_t)c);
    }
  }
}
