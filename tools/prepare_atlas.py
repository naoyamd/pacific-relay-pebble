"""Stage the approved ORBIT ATLAS variant as a separate Pebble project."""
import json
import shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent
DEST=ROOT/'work'/'atlas'
(DEST/'src'/'c').mkdir(parents=True,exist_ok=True)
for source in (ROOT/'src'/'atlas').glob('*.[ch]'):
    shutil.copy2(source,DEST/'src'/'c'/source.name)
for name in ('timezone.c','timezone.h'):
    shutil.copy2(ROOT/'src'/'c'/name,DEST/'src'/'c'/name)
shutil.copy2(ROOT/'wscript',DEST/'wscript')
shutil.copytree(ROOT/'resources'/'atlas',DEST/'resources',dirs_exist_ok=True)
package=json.loads((ROOT/'package.json').read_text(encoding='utf-8'))
package.update(name='pacific-orbit-atlas',version='1.0.0')
package['pebble'].update(displayName='ORBIT ATLAS',uuid='bf0d7c6b-b3ec-4be0-ae3a-95c04d858e10')
media=[dict(type='bitmap',name='PACIFIC_EARTH',file='images/pacific-earth-124.png',
            memoryFormat='4BitPalette',spaceOptimization='memory')]
for name,size,chars,tracking in [
    ('TIME',54,'[0-9:]',-3),('WEEKDAY',23,'[A-Z]',0),('DAY',28,'[0-9]',0),
    ('JAPAN',18,'[0-9:]',-1),('ZONE',12,'[A-Z/ ]',0),
    ('STATUS',11,'[0-9,%]',0),('SMALL',9,'[A-Z]',0),('TINY',8,'[0-9A-Z/+ ]',0)]:
    media.append(dict(type='font',name=f'FONT_{name}_{size}',file='fonts/DejaVuSans-Bold.ttf',
                      characterRegex=chars,trackingAdjust=tracking))
media.append(dict(type='raw',name='FONT_LICENSE',file='fonts/LICENSE.txt'))
package['pebble']['resources']['media']=media
(DEST/'package.json').write_text(json.dumps(package,indent=2)+'\n',encoding='utf-8')
print(f'Atlas project ready: {DEST}')
