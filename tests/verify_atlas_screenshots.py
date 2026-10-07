"""Read-only framebuffer checks; run with the Pebble SDK's Python/Pillow."""
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parent.parent
FRAMES=ROOT/'outputs' if (ROOT/'outputs/orbit-atlas-day.png').exists() else ROOT/'tests/fixtures/atlas'
STATES={'day':True,'night':False,'wide':False,'newyear':True,'dawn':False,
        'sunrise':True,'dusk':True,'sunset':False,'dst-before':True,'dst-after':False}
for name,day in STATES.items():
    image=Image.open(FRAMES/f'orbit-atlas-{name}.png').convert('RGB')
    assert image.size==(200,228), (name,'screen dimensions')
    assert image.getpixel((0,0))==(0,0,85), (name,'native background')
    assert image.getpixel((20,161))==((255,170,0) if day else (85,0,85)), (name,'JST fill')
    assert image.getpixel((11,170))==((255,170,0) if day else (170,170,255)), (name,'JST rail')
    # Time and AM/PM retain a quiet background above the independent art layer.
    colors=set(image.crop((7,62,195,112)).getdata())
    assert colors=={(0,0,85),(255,255,255)}, (name,'time/art separation')
    green=sum(1 for r,g,b in image.crop((112,116,200,208)).getdata() if g>r and g>b and g>=85)
    assert green>60, (name,'generated globe loaded')
    assert image.getpixel((134,217))==(170,170,170), (name,'battery fill')
print('Native render checks passed: 10 frames, exact dimensions, generated globe, JST colors, art separation, battery')
