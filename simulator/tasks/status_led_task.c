#include "status_led_task.h"

#include "hardware/gpio.h"
#include "pico/stdio_usb.h"
#include "pindefs.h"

static void status_led_task(void *pvParameters) {
  (void)pvParameters;
  while (true) {
    gpio_put(STAT_LED_PIN, stdio_usb_connected() ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

BaseType_t status_led_task_create(UBaseType_t priority, TaskHandle_t *out_handle) {
  return xTaskCreate(status_led_task, "status", 256, NULL, priority, out_handle);
}

