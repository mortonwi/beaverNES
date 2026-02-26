import sys

# Undocumented / illegal NOP opcodes used by nestest
UNDOCUMENTED = {
    0x04, 0x14, 0x34, 0x44, 0x54, 0x64, 0x74,
    0x0C,
    0x1C, 0x3C, 0x5C, 0x7C, 0xDC, 0xFC,
    0xD4, 0xF4
}

def strip_ppu(line):
    """
    Remove the PPU:xxx,yyy portion but KEEP the CYC:xxxx portion.
    nestest format ends with: 'PPU:  x,  y CYC:zz'
    """
    if "PPU:" in line:
        before_ppu, after_ppu = line.split("PPU:", 1)

        # after_ppu contains something like "  0,  0 CYC:7"
        # we want to extract the CYC: part
        cyc_index = after_ppu.find("CYC:")
        if cyc_index != -1:
            cycles = after_ppu[cyc_index:].rstrip()
            return (before_ppu + cycles).rstrip()

        # fallback: no cycles found
        return before_ppu.rstrip()

    return line.rstrip()

def extract_opcode(line):
    """
    Extract the opcode byte from a nestest-style trace line.
    Opcode is always at columns 6:8.
    """
    try:
        return int(line[6:8], 16)
    except:
        return None

def main():
    with open("outputTest.txt") as f1, open("rom/nestest.log") as f2:
        for i, (l1, l2) in enumerate(zip(f1, f2), start=1):
            s1 = strip_ppu(l1)
            s2 = strip_ppu(l2)

            opcode = extract_opcode(s2)

            # Skip undocumented opcodes
            if opcode in UNDOCUMENTED:
                continue

            if s1 != s2:
                print(f"Difference at line {i}:")
                print(f"  output.txt:   {s1}")
                print(f"  nestest.log:  {s2}")
                return

    print("No differences found (ignoring PPU and undocumented opcodes).")

if __name__ == "__main__":
    main()
