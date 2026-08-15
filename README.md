# PACIFIC RELAY

Retro flight-board watchface linking HND and SFO. In the published repository,
[`mockup.html`](mockup.html) is the 2x browser design specification beside this README; the
native Pebble implementation stays minimal in `src/c/`.

## Features

- emery-only 200x228 watchface with one Window and one custom drawing Layer
- 12-hour JST (UTC+9) as the primary time and independent SFO local date/time
- automatic US Pacific PST/PDT calculation and day/night SFO color bands
- HealthService steps, BatteryStateService charge, and minute ticks
- low-contrast globe, curved HND>SFO route, DATE LINE, and status band
- no JavaScript, settings screen, image resources, or external dependencies

## Build and install

From the repository root with a Pebble SDK/Rebble toolchain:

```text
pebble build
pebble install --phone <phone-ip>
```

The project is intentionally targeted only at `emery` (Pebble Time 2).

## DST rule

`src/c/timezone.c` evaluates US rules in UTC: the second Sunday in March at
10:00 UTC starts PDT, and the first Sunday in November at 09:00 UTC ends it.
`tests/test_timezone.py` contains the UTC boundary assertions and source
contract check. Run it with `python tests/test_timezone.py` when a C compiler
is unavailable.

## License

MIT; see `LICENSE`.
