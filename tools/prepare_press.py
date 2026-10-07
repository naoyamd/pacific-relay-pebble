"""Stage PACIFIC PRESS, sharing the tested Pacific/Japan clock and existing font."""
import json
import shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent
DEST=ROOT/'work'/'press'
(DEST/'src'/'c').mkdir(parents=True,exist_ok=True)
shutil.copy2(ROOT/'src'/'press'/'main.c',DEST/'src'/'c'/'main.c')
for name in ('clock.c','clock.h'):
    shutil.copy2(ROOT/'src'/'atlas'/name,DEST/'src'/'c'/name)
for name in ('timezone.c','timezone.h'):
    shutil.copy2(ROOT/'src'/'c'/name,DEST/'src'/'c'/name)
shutil.copy2(ROOT/'wscript',DEST/'wscript')
shutil.copytree(ROOT/'resources'/'press',DEST/'resources',dirs_exist_ok=True)
(DEST/'resources'/'fonts').mkdir(exist_ok=True)
for name in ('DejaVuSans-Bold.ttf','LICENSE.txt'):
    shutil.copy2(ROOT/'resources'/'atlas'/'fonts'/name,DEST/'resources'/'fonts'/name)
package=json.loads((ROOT/'package.json').read_text(encoding='utf-8'))
package.update(name='pacific-press-pebble',version='1.0.1')
package['pebble'].update(displayName='PACIFIC PRESS',uuid='aef4bb67-6838-43f8-9f52-59ff93b15fbc')
media=[dict(type='bitmap',name='PACIFIC_WAVE',file='images/wave-white.png',
            memoryFormat='4BitPalette',spaceOptimization='memory')]
for name,size,chars,tracking in [
    ('TIME',54,'[0-9:]',-3),('WEEKDAY',17,'[A-Z]',-2),('DAY',35,'[0-9]',-2),
    ('JAPAN',18,'[0-9:]',-1),('LABEL',11,'[A-Z]',0),('SMALL',9,'[A-Z/ ]',0),
    ('STATUS',10,'[0-9,%]',0),('TINY',8,'[0-9A-Za-z/+ ]',0)]:
    media.append(dict(type='font',name=f'FONT_{name}_{size}',file='fonts/DejaVuSans-Bold.ttf',
                      characterRegex=chars,trackingAdjust=tracking))
media.append(dict(type='raw',name='FONT_LICENSE',file='fonts/LICENSE.txt'))
package['pebble']['resources']['media']=media
(DEST/'package.json').write_text(json.dumps(package,indent=2)+'\n',encoding='utf-8')
print(f'Press project ready: {DEST}')
