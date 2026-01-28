#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    // Initialize stdio (USB serial)
    stdio_init_all();
    
    // Wait for USB serial to enumerate
    sleep_ms(2000);

    // Send a welcome message
    printf("\n\n=== USB Serial Echo Program ===\n");
    printf("Type something and it will be echoed back...\n\n");
    fflush(stdout);

    absolute_time_t last_heartbeat = get_absolute_time();
    
    // Echo loop - read from USB serial and echo it back
    while (true) {
        // Check if data is available (10ms timeout)
        int c = getchar_timeout_us(10000);
        
        if (c != PICO_ERROR_TIMEOUT) {
            // Echo the character back
            putchar(c);
            fflush(stdout);
        }
        
        // Heartbeat every 5 seconds to show we're alive
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_heartbeat, now) > 5000000) {
            printf(".\n");
            fflush(stdout);
            last_heartbeat = now;
        }
    }

    return 0;
}
