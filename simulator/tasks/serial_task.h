#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>

typedef int (*serial_line_handler_t)(char *line);

typedef struct serial_task_config {
  serial_line_handler_t line_handler;
  bool *enable_echo; // optional; if NULL, echo is disabled
} serial_task_config_t;

// Create the serial task. `cfg` must remain valid for the lifetime of the task.
BaseType_t serial_task_create(const serial_task_config_t *cfg,
                              UBaseType_t priority, TaskHandle_t *out_handle);

