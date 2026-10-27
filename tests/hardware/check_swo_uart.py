#!/usr/bin/env python3
"""Check SWO UART receiver using the RAM-only Pico UART source (not ITM)."""
import argparse
import struct
import time
import usb.core
import usb.util

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--serial', required=True)
parser.add_argument('--seconds', type=float, default=3.0)
parser.add_argument('--baud', type=int, default=115200, help='match SWO_TEST_BAUD in source build')
args = parser.parse_args()
if not 1 <= args.baud <= 2000000 or args.seconds <= 0:
    parser.error('baud must be 1..2000000 and seconds must be positive')
matches = []
for dev in usb.core.find(find_all=True, idVendor=0x2e8a, idProduct=0x000c):
    try:
        if usb.util.get_string(dev, dev.iSerialNumber) == args.serial:
            matches.append(dev)
    except (usb.core.USBError, ValueError):
        pass
if len(matches) != 1:
    raise SystemExit('Expected exactly one matching accessible probe')
dev = matches[0]
usb.util.claim_interface(dev, 0)
try:
    for _ in range(33):
        try:
            dev.read(0x85, 64, timeout=200)
        except usb.core.USBTimeoutError:
            break
    else:
        raise RuntimeError('endpoint did not become idle')
    def command(request, length):
        dev.write(0x04, bytes(request), timeout=1000)
        data = bytes(dev.read(0x85, 64, timeout=1000))
        if len(data) < length or data[0] != request[0]:
            raise RuntimeError(f'invalid response to {request[0]:02x}: {data.hex()}')
        return data
    def config(op, value):
        data = command([op, value], 2)
        if data[1]:
            raise RuntimeError(f'command {op:02x} rejected: {data.hex()}')
    started = False
    try:
        config(0x17, 1)
        config(0x18, 1)
        actual = struct.unpack_from('<I', command([0x19, *struct.pack('<I', args.baud)], 5), 1)[0]
        if abs(actual - args.baud) > args.baud * 0.03:
            raise RuntimeError(f'{args.baud} baud unavailable: {actual}')
        config(0x1a, 1)
        started = True
        last = None
        count = 0
        deadline = time.monotonic() + args.seconds
        while time.monotonic() < deadline:
            data = command([0x1c, 60, 0], 4)
            status, size = data[1], int.from_bytes(data[2:4], 'little')
            if status & 0xc0:
                raise RuntimeError(f'SWO receive error status 0x{status:02x}')
            if size > len(data) - 4 or size > 60:
                raise RuntimeError('invalid SWO data size')
            for byte in data[4:4 + size]:
                if last is not None and byte != ((last + 1) & 255):
                    raise RuntimeError(f'out of sequence after {count} bytes: {last:02x} -> {byte:02x}')
                last = byte
                count += 1
            if not size:
                time.sleep(0.002)
        if count < 256:
            raise RuntimeError(f'only {count} bytes received')
        print(f'UART waveform received: {count} sequential bytes at {actual} baud; '
              'this does not validate ITM or a real SWO target')
    finally:
        if started:
            config(0x1a, 0)
        config(0x18, 0)
finally:
    usb.util.release_interface(dev, 0)
    usb.util.dispose_resources(dev)
