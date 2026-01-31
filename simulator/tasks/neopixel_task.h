#pragma once

#include "FreeRTOS.h"
#include "neopixel_ws2812.h"
#include "task.h"
#include <stdint.h>

typedef struct neopixel_task_config {
  neopixel_ws2812_t *np;
  uint8_t startup_r;
  uint8_t startup_g;
  uint8_t startup_b;
  uint8_t max_brightness; // 0-255
  TickType_t startup_delay_ticks;
  TickType_t frame_delay_ticks;
} neopixel_task_config_t;

// Create the NeoPixel animation task. `cfg` must remain valid for the lifetime
// of the task, and `cfg->np` must point to an already-initialized NeoPixel.
BaseType_t neopixel_task_create(const neopixel_task_config_t *cfg,
                                UBaseType_t priority, TaskHandle_t *out_handle);

