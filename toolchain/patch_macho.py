#!/usr/bin/env python3
# patch_macho.py
import lief
import sys
from pathlib import Path

if len(sys.argv) < 3:
    print("usage: patch_macho.py <infile> <outfile>")
    sys.exit(2)

infile = Path(sys.argv[1])
outfile = Path(sys.argv[2])

# parse (handles fat/universal)
binary = lief.parse(str(infile))

if isinstance(binary, lief.MachO.FatBinary):
    binaries = list(binary)
else:
    binaries = [binary]

for b in binaries:
    hdr = b.header

    # 1) change filetype
    hdr.file_type = lief.MachO.Header.FILE_TYPE.DYLIB   # MH_DYLIB
    b.add(lief.MachO.DylibCommand.id_dylib(outfile.name, 0, 0, 0))

    # 2) patch __PAGEZERO if present
    pagezero = None
    for seg in b.segments:
        if seg.name == "__PAGEZERO":
            pagezero = seg
            break

    if pagezero:
        pagezero.virtual_address = 0xFFFFC000
        pagezero.virtual_size    = 0x4000
        pagezero.file_offset     = 0
        pagezero.file_size       = 0
        print(f"[+] Patched __PAGEZERO in {infile}")
    else:
        print(f"[!] No __PAGEZERO segment found in {infile}")

# 3) rebuild and write out
binary.write(str(outfile))

print(f"[✓] Wrote patched file to {outfile}")
