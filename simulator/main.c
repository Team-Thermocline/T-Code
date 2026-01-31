#include "hardware/gpio.h"
#include "neopixel_ws2812.h"
#include "pico/error.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/time.h"
#include "pindefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool ENABLE_ECHO = false;

// NeoPixel (WS2812) config
static const float NEOPIXEL_FREQ_HZ = 800000.0f;

int process_line(char *line) {
  // Line number (optional)
  int line_number = 0;

  // Split the line into segments
  char *segments[32];    // Up to 32 segments
  int segment_count = 0; // The total number of segments
  int cur_segment = 0;   // The segment we're currently processing

  // Checksum
  // Check for checksum delimeter '*'
  char *checksum_ptr = strrchr(line, '*');
  if (checksum_ptr != NULL && strlen(checksum_ptr + 1) >= 2) {
    // Copy the hex digits
    char checksum_hex[3] = {0};
    checksum_hex[0] = checksum_ptr[1];
    checksum_hex[1] = checksum_ptr[2];

    // Convert the checksum hex to integer
    unsigned int given_checksum = 0;
    sscanf(checksum_hex, "%2x", &given_checksum);

    // Place a null pointer at the checksum delim (truncate the line here)
    *checksum_ptr = '\0';

    // Calculate our checksum over the line (up to but NOT including '*'),
    // simple xor of all chars
    unsigned int calculated_checksum = 0;
    for (char *p = line; *p; ++p) {
      calculated_checksum ^= (unsigned char)(*p);
    }

    // Compare checksums
    if (calculated_checksum != given_checksum) {
      printf("ERROR: Wrong checksum! (got %02X, expected %02X)\n",
             calculated_checksum, given_checksum);
      return -1;
    }
  }

  // Split on " " token
  char *token = strtok(line, " ");
  while (token != NULL && segment_count < 32) {
    segments[segment_count++] = token;
    token = strtok(NULL, " ");
  }

  // Process line number (if there)
  if (segment_count > 0 && segments[cur_segment][0] == 'N') {
    // Check if following part(s) are digits after 'n'
    char *ptr = segments[0] + 1;
    if (*ptr) {
      // Convert digits after 'n' to integer
      line_number = atoi(ptr);
    }
    cur_segment++;
  }

  // Process zone command
  if (segment_count > 0 && segments[cur_segment][0] == 'Z') {

    // One of either T or H must be included.
    if (cur_segment + 1 < segment_count &&
        (segments[cur_segment + 1][0] == 'T' ||
         segments[cur_segment + 1][0] == 'H')) {
      printf("Zone command: %s\n", segments[cur_segment + 1]);
    } else {
      printf("Error: Expected 'T' or 'H' after 'Z'\n");
    }
  }

  // Process M (machine) command
  if (segment_count > 0 && segments[cur_segment][0] == 'M') {
    // Machine command
    printf("Machine command: %s\n", segments[cur_segment + 1]);
  }

  return 0;
}

int main() {
  // Initialize stdio (USB serial)
  stdio_init_all();
  setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering for stdout

  // Status LED (GPIO25 on Pico)
  gpio_init(STAT_LED_PIN);
  gpio_set_dir(STAT_LED_PIN, GPIO_OUT);
  gpio_put(STAT_LED_PIN, 0);

  fflush(stdout);

  neopixel_ws2812_t neopixel = {0};
  neopixel_ws2812_init(&neopixel, pio0, NEOPIXEL_PIN, NEOPIXEL_FREQ_HZ, false);
  neopixel_ws2812_put_rgb(&neopixel, 0, 0, 0); // Lights off

  absolute_time_t last_heartbeat = get_absolute_time();

  // Buffer for reading lines
  char line_buffer[256];
  int line_index = 0;

  // Core Loop
  while (true) {
    // Check if data is available (10ms timeout)
    int c = getchar_timeout_us(10000);

    if (c != PICO_ERROR_TIMEOUT) {
      if (ENABLE_ECHO) {
        // Echo the character back
        putchar(c);
        fflush(stdout);
      }

      // Read/load entire line into buffer
      if (c == '\n' || c == '\r') {
        // End of line - null terminate and process
        if (line_index > 0) {
          line_buffer[line_index] = '\0';
          process_line(line_buffer);
          printf("ok\n");
          line_index = 0; // Reset for next line
        }
      } else if (line_index < sizeof(line_buffer) - 1) {
        // Add character to buffer (leave room for null terminator)
        line_buffer[line_index++] = (char)c;
      }
      // If buffer is full, ignore remaining characters until newline
    }

    // Heartbeat every 5 seconds to show we're alive
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_heartbeat, now) > 5000000) {
      printf(".\n");
      fflush(stdout);
      last_heartbeat = now;
    }

    if (stdio_usb_connected()) {
      gpio_put(STAT_LED_PIN, 1);
    } else {
      gpio_put(STAT_LED_PIN, 0);
    }
  }

  return 0;
}
