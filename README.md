# TCODE

![TCODE Meme](static/tcode.png)

v0.1 (Draft)

## What is TCODE

TCODE was developed as a collaboration between the two thermal chamber 
testing teams working this year at SNHU, [Team Thermocline](https://team-thermocline.github.io/)
and [EZ-Bake](https://ez-bake.readthedocs.io/en/latest/).

TCODE is a simple, line-oriented ASCII command protocol for controlling
environmental chambers. It is inspired by G-code-style serial protocols
but is **not compatible** with G-code.

TCODE is designed to be:
- Human-readable
- Easy to parse on embedded systems
- Reliable over serial or stream transports
- Suitable for temperature and humidity control


## Transport

TCODE operates over any byte-stream transport, including but not limited to:
- USB serial
- UART
- TCP sockets
- WebSockets

Commands are newline-delimited using LF (``\\n``). CRLF (``\\r\\n``) MAY be accepted.


# General Command Format

Each command consists of a single ASCII line with space-separated fields.

```nc
[N<num>] [Z<zone>] T<temperature> [H<humidity>] *<checksum>
```

Fields are positional-independent but MUST appear before the checksum.

## Field Definitions

``N<num>`` (optional)
  Line number.
  - Integer
  - Used for command ordering and resend requests

``Z<zone>`` (optional)
  Target zone or chamber index.
  - Integer ≥ 0
  - If omitted, defaults to ``Z0``

``T<temperature>`` (T or H required)
  Temperature setpoint.
  - Units: degrees Celsius
  - Signed floating-point value

``H<humidity>`` (T or H required)
  Humidity setpoint.
  - Units: percent relative humidity (%RH)
  - Floating-point or integer value
  - Valid range: 0–100
  - If omitted, humidity is not modified

``*<checksum>`` (required)
  Checksum field.
  - Indicates end of command
  - Used for transmission integrity checking


## Whitespace

- Fields MUST be separated by at least one space
- Leading and trailing whitespace SHOULD be ignored
- No whitespace is permitted inside a field

## Keepalives

T-Code controllers and devices may send a `.` to be ignored by receiving systems but used as a standby
heartbeat and keepalive.

```nc
< .
```

## Checksum

The checksum is an 8-bit XOR of all ASCII bytes from the start of the line
up to (but not including) the ``*`` character.

- Newline characters are NOT included
- The checksum is represented as two hexadecimal digits (00–FF)

## Checksum Example

```cpp

#include <stdio.h>
#include <stdint.h>

uint8_t tcode_checksum(const char *line) {
    uint8_t cs = 0;
    while (*line && *line != '*') {
        cs ^= (uint8_t)(*line++);
    }
    return cs;
}

int main() {
    const char *cmd = "T-10.0 H35.0";
    uint8_t cs = tcode_checksum(cmd);
    printf("%s*%02X\n", cmd, cs); // Output: T-10.0 H35.0*3C
    return 0;
}
```

## Firmware Responses

Every command MUST result in an ``ok`` response.

Responses are newline-delimited ASCII lines.

``ok``
  Indicates the command was received and processed (or rejected).

``error:<code> <message>``
  Indicates an error occurred while processing the command.
  - You must still send an ``ok``!

``resend:<line>``
  Requests retransmission of the specified line number.
  - Only valid if line numbers are in use
  - You must still send an ``ok``!

## Response Ordering

If an error or resend is generated, it MUST precede ``ok``.

Example:

```nc
< N100 H120*A0
> error:RANGE H=120.0 exceeds 0–100
> ok
```

# Examples

Temperature only, default zone

```nc
< T-10.0*5A
> ok
```

Temperature and humidity, default zone

```nc
< T-10.0 H35.0*3C
> ok
```

Explicit zone and line number

```nc
< N12 Z1 T25.0 H50.0*9F
> ok
```

Invalid humidity example

```nc
< N13 Z0 T20.0 H120.0*AA
> error:RANGE H=120.0 exceeds 0–100
> ok
```

# Q and N codes

## Q Codes

### Q0

`Q` Codes are to be used for querrying specific information from the system. The most common of which is `Q0`:

```nc
< Q0*44   ; query status
> data: TEMP=-9.2 RH=33.8 HEAT=false STATE=RUN ALARM=0
> ok
```

Controller should reply with `data: ` header and list of values with these values being common:

| Value   | Type    | Typical Range / Values                 | Description                                  |
|---------|---------|----------------------------------------|----------------------------------------------|
| TEMP    | float   | -40.0 to 125.0 (°C)                    | Current temperature                          |
| RH      | float   | 0.0–100.0 (%)                          | Relative humidity                            |
| HEAT    | bool    | true / false                           | Heating active (true = heating)              |
| STATE   | string  | "RUN", "IDLE", "FAULT", "ALARM", etc.  | System/controller state                      |
| ALARM   | int     | 0+                                     | Alarm/alert code (0 = no alarm)              |


### Q1

Machine Information
```nc
< Q1 BUILD*13   ; query machine information
> data: BUILD=ver1.0_x
> ok
```

The format is
```
Q1 <machine information name>*XX
```

You could choose to reply to and respond with any number of keys and values but at the very least you should respond to these fields.

| Key                | Example      |
|--------------------|--------------|
| BUILD              | v1.0-g123456 |
| BUILDER            | Your_Name    |
| BUILD_DATE         | 1769979847   |


## M Codes

M codes should be used to modify or to return specific controller settings and values, for example:

```
M0        Stop / idle chamber
M1        Start previously configured profile
M2        Abort profile (immediate)
M3        Pause profile
M4        Resume profile
```

```
M10               List available profiles
M11 P<name>       Load profile into active context
M12               Clear loaded profile
```

Example

```nc
< N20 M11 P=COLD_SOAK*CS  ; Load cold whatever
> ok
< N21 M1*CS               ; Run that profile
> ok
```


```nc
< M1 P=PROFILE_A*3C       ; start profile A
> ok
```

## M Settings

```
< M20                     ; List all settings
< M21 K<key>              ; Read setting
< M22 K<key> V<val>       ; Write setting (volatile)
< M23 K<key> V<val>       ; Save setting (persistent)
```

Example:

```nc
< N30 M20*CS
> data:MAX_TEMP=85.0
> data:MAX_RAMP=3.0
> data:DEFAULT_ZONE=0
> ok
```

```nc
> N31 M22 K=MAX_RAMP V=2.0*CS
< ok
```


# Behavioral Rules

- ``T`` or ``H`` is REQUIRED (omission of one or the other should leave that setpoint unchaged)
- ``Z`` defaults to ``0`` if omitted
- All numeric fields MUST be validated independently
- Commands MAY update multiple parameters atomically
- Firmware MAY queue commands; ``ok`` does not imply physical completion



Compatibility
-------------

TCODE is intentionally not compatible with existing G-code dialects.
Implementations SHOULD NOT attempt to interpret TCODE commands as G-code.
