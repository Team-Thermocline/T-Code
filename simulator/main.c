#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"
#include "neopixel_ws2812.h"
#include "pico/error.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pindefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool ENABLE_ECHO = false;

// NeoPixel (WS2812) config
static const float NEOPIXEL_FREQ_HZ = 800000.0f;
static const uint8_t NEOPIXEL_MAX_BRIGHTNESS = 24; // 0-255, keep it gentle
static const TickType_t NEOPIXEL_STARTUP_DELAY_MS = 3000;
static const TickType_t NEOPIXEL_FRAME_DELAY_MS = 20;

static neopixel_ws2812_t g_neopixel;

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

static void serial_task(void *pvParameters) {
  (void)pvParameters;

  // Buffer for reading lines
  char line_buffer[256];
  int line_index = 0;

  while (true) {
    // Non-blocking read; yield when there's nothing to do
    int c = getchar_timeout_us(0);

    if (c == PICO_ERROR_TIMEOUT) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    if (ENABLE_ECHO) {
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
        fflush(stdout);
        line_index = 0; // Reset for next line
      }
    } else if (line_index < (int)sizeof(line_buffer) - 1) {
      // Add character to buffer (leave room for null terminator)
      line_buffer[line_index++] = (char)c;
    }
    // If buffer is full, ignore remaining characters until newline
  }
}

static void status_led_task(void *pvParameters) {
  (void)pvParameters;
  while (true) {
    gpio_put(STAT_LED_PIN, stdio_usb_connected() ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

static void heartbeat_task(void *pvParameters) {
  (void)pvParameters;
  while (true) {
    printf(".\n");
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

static void neopixel_task(void *pvParameters) {
  (void)pvParameters;

  // Keep the LED dim for a bit after boot (less distracting / power draw).
  neopixel_ws2812_put_rgb(&g_neopixel, 2, 2, 2);
  vTaskDelay(pdMS_TO_TICKS(NEOPIXEL_STARTUP_DELAY_MS));

  uint8_t t = 0;
  int dir = 1; // +1: blue->green, -1: green->blue

  while (true) {
    // Blue <-> green crossfade (dimmed)
    uint8_t g = (uint8_t)(((uint16_t)t * NEOPIXEL_MAX_BRIGHTNESS) / 255u);
    uint8_t b =
        (uint8_t)(((uint16_t)(255u - t) * NEOPIXEL_MAX_BRIGHTNESS) / 255u);
    neopixel_ws2812_put_rgb(&g_neopixel, 0, g, b);

    int next = (int)t + dir;
    if (next >= 255) {
      t = 255;
      dir = -1;
    } else if (next <= 0) {
      t = 0;
      dir = 1;
    } else {
      t = (uint8_t)next;
    }

    vTaskDelay(pdMS_TO_TICKS(NEOPIXEL_FRAME_DELAY_MS));
  }
}

int main() {
  // Initialize stdio (USB serial)
  stdio_init_all();
  setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering for stdout

  // Configure Status LEDs (GPIO25 on Pico)
  gpio_init(STAT_LED_PIN);
  gpio_set_dir(STAT_LED_PIN, GPIO_OUT);
  gpio_put(STAT_LED_PIN, 0);

  neopixel_ws2812_init(&g_neopixel, pio0, NEOPIXEL_PIN, NEOPIXEL_FREQ_HZ, false);
  neopixel_ws2812_put_rgb(&g_neopixel, 2, 2, 2); // Dim white light on startup.

  fflush(stdout);

  // =============
  // Program Begin
  // =============

  if (xTaskCreate(serial_task, "serial", 1024, NULL, 2, NULL) != pdPASS)
    vApplicationMallocFailedHook();
  if (xTaskCreate(status_led_task, "status", 256, NULL, 1, NULL) != pdPASS)
    vApplicationMallocFailedHook();
  if (xTaskCreate(heartbeat_task, "heartbeat", 512, NULL, 1, NULL) != pdPASS)
    vApplicationMallocFailedHook();
  if (xTaskCreate(neopixel_task, "neopixel", 512, NULL, 1, NULL) != pdPASS)
    vApplicationMallocFailedHook();

  vTaskStartScheduler();

  // If we get here, the scheduler couldn't start (usually heap too small).
  vApplicationMallocFailedHook();

  // Should never reach here.
  while (true) {
  }

  return 0;
}
