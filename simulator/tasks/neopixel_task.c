#include "neopixel_task.h"

#include "pico/stdlib.h"

static void neopixel_task(void *pvParameters) {
  const neopixel_task_config_t *cfg =
      (const neopixel_task_config_t *)pvParameters;
  if (!cfg || !cfg->np) {
    vTaskDelete(NULL);
    return;
  }

  // Keep the LED dim for a bit after boot (less distracting / power draw).
  neopixel_ws2812_put_rgb(cfg->np, cfg->startup_r, cfg->startup_g,
                          cfg->startup_b);
  vTaskDelay(cfg->startup_delay_ticks);

  uint8_t t = 0;
  int dir = 1; // +1: blue->green, -1: green->blue

  while (true) {
    // Blue <-> green crossfade (dimmed)
    uint8_t g = (uint8_t)(((uint16_t)t * cfg->max_brightness) / 255u);
    uint8_t b = (uint8_t)(((uint16_t)(255u - t) * cfg->max_brightness) / 255u);
    neopixel_ws2812_put_rgb(cfg->np, 0, g, b);

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

    vTaskDelay(cfg->frame_delay_ticks);
  }
}

BaseType_t neopixel_task_create(const neopixel_task_config_t *cfg,
                                UBaseType_t priority,
                                TaskHandle_t *out_handle) {
  return xTaskCreate(neopixel_task, "neopixel", 512, (void *)cfg, priority,
                     out_handle);
}

