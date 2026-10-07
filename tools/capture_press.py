"""Capture production PACIFIC PRESS on Emery; fixtures affect the emulator only."""
from pathlib import Path
from datetime import datetime,timedelta,timezone
import time
import png
from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.websocket import WebsocketTransport
from libpebble2.communication.transports.qemu.protocol import QemuBattery
from libpebble2.protocol.system import TimeMessage,SetUTC
from libpebble2.services.screenshot import Screenshot
from pebble_tool.sdk.emulator import get_emulator_info
from pebble_tool.commands.emucontrol import send_data_to_qemu,send_health_metric,HEALTH_METRIC_STEPS

ROOT=Path(__file__).resolve().parent.parent
info=get_emulator_info('emery')
if not info:
    raise SystemExit('Start the Emery emulator first')
pebble=PebbleConnection(WebsocketTransport(f"ws://127.0.0.1:{int(info['pypkjs']['port'])}/"))
pebble.connect();pebble.run_async()
version=pebble.firmware_version
print(f'Connected to local Emery simulator (firmware {version.major}.{version.minor})',flush=True)
for label,utc,steps,battery in [
    ('day','2026-10-03T00:45:00+00:00',8432,82),
    ('night','2026-10-02T15:45:00+00:00',1208,64),
    ('wide','2026-10-02T19:59:00+00:00',12345,100),
    ('winter','2026-11-01T09:30:00+00:00',12345,100),
    ('newyear','2027-01-01T07:59:00+00:00',8432,82),
    ('dawn','2026-10-02T20:59:00+00:00',8432,82),
    ('sunrise','2026-10-02T21:00:00+00:00',8432,82),
    ('dusk','2026-10-02T08:59:00+00:00',8432,82),
    ('sunset','2026-10-02T09:00:00+00:00',8432,82),
    ('dst-before','2026-03-08T09:59:00+00:00',328,45),
    ('dst-after','2026-03-08T10:00:00+00:00',328,45),
    ('fold-before','2026-11-01T08:59:00+00:00',542,69),
    ('fold-after','2026-11-01T09:00:00+00:00',542,69),
    ('midnight','2026-10-03T06:59:00+00:00',9821,18),
    ('nextday','2026-10-03T07:00:00+00:00',0,18),
    ('noon','2026-10-02T19:00:00+00:00',6482,78),
    ('wed','2026-10-07T19:59:00+00:00',12345,100)
]:
    epoch=int(datetime.fromisoformat(utc).timestamp())
    pebble.send_packet(TimeMessage(message=SetUTC(unix_time=epoch-1,utc_offset=0,tz_name='UTC')))
    time.sleep(1.2)
    send_health_metric(pebble.transport,HEALTH_METRIC_STEPS,steps)
    send_data_to_qemu(pebble.transport,QemuBattery(percent=battery,charging=False))
    time.sleep(1.1)
    image=Screenshot(pebble).grab_image()
    assert len(image)==228 and len(image[0])==600,'Expected 200x228 RGB framebuffer'
    assert tuple(image[0][:3])==(255,255,255),'Select PACIFIC PRESS in the emulator Watchfaces menu before capturing'
    japan_hour=datetime.fromtimestamp(epoch,timezone(timedelta(hours=9))).hour
    expected=(255,255,85) if 6<=japan_hour<18 else (0,0,85)
    assert tuple(image[168][12*3:13*3])==expected,(label,'JST card did not refresh')
    png.from_array(image,mode='RGB;8').save(str(ROOT/'outputs'/f'pacific-press-{label}.png'))
    print(f'Captured {label}: UTC {epoch}, steps {steps}, battery {battery}',flush=True)
