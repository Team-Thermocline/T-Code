#include <Arduino.h>

// The library is written in C, so ensure C linkage when included from C++.
extern "C" {
#include "tcode_protocol.h"
}

void setup() {
  Serial.begin(115200);

  // Minimal smoke test to ensure the library compiles and links.
  char line[] = "Q0*61"; // 'Q' ^ '0' == 0x61
  tcode_parsed_line_t parsed;
  tcode_status_t st = tcode_parse_inplace(line, &parsed);

  Serial.print(F("tcode_parse_inplace: "));
  Serial.println(tcode_status_str(st));
}

void loop() {
  // Intentionally empty.
}

