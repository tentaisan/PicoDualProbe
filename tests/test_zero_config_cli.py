#!/usr/bin/env python3
"""Host-only tests of settings utility's wire format and verification."""
import contextlib
import importlib.util
import io
import json
import pathlib
import sys
import tempfile
from unittest.mock import patch

root = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('zero_config', root / 'tools/zero_config.py')
mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
assert mod.decode(mod.encode(mod.DEFAULT)) == mod.DEFAULT
try:
    mod.encode({'swclk': 1})
    raise AssertionError('incomplete config accepted')
except ValueError:
    pass

class Device:
    iSerialNumber = 1
    def __init__(self): self.pending = None; self.saved = None
    def write(self, ep, data, timeout):
        assert ep == 4 and data[0] == 0x81
        op = data[1]
        if op == 0: self.pending = bytes([0x81, 0]) + mod.encode(mod.DEFAULT)
        elif op == 1: self.pending = bytes([0x81, 0 if self.saved else 4]) + (self.saved or mod.encode(mod.DEFAULT))
        elif op == 2: self.saved = bytes(data[2:]); self.pending = bytes([0x81, 0])
        else: raise AssertionError('unexpected command')
    def read(self, ep, n, timeout):
        if self.pending is None: raise mod.usb.core.USBTimeoutError('idle')
        r, self.pending = self.pending, None
        return r

dev = Device()
def run(args):
    with patch.object(sys, 'argv', ['zero_config.py', '--serial', 'test'] + args), \
         patch.object(mod.usb.core, 'find', return_value=[dev]), \
         patch.object(mod.usb.util, 'get_string', return_value='test'), \
         patch.object(mod.usb.util, 'claim_interface'), patch.object(mod.usb.util, 'release_interface'), \
         patch.object(mod.usb.util, 'dispose_resources'), contextlib.redirect_stdout(io.StringIO()) as out:
        mod.main(); return out.getvalue()
with tempfile.TemporaryDirectory() as d:
    f = pathlib.Path(d) / 'cfg.json'; f.write_text(json.dumps(mod.DEFAULT))
    assert 'no saved settings' in run(['show'])
    pinout = run(['pinout'])
    assert 'row 1, column 1: GP1 ' in pinout and 'SWCLK / TCK' in pinout
    assert 'row 1, column 2: GP2 ' in pinout and 'SWIO' in pinout
    assert 'GP17: SWO' in pinout
    changed = dict(mod.DEFAULT); changed['led_enabled'] &= ~1; changed['led_active_high'] = 0
    physical = json.loads((root / 'tools/zero-connector.json').read_text())
    physical['leds'] = [{'name': 'Example LED', 'gpio': 9, 'color': 'orange', 'other_end': 'GND'}]
    rendered = mod.format_pinout(changed, physical)
    assert 'LED ERROR (disabled)' in rendered
    physical['rows'][0][0] = {'pin': 'GP1', 'wire_color': 'orange'}
    rendered = mod.format_pinout(changed, physical)
    assert 'row 1, column 1: GP1  SWCLK / TCK / RVSWD CLK; wire: orange' in rendered
    changed['swclk'] = 18; changed['swdio'] = 19
    rendered = mod.format_pinout(changed, physical)
    assert 'row 1, column 1: GP1  unassigned; wire: orange' in rendered
    physical['rows'][0][0]['wire_color'] = 3
    try:
        mod.format_pinout(changed, physical)
        raise AssertionError('invalid wire color accepted')
    except ValueError:
        pass
    assert 'polarity differs from recorded wiring' in rendered
    pending = dict(mod.DEFAULT); pending['swo_enabled'] = 0
    dev.saved = mod.encode(pending)
    assert 'Saved configuration (pending reboot)' in run(['pinout'])
    other_map = pathlib.Path(d) / 'alternate.json'
    other_map.write_text(json.dumps({'name': 'Alternate', 'rows': [[
        {'pin': 'GP2', 'wire_color': 'yellow'}, 'GND']],
        'leds': [{'name': 'Front indicator', 'gpio': 11, 'color': 'green', 'other_end': 'GND'}]}))
    output = run(['pinout', '--map', str(other_map)])
    assert 'Alternate' in output and 'GP2  SWDIO / SWIO / RVSWD IO; wire: yellow' in output
    assert 'Front indicator: GP11 green; LED SWD' in output
    assert 'Saved configuration (pending reboot)' in output
    dev.saved = None
    assert 'Reboot' in run(['set', str(f)])
    assert 'match' in run(['verify', str(f)])
    bad = dict(mod.DEFAULT); bad['tdi'] = 99; f.write_text(json.dumps(bad))
    try:
        run(['verify', str(f)])
        raise AssertionError('mismatched active values accepted')
    except RuntimeError:
        pass
print('Zero settings utility tests passed')
