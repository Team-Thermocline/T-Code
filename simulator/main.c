#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"
#include "neopixel_ws2812.h"
#include "pico/error.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pindefs.h"
#include "neopixel_task.h"
#include "serial_task.h"
#include "status_led_task.h"
#include "tcode_line_parser.h"
#include <stdio.h>

bool ENABLE_ECHO = false;

// NeoPixel (WS2812) config
static const float NEOPIXEL_FREQ_HZ = 800000.0f;
static const uint8_t NEOPIXEL_MAX_BRIGHTNESS = 24; // 0-255
static const TickType_t NEOPIXEL_STARTUP_DELAY_MS = 3000;
static const TickType_t NEOPIXEL_FRAME_DELAY_MS = 20;

static neopixel_ws2812_t g_neopixel;

static void heartbeat_task(void *pvParameters) {
  (void)pvParameters;
  while (true) {
    printf(".\n");
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(5000));
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

  static const serial_task_config_t serial_cfg = {
      .line_handler = tcode_process_line,
      .enable_echo = &ENABLE_ECHO,
  };
  static const neopixel_task_config_t neopixel_cfg = {
      .np = &g_neopixel,
      .startup_r = 2,
      .startup_g = 2,
      .startup_b = 2,
      .max_brightness = NEOPIXEL_MAX_BRIGHTNESS,
      .startup_delay_ticks = pdMS_TO_TICKS(NEOPIXEL_STARTUP_DELAY_MS),
      .frame_delay_ticks = pdMS_TO_TICKS(NEOPIXEL_FRAME_DELAY_MS),
  };

  if (serial_task_create(&serial_cfg, 2, NULL) != pdPASS)
    vApplicationMallocFailedHook();
  if (status_led_task_create(1, NULL) != pdPASS)
    vApplicationMallocFailedHook();
  if (xTaskCreate(heartbeat_task, "heartbeat", 512, NULL, 1, NULL) != pdPASS)
    vApplicationMallocFailedHook();
  if (neopixel_task_create(&neopixel_cfg, 1, NULL) != pdPASS)
    vApplicationMallocFailedHook();

  vTaskStartScheduler();

  // If we get here, the scheduler couldn't start (usually heap too small).
  vApplicationMallocFailedHook();

  // Should never reach here.
  while (true) {
  }

  return 0;
}
