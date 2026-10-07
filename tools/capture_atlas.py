"""Capture the selected ORBIT ATLAS with the SDK Python runtime in WSL.

Keep one connection: the CLI's per-command emulator handshake resets UTC.
Set each fixture one second before its minute, then let the real minute tick
refresh the production app. No test-only code is compiled into the PBW.
"""
from pathlib import Path
import time
import png
from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.websocket import WebsocketTransport
from libpebble2.communication.transports.qemu.protocol import QemuBattery
from libpebble2.protocol.system import TimeMessage, SetUTC
from libpebble2.services.screenshot import Screenshot
from pebble_tool.sdk.emulator import get_emulator_info
from pebble_tool.commands.emucontrol import send_data_to_qemu, send_health_metric, HEALTH_METRIC_STEPS

ROOT=Path(__file__).resolve().parent.parent
info=get_emulator_info('emery')
if not info:
    raise SystemExit('Start the Emery emulator and select ORBIT ATLAS first')
port=int(info['pypkjs']['port'])
pebble=PebbleConnection(WebsocketTransport(f'ws://127.0.0.1:{port}/'))
pebble.connect()
pebble.run_async()
# Finish the connection's firmware handshake before setting fixture time.
version=pebble.firmware_version
print(f'Connected to local Emery simulator (firmware {version.major}.{version.minor})',flush=True)
for label,epoch,steps,battery in [
    ('day',1790988300,8432,82),('night',1790955900,1208,64),
    ('wide',1790971140,12345,100),('newyear',1798790340,8432,82),
    ('dawn',1781557140,8432,82),('sunrise',1781557200,8432,82),
    ('dusk',1781600340,8432,82),('sunset',1781600400,8432,82),
    ('dst-before',1793523540,8432,82),('dst-after',1793523600,8432,82)
]:
    pebble.send_packet(TimeMessage(message=SetUTC(unix_time=epoch-1,utc_offset=0,tz_name='UTC')))
    time.sleep(1.2)
    send_health_metric(pebble.transport,HEALTH_METRIC_STEPS,steps)
    send_data_to_qemu(pebble.transport,QemuBattery(percent=battery,charging=False))
    time.sleep(1.1)
    image=Screenshot(pebble).grab_image()
    target=ROOT/'outputs'/f'orbit-atlas-{label}.png'
    assert len(image)==228 and len(image[0])==600, 'Expected 200x228 RGB framebuffer'
    png.from_array(image,mode='RGB;8').save(str(target))
    print(f'Captured {label}: UTC {epoch}, steps {steps}, battery {battery}',flush=True)
