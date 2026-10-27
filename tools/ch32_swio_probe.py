#!/usr/bin/env python3
"""Experimental CMSIS-DAP vendor interface for CH32V003 SWIO identification."""
import argparse
import struct
import sys

import usb.core
import usb.util


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial', help='CMSIS-DAP USB serial number')
    parser.add_argument('--rvswd', action='store_true', help='use two-wire RVSWD')
    args = parser.parse_args()
    matches = []
    for device in usb.core.find(find_all=True, idVendor=0x2e8a, idProduct=0x000c):
        try:
            serial = usb.util.get_string(device, device.iSerialNumber)
        except (usb.core.USBError, ValueError):
            continue
        if args.serial is None or serial == args.serial:
            matches.append((device, serial))
    if len(matches) != 1:
        raise RuntimeError('select exactly one accessible PicoDualProbe with --serial')
    dev, serial = matches[0]
    usb.util.claim_interface(dev, 0)
    # Drop responses left behind by a previous interrupted client.
    for _ in range(33):
        try:
            dev.read(0x85, 64, timeout=200)
        except usb.core.USBTimeoutError:
            break
    else:
        raise RuntimeError('CMSIS-DAP response endpoint did not become idle')

    def command(op, address=None):
        packet = bytes([0x80, op]) + (bytes([address]) if address is not None else b'')
        dev.write(0x04, packet, timeout=1000)
        result = bytes(dev.read(0x85, 64, timeout=1000))
        if len(result) < 2 or result[0] != 0x80 or result[1] != 0:
            raise RuntimeError(f'{mode} command {op} failed: {result.hex()}')
        if address is not None:
            if len(result) != 6:
                raise RuntimeError(f'invalid {mode} response: {result.hex()}')
            return struct.unpack_from('<I', result, 2)[0]

    started = False
    mode = 'RVSWD' if args.rvswd else 'SWIO'
    try:
        command(4 if args.rvswd else 0)
        started = True
        part = command(2, 0x7f)
        cpbr = command(2, 0x7c)
        print(f'Probe serial: {serial}')
        print(f'WCH debug PART: 0x{part:08x}')
        print(f'WCH debug CPBR: 0x{cpbr:08x}')
        if part in (0, 0xffffffff) and cpbr in (0, 0xffffffff):
            raise RuntimeError('no meaningful debug response; check target power and SWIO')
        print('Raw debug response received; chip identification is not yet validated.')
    finally:
        try:
            if started:
                command(1)
        finally:
            usb.util.release_interface(dev, 0)
            usb.util.dispose_resources(dev)


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, usb.core.USBError) as exc:
        print(f'error: {exc}', file=sys.stderr)
        sys.exit(1)
