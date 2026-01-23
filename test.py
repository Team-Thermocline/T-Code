def tcode_checksum(line: str) -> int:
    cs = 0
    for c in line:
        cs ^= ord(c)
    return cs

line = "N100 Z0 T-10 H50"
print(f"{tcode_checksum(line):02X}")
