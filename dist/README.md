# Verified Time 2 packages

| File | Version | Design |
|---|---|---|
| [orbit-atlas.pbw](orbit-atlas.pbw) | 1.0.0 | Pacific globe, dark background, small JST card |
| [pacific-press.pbw](pacific-press.pbw) | 1.0.1 | White background, ImageGen wave, ocean-navy Pacific time/date |

These are the exact packages installed and visually checked on the physical
Pebble Time 2. They target `emery` and have distinct UUIDs, allowing both to stay
installed. SHA256 hashes and emulator check counts are in
[verification.json](verification.json).

With the mobile app connected and Dev Connection set to CloudPebble:

```sh
pebble install --cloudpebble dist/orbit-atlas.pbw
pebble install --cloudpebble dist/pacific-press.pbw
```

Keep the mobile app in the foreground until installation finishes. Select the
new face in Watchfaces if the previous face stays active. The font license is
embedded in each package as a raw resource.
