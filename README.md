# quick_dpi_tester

A minimal Win32 tool that counts raw mouse movement reports. Use it to measure
a mouse's **true DPI (CPI)** empirically instead of trusting the number printed
on the box or in the driver.

It registers a raw input device (`RegisterRawInputDevices`) and accumulates the
relative `lLastX` / `lLastY` counts from every `WM_INPUT` mouse report, so you
get the exact hardware counts with no acceleration, scaling, or pointer ballistics
in the way.

![one window, count and log](#) <!-- add a screenshot if you like -->

## Why

The DPI a mouse advertises and the DPI it actually reports often disagree. To
check it, you only need one number: how many counts the sensor emits when you
slide the mouse a known physical distance.

## Measuring true DPI

1. Set the mouse to the DPI you want to verify (e.g. 800).
2. Lay a ruler along your mousepad and line the mouse up with the 0 mark.
3. Press **ENTER**, slide the mouse in a straight line over an exact distance
   (e.g. 10 inches), then press **ENTER** again to stop.
4. Read the `X` count for that session.

```
true DPI = X_count / distance_in_inches
```

So if a "800 DPI" setting yields 7600 counts over 10 inches, the real CPI is
**760**, not 800. Average several runs for a cleaner number, and keep the motion
as straight as possible so `Y` stays near zero.

The `reports` column shows how many `WM_INPUT` packets arrived during the
session, which is handy as a rough sanity check on polling.

## Controls

| Key     | Action                                |
| ------- | ------------------------------------- |
| `ENTER` | start / stop a count                  |
| `C`     | clear the log (only while stopped)    |
| `ESC`   | quit                                  |

Each completed count is appended to the on-screen log (newest first), so you can
do a batch of runs and read them all at once.

## Build

The repo ships only the source. Build it with either toolchain:

```bat
build.bat
```

`build.bat` tries MSVC (`cl.exe`) first and falls back to MinGW (`gcc`). Or build
by hand:

```bat
:: MSVC
cl /W4 /O2 dpi_counter.c user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS

:: MinGW
gcc -O2 -Wall -o dpi_counter.exe dpi_counter.c -luser32 -lgdi32 -mwindows
```

Prefer not to compile it yourself? Grab the prebuilt `dpi_counter.exe` from the
[Releases](../../releases) page.

## License

MIT
