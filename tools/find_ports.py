#!/usr/bin/env python3
import json
import subprocess
import sys


def pio(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, "-m", "platformio", *args],
        check=True,
        capture_output=True,
        text=True,
    )


def main() -> int:
    try:
        devices = json.loads(pio("device", "list", "--json-output").stdout or "[]")
    except Exception as exc:
        print(f"Unable to query PlatformIO serial devices: {exc}")
        return 1

    if not devices:
        print("No serial devices found.")
        return 2

    print("Detected serial devices:")
    for index, dev in enumerate(devices, 1):
        port = dev.get("port", "?")
        desc = dev.get("description", "") or ""
        hwid = dev.get("hwid", "") or ""
        print(f"  [{index}] {port:8} {desc}")
        if hwid:
            print(f"      {hwid}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
