#!/usr/bin/env python3
"""Normalize only the generated compiler-flags display, never compiler inputs."""

import argparse
from pathlib import Path
import re
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", required=True, type=Path,
                        help="Generated FileZilla config/config.h")
    parser.add_argument("--toolchain-root", required=True,
                        help="Native MSYS2 installation prefix from cygpath -m /")
    args = parser.parse_args()
    path = args.config
    if path.name != "config.h" or path.parent.name != "config":
        parser.error("Expected the generated config/config.h file")
    text = path.read_bytes().decode("utf-8")
    lines = text.splitlines(keepends=True)
    matches = [i for i, line in enumerate(lines)
               if re.match(r'^#define\s+USED_CXXFLAGS\s+L?"', line)]
    if len(matches) != 1:
        raise ValueError("Expected exactly one generated USED_CXXFLAGS definition")
    index = matches[0]
    original = lines[index]
    native = args.toolchain_root.rstrip("/\\")
    if not native:
        raise ValueError("The native toolchain prefix must not be empty")
    backslash = native.replace("/", "\\")
    variants = {native, backslash, backslash.replace("\\", "\\\\")}
    normalized = original
    for variant in sorted(variants, key=len, reverse=True):
        normalized = normalized.replace(variant, "/toolchain/msys64")
    if normalized != original:
        lines[index] = normalized
        path.write_bytes("".join(lines).encode("utf-8"))
        print("Normalized generated USED_CXXFLAGS display; compiler flags unchanged.")
    else:
        print("Generated USED_CXXFLAGS display already contains no native toolchain prefix.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"Build-info normalization failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
