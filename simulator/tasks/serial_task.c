#include "serial_task.h"

#include "pico/error.h"
#include "pico/stdio.h"
#include <stdio.h>

static void serial_task(void *pvParameters) {
  const serial_task_config_t *cfg = (const serial_task_config_t *)pvParameters;

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

    // Echo back the character if enabled
    if (cfg && cfg->enable_echo && *(cfg->enable_echo)) {
      putchar(c);
      fflush(stdout);
    }

    // Read/load entire line into buffer
    if (c == '\n' || c == '\r') {
      // End of line - null terminate and process
      if (line_index > 0) {
        line_buffer[line_index] = '\0';
        if (cfg && cfg->line_handler) {
          cfg->line_handler(line_buffer);
        }
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

BaseType_t serial_task_create(const serial_task_config_t *cfg,
                              UBaseType_t priority, TaskHandle_t *out_handle) {
  return xTaskCreate(serial_task, "serial", 1024, (void *)cfg, priority,
                     out_handle);
}

