#!/usr/bin/env python3
"""Configure PicoDualProbe Zero. Changes take effect after reboot."""
import argparse
import json
from pathlib import Path
import struct
import sys
import time
import usb.core
import usb.util

FIELDS = ('swclk', 'swdio', 'tdi', 'tms', 'tdo', 'reset', 'uart_tx', 'uart_rx',
          'swo', 'led_error', 'led_jtag', 'led_swd', 'led_ready', 'led_reserved',
          'led_enabled', 'led_active_high', 'swo_enabled')
DEFAULT = dict(zip(FIELDS, (1, 2, 3, 6, 7, 8, 4, 5, 17, 9, 10, 11, 12, 13, 31, 1, 1)))
STATUS = {0: 'ok', 1: 'invalid setting or command', 2: 'debug or trace is active',
          3: 'flash write failed', 4: 'no saved settings', 5: 'saved settings corrupt'}


def encode(settings):
    if set(settings) != set(FIELDS):
        raise ValueError('JSON must contain exactly: ' + ', '.join(FIELDS))
    values = [settings[k] for k in FIELDS]
    if any(type(v) is not int or v < 0 or v > 255 for v in values):
        raise ValueError('all values must be integers from 0 to 255')
    return bytes(values)


def decode(data):
    if len(data) != len(FIELDS):
        raise ValueError('invalid settings response length')
    return dict(zip(FIELDS, data))



def format_pinout(settings, connector):
    rows = connector.get('rows')
    if not isinstance(rows, list) or not rows or any(not isinstance(row, list) or not row for row in rows):
        raise ValueError('connector rows must be non-empty lists')
    mapping = {}
    for key, label in (('swclk', 'SWCLK / TCK / RVSWD CLK'),
                       ('swdio', 'SWDIO / SWIO / RVSWD IO'), ('tdi', 'JTAG TDI'),
                       ('tms', 'JTAG TMS'), ('tdo', 'JTAG TDO'), ('reset', 'nRESET'),
                       ('uart_tx', 'UART TX'), ('uart_rx', 'UART RX')):
        mapping.setdefault(settings[key], []).append(label)
    if settings['swo_enabled']:
        mapping.setdefault(settings['swo'], []).append('SWO')
    for i, key in enumerate(('led_error', 'led_jtag', 'led_swd', 'led_ready', 'led_reserved')):
        if settings['led_enabled'] & (1 << i):
            mapping.setdefault(settings[key], []).append(key.replace('_', ' ').upper())
    lines = [connector.get('name', 'Connector'), connector.get('orientation', 'Viewing direction unspecified')]
    exposed = set()
    for r, row in enumerate(rows, 1):
        for c, entry in enumerate(row, 1):
            wire_color = None
            if isinstance(entry, dict):
                if set(entry) - {'pin', 'wire_color'}:
                    raise ValueError('unknown connector pin field')
                pin = entry.get('pin')
                wire_color = entry.get('wire_color')
                if wire_color is not None and (not isinstance(wire_color, str) or not wire_color.strip()):
                    raise ValueError('wire_color must be a non-empty string')
            else:
                pin = entry
            if not isinstance(pin, str):
                raise ValueError('each connector pin must be a string or a pin object')
            if pin == 'GND': label = 'GND'
            elif pin in ('3V3', '5V'): label = 'Power ' + pin
            elif pin.startswith('GP') and pin[2:].isdigit() and 0 <= int(pin[2:]) <= 29:
                exposed.add(int(pin[2:]))
                label = ' / '.join(mapping.get(int(pin[2:]), [])) or 'unassigned'
            else: raise ValueError('invalid connector pin: ' + pin)
            color_note = f'; wire: {wire_color}' if wire_color is not None else ''
            lines.append(f'row {r}, column {c}: {pin:<4} {label}{color_note}')
    leds = connector.get('leds', [])
    if not isinstance(leds, list):
        raise ValueError('connector leds must be a list')
    if leds:
        lines.append('Physical LEDs (order is not specified):')
        for led in leds:
            if not isinstance(led, dict) or type(led.get('gpio')) is not int or not 0 <= led['gpio'] <= 29:
                raise ValueError('each LED requires a GPIO number from 0 to 29')
            gpio = led['gpio']
            name = led.get('name', f'LED on GP{gpio}')
            color = led.get('color', 'unspecified color')
            other_end = led.get('other_end', 'unspecified')
            if not all(isinstance(x, str) for x in (name, color, other_end)):
                raise ValueError('LED name, color, and other_end must be strings')
            roles = []
            for i, key in enumerate(('led_error', 'led_jtag', 'led_swd', 'led_ready', 'led_reserved')):
                if settings[key] == gpio:
                    enabled = bool(settings['led_enabled'] & (1 << i))
                    roles.append(key.replace('_', ' ').upper() + ('' if enabled else ' (disabled)'))
            role = ' / '.join(roles) or 'no LED role assigned'
            polarity = 'High' if settings['led_active_high'] else 'Low'
            warning = ''
            if roles and ((other_end == 'GND' and polarity == 'Low') or
                          (other_end == '3V3' and polarity == 'High')):
                warning = ' [polarity differs from recorded wiring]'
            lines.append(f'{name}: GP{gpio} {color}; {role}; configured {polarity}; other end {other_end}{warning}')
            exposed.add(gpio)
    missing = [f'GP{gpio}: {" / ".join(labels)}' for gpio, labels in sorted(mapping.items()) if gpio not in exposed]
    if missing:
        lines.append('Configured outside this connector: ' + '; '.join(missing))
    return '\n'.join(lines)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial', required=True, help='Zero CMSIS-DAP serial number')
    sub = parser.add_subparsers(dest='action', required=True)
    sub.add_parser('show')
    pinout = sub.add_parser('pinout')
    pinout.add_argument('--map', default=str(Path(__file__).with_name('zero-connector.json')), help='physical connector map JSON')
    setcmd = sub.add_parser('set'); setcmd.add_argument('file', help='JSON file')
    verify = sub.add_parser('verify'); verify.add_argument('file', help='expected JSON file')
    sub.add_parser('reset')
    led = sub.add_parser('led'); led.add_argument('role', choices=('error', 'jtag', 'swd', 'ready', 'reserved'))
    led.add_argument('--seconds', type=float, default=1.0)
    args = parser.parse_args()
    matches = []
    for dev in usb.core.find(find_all=True, idVendor=0x2e8a, idProduct=0x000c):
        try:
            if usb.util.get_string(dev, dev.iSerialNumber) == args.serial:
                matches.append(dev)
        except (usb.core.USBError, ValueError):
            pass
    if len(matches) != 1:
        raise RuntimeError('select exactly one accessible Zero by serial')
    dev = matches[0]
    usb.util.claim_interface(dev, 0)
    try:
        for _ in range(33):
            try:
                dev.read(0x85, 64, timeout=200)
            except usb.core.USBTimeoutError:
                break
        else:
            raise RuntimeError('CMSIS-DAP response endpoint did not become idle')
        def command(op, payload=b''):
            dev.write(0x04, bytes([0x81, op]) + payload, timeout=2000)
            resp = bytes(dev.read(0x85, 64, timeout=3000))
            if len(resp) < 2 or resp[0] != 0x81:
                raise RuntimeError('invalid configuration response: ' + resp.hex())
            return resp[1], resp[2:]
        def read(op):
            status, data = command(op)
            if status not in (0, 4, 5) or len(data) != len(FIELDS):
                raise RuntimeError('configuration read failed: ' + STATUS.get(status, str(status)))
            return status, decode(data)
        if args.action == 'show':
            _, active = read(0)
            state, saved = read(1)
            print(json.dumps({'active': active, 'saved': saved if state == 0 else None,
                              'saved_status': STATUS[state],
                              'pending_restart': state == 0 and saved != active}, indent=2))
        elif args.action == 'pinout':
            _, active = read(0)
            state, saved = read(1)
            with open(args.map, encoding='utf-8') as f:
                connector = json.load(f)
            print('Active configuration:')
            print(format_pinout(active, connector))
            if state == 0 and saved != active:
                print('\nSaved configuration (pending reboot):')
                print(format_pinout(saved, connector))
            elif state in (4, 5):
                print('\nSaved settings: ' + STATUS[state])
        elif args.action in ('set', 'reset'):
            settings = DEFAULT if args.action == 'reset' else json.load(open(args.file, encoding='utf-8'))
            status, _ = command(2, encode(settings))
            if status:
                raise RuntimeError('save failed: ' + STATUS.get(status, str(status)))
            state, saved = read(1)
            if state or saved != settings:
                raise RuntimeError('save readback did not match')
            print('Saved and read back. Reboot Zero to apply; use verify afterward.')
        elif args.action == 'verify':
            expected = json.load(open(args.file, encoding='utf-8'))
            encode(expected)
            _, active = read(0)
            state, saved = read(1)
            if active != expected or state != 0 or saved != expected:
                raise RuntimeError('active and/or saved settings do not match the file')
            print('Active and saved settings match the file.')
        else:
            if args.seconds <= 0 or args.seconds > 10:
                raise ValueError('LED test duration must be >0 and <=10 seconds')
            role = ('error', 'jtag', 'swd', 'ready', 'reserved').index(args.role)
            status, _ = command(3, bytes([role, 1]))
            if status:
                raise RuntimeError('LED unavailable')
            try:
                time.sleep(args.seconds)
            finally:
                command(3, bytes([role, 0]))
    finally:
        usb.util.release_interface(dev, 0)
        usb.util.dispose_resources(dev)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, usb.core.USBError, json.JSONDecodeError) as exc:
        print(f'error: {exc}', file=sys.stderr)
        sys.exit(1)
