#!/usr/bin/env python3
# patch_add_syslog_shim.py
import lief, sys
from pathlib import Path

if len(sys.argv) < 3:
    print("usage: patch_add_syslog_shim.py <infile> <outfile>")
    sys.exit(2)

infile = Path(sys.argv[1])
outfile = Path(sys.argv[2])

# parse (handles fat/universal)
binary = lief.parse(str(infile))

# print(binary)

# Make sure there’s an RPATH that points to your app’s Frameworks folder
# (adjust if your bundle layout differs)
# if "@loader_path/../Frameworks" not in bin.rpaths:
#     bin.add_rpath("@loader_path/../Frameworks")

# Add a load command for the shim
binary.add(lief.MachO.DylibCommand.load_dylib("@loader_path/libsyslog_extsn_shim.dylib", 0, 0, 0))

binary.write(str(outfile))

print(f"[✓] Wrote patched file to {outfile}")
