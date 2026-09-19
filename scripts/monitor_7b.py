"""Capture UART logs without enabling USB CDC or occupying COM9 afterwards."""
import argparse
import time
from pathlib import Path
import serial

p = argparse.ArgumentParser()
p.add_argument("--port", default="COM9")
p.add_argument("--seconds", type=int, default=90)
p.add_argument("--output", default=".pio/boot-7b.log")
p.add_argument("--reset", action="store_true")
args = p.parse_args()
dest = Path(args.output)
dest.parent.mkdir(parents=True, exist_ok=True)
s = serial.Serial(port=None, baudrate=115200, timeout=0.2)
s.port = args.port
s.dtr = False
s.rts = False
with s, dest.open("wb") as out:
    if args.reset:
        s.rts = True
        time.sleep(0.15)
        s.rts = False
    end = time.monotonic() + args.seconds
    while time.monotonic() < end:
        data = s.read(s.in_waiting or 1)
        if data:
            out.write(data)
            out.flush()
print(f"UART capture saved: {dest}")
