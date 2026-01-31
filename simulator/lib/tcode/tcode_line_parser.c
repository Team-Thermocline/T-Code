#include "tcode_line_parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to check if a string is an unsigned integer
static bool is_unsigned_int_token(const char *s) {
  if (!s || !*s)
    return false;
  for (const char *p = s; *p; ++p) {
    if (*p < '0' || *p > '9')
      return false;
  }
  return true;
}

int tcode_process_line(char *line) {
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
      (void)line_number;
    }
    cur_segment++;
  }

  // Process setpoint commands:
  // - "T15" / "H55"                (implicit zone 0)
  // - "Z0 T15" / "Z0 H55"          (explicit zone; zone must be included in Z token)
  //
  // Not accepted:
  // - "ZT15" / "Z T15" / "Z 0 T15" (must be a space between zone and T/H, and zone must be in Z token)
  if (segment_count > 0) {
    const char *cmd = segments[cur_segment];
    if (cmd && (cmd[0] == 'T' || cmd[0] == 'H' || cmd[0] == 'Z')) {
      int zone = 0;
      const char *th = NULL; // "T15" or "H55"

      if (cmd[0] == 'Z') {
        const char *zone_str = cmd[1] ? (cmd + 1) : NULL;
        if (!zone_str) {
          printf("Error: expected Z0 T15\n");
        } else if (!is_unsigned_int_token(zone_str)) {
          printf("Error: bad zone\n");
        } else if (cur_segment + 1 >= segment_count) {
          printf("Error: expected T/H after Z\n");
        } else {
          zone = atoi(zone_str);
          th = segments[cur_segment + 1];
        }
      } else {
        th = cmd; // implicit zone 0
      }

      if (th) {
        if (!(th[0] == 'T' || th[0] == 'H') || th[1] == '\0') {
          printf("Error: bad setpoint\n");
        } else if (zone != 0) {
          printf("Error: zone not supported\n");
        } else {
          int value = atoi(th + 1);
          extern float current_temperature_setpoint;
          extern float current_humidity_setpoint;
          if (th[0] == 'T') {
            if (value < -45 || value > 90)
              printf("Error: temp out of range\n");
            else
              current_temperature_setpoint = (float)value;
          } else {
            if (value < 0 || value > 100)
              printf("Error: humidity out of range\n");
            else
              current_humidity_setpoint = (float)value;
          }
        }
      }
    }
  }

  // Process M (machine) command
  if (segment_count > 0 && segments[cur_segment][0] == 'M') {
    // Accept either "M 123" (2 tokens) or "M123" (1 token)
    const char *marg = NULL;
    if (segments[cur_segment][1] != '\0') {
      marg = segments[cur_segment] + 1;
    } else if (cur_segment + 1 < segment_count) {
      marg = segments[cur_segment + 1];
    }

    if (marg) {
      printf("Machine command: %s\n", marg);
    } else {
      printf("Error: Missing M command argument\n");
    }
  }

  // Process Q (query) command
  if (segment_count > 0 && segments[cur_segment][0] == 'Q') {

    // Accept either "Q 0" (2 tokens) or "Q0" (1 token)
    const char *qarg = NULL;
    if (segments[cur_segment][1] != '\0') {
      qarg = segments[cur_segment] + 1;
    } else if (cur_segment + 1 < segment_count) {
      qarg = segments[cur_segment + 1];
    }

    // Query 0 for status
    if (qarg && strcmp(qarg, "0") == 0) {

      // Example status values; in real code these would come from live
      // variables
      extern float current_temperature;
      extern float current_humidity;
      extern float current_temperature_setpoint;
      extern float current_humidity_setpoint;
      extern bool heater_on;
      extern int current_state; // e.g., 0=IDLE, 1=RUN, etc. TODO: use an enum
      extern int alarm_state;

      // TODO: Remove me!
      current_humidity = 20.0f + ((float)(rand() % 61)); // 20 to 80 inclusive

      // Map current_state to a string
      const char *state_str = "UNKNOWN";
      switch (current_state) {
      case 0:
        state_str = "IDLE";
        break;
      case 1:
        state_str = "RUN";
        break;
      case 2:
        state_str = "STOP";
        break;
      case 3:
        state_str = "FAULT";
        break;
      }

      printf("data: TEMP=%.1f RH=%.1f HEAT=%s STATE=%s SET_TEMP=%.1f SET_RH=%.1f "
             "ALARM=%d\n",
             current_temperature, current_humidity,
             heater_on ? "true" : "false", state_str,
             current_temperature_setpoint, current_humidity_setpoint,
             alarm_state);
    } else {
      printf("Error: %s not a valid query command\n", qarg ? qarg : "(missing)");
    }
  }

  return 0;
}
