#pragma once

// NOTE: This parser modifies the input line in-place (uses tokenization).
// Pass a writable buffer.

int tcode_process_line(char *line);

