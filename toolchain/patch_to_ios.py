#!/usr/bin/env python3
import argparse, sys, re
import lief

# ---- Helpers ---------------------------------------------------------------

def parse_ver_triplet(s, default=(0,0,0)):
    # accept "17", "17.0", "17.0.1"
    if not s: return default
    parts = [int(x) for x in s.split(".")]
    while len(parts) < 3: parts.append(0)
    return tuple(parts[:3])

def platform_enum(name: str):
    name = name.lower().replace("-", "_")
    P = lief.MachO.BuildVersion.PLATFORMS
    if name in ("ios",): return P.IOS
    if name in ("ios_sim", "ios_simulator", "sim", "simulator"): return P.IOS_SIMULATOR
    if name in ("macos", "mac"): return P.MACOS
    return P.IOS  # default

def patch_one_binary(binobj, args):
    # 1) Remove signature (any change invalidates it anyway)
    if getattr(binobj, "has_code_signature", False):
        try:
            binobj.remove_signature()
        except Exception:
            pass

    # 2) Ensure LC_BUILD_VERSION (platform/minos/sdk)
    minos = parse_ver_triplet(args.minos or "13.0.0")
    sdk   = parse_ver_triplet(args.sdk   or "18.0.0")
    plat  = platform_enum(args.platform)

    bv = binobj.build_version
    if bv is None:
        # Create a fresh BuildVersion
        try:
            bv = lief.MachO.BuildVersion()
        except Exception:
            # Older LIEF versions still expose BuildVersion; if ctor fails, re-raise
            raise
        bv.platform = plat
        bv.minos = list(minos)
        bv.sdk   = list(sdk)
        binobj.add(bv)  # add LC_BUILD_VERSION
    else:
        bv.platform = plat
        bv.minos = list(minos)
        bv.sdk   = list(sdk)

    # 3) Optionally (re)set LC_ID_DYLIB (install name)
    if args.install_name:
        # Remove any existing ID_DYLIB (only one allowed)
        for cmd in list(binobj.commands):
            t = getattr(cmd, "command", None) or getattr(cmd, "type", None)
            # Robustly check type by name to cover bindings
            if cmd.__class__.__name__ in ("DylibCommand",) and getattr(cmd, "is_id", False):
                binobj.remove(cmd)

        # LIEF uses packed integers for version fields; helper converts tuple->int
        cur = lief.MachO.DylibCommand.version2int(parse_ver_triplet(args.current_version or "1.0.0"))
        comp = lief.MachO.DylibCommand.version2int(parse_ver_triplet(args.compat_version or "1.0.0"))
        idcmd = lief.MachO.DylibCommand.id_dylib(args.install_name, 0, cur, comp)
        binobj.add(idcmd)

def main():
    ap = argparse.ArgumentParser(
        description="Patch Mach-O to claim iOS via LC_BUILD_VERSION (+ optional LC_ID_DYLIB)."
    )
    ap.add_argument("input", help="input Mach-O (single or fat)")
    ap.add_argument("-o", "--output", required=True, help="output path")
    ap.add_argument("--platform", default="ios",
                    choices=["ios","ios-simulator","ios_simulator","sim","macos","mac"],
                    help="target platform for LC_BUILD_VERSION (default: ios)")
    ap.add_argument("--minos", help="minimum OS version (e.g. 13.0.0). Default 13.0.0")
    ap.add_argument("--sdk", help="SDK version (e.g. 18.0.0). Default 18.0.0")

    # LC_ID_DYLIB options (optional)
    ap.add_argument("--install-name", help="set LC_ID_DYLIB install name (e.g. @rpath/libfoo.dylib)")
    ap.add_argument("--current-version", help="current version X.Y.Z for LC_ID_DYLIB (default 1.0.0)")
    ap.add_argument("--compat-version", help="compat version X.Y.Z for LC_ID_DYLIB (default 1.0.0)")

    args = ap.parse_args()

    fat = lief.MachO.parse(args.input, config=lief.MachO.ParserConfig.deep)
    if fat is None:
        print("Not a Mach-O?", file=sys.stderr)
        sys.exit(2)

    # fat object also represents single-arch binaries; iterate uniformly
    for i in range(fat.size):
        patch_one_binary(fat.at(i), args)

    fat.write(args.output)
    print(f"Patched -> {args.output}")
    print("Reminder: codesign the result, e.g.:")
    print('  codesign -f -s "-" --entitlements <your.entitlements> "<output>"')

if __name__ == "__main__":
    main()