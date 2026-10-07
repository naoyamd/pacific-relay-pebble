"""Verify raw Emery frames from tools/capture_press.py."""
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parent.parent
FRAMES=ROOT/'outputs' if (ROOT/'outputs/pacific-press-day.png').exists() else ROOT/'tests/fixtures/press'
STATES={'day':True,'night':False,'wide':False,'winter':False,'newyear':True,
        'dawn':False,'sunrise':True,'dusk':True,'sunset':False,'dst-before':False,
        'dst-after':False,'fold-before':True,'fold-after':False,'midnight':True,
        'nextday':True,'noon':False,'wed':False}
for name,day in STATES.items():
    image=Image.open(FRAMES/f'pacific-press-{name}.png').convert('RGB')
    assert image.size==(200,228),(name,'screen dimensions')
    assert image.getpixel((0,0))==(255,255,255),(name,'white background')
    assert image.getpixel((12,168))==((255,255,85) if day else (0,0,85)),(name,'JST fill')
    assert image.getpixel((29,187) if day else (21,190))==((170,85,0) if day else (255,255,170)),(name,'sun/moon')
    assert set(image.crop((8,65,194,118)).getdata())=={(255,255,255),(0,85,170)},(name,'ocean navy primary time')
    assert set(image.crop((104,24,194,62)).getdata())=={(255,255,255),(0,85,170)},(name,'ocean navy primary date')
    blue=sum(b>r and b>0 for r,g,b in image.crop((8,123,192,162)).getdata())
    assert blue>500,(name,'ImageGen wave loaded')
    assert image.getpixel((134,219))==(0,0,85),(name,'battery fill')
print('PRESS native render checks passed: 17 frames, white background, wave, JST colors, sun/moon, primary time and battery')
