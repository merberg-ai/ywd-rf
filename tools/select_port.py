#!/usr/bin/env python3
import json
import subprocess
import sys


def devices():
    proc = subprocess.run(
        [sys.executable, "-m", "platformio", "device", "list", "--json-output"],
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads(proc.stdout or "[]")


def main() -> int:
    try:
        found = devices()
    except Exception as exc:
        print(f"ERROR: unable to query serial ports: {exc}", file=sys.stderr)
        return 2

    if not found:
        print("ERROR: no serial devices found", file=sys.stderr)
        return 3

    if len(found) == 1:
        print(found[0]["port"])
        return 0

    print("Select a serial device:", file=sys.stderr)
    for i, dev in enumerate(found, 1):
        print(f"  {i}. {dev.get('port','?')}  {dev.get('description','')}", file=sys.stderr)

    while True:
        try:
            choice = input("Port number: ").strip()
            idx = int(choice) - 1
            if 0 <= idx < len(found):
                print(found[idx]["port"])
                return 0
        except (ValueError, EOFError):
            pass
        print("Invalid selection.", file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())
