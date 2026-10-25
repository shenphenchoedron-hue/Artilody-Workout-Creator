**Artilody Workout Creator** is a free, open-source (GPL-3.0) desktop app for creating structured cycling and rowing workouts on any computer – with the same Workout Builder as [Artilody Watts Trainer](https://github.com/shenphenchoedron-hue/Artilody-Watts-Trainer). Save workouts as Artilody `.awt` files or export them for Zwift (`.zwo`).

## What's new in {{VERSION}}

- **Everything in % of FTP**: block cards, *Interval Settings* and the chart show power as % of FTP, so a workout scales to every rider's FTP. Watts are only shown as a hint.
- **Interval sets with repeats**: *Intervals* asks for the number of repeats, the interval time and % FTP and the recovery time and % FTP, e.g. 5 × 30 s @ 120 % with 1 min recovery @ 55 %.
- **Ramps and staircases**: *Ramp / Steps* builds a rising or falling staircase (number of steps, time per step, first and last step in % FTP) or one smooth ramp. Every step stays editable on its own.
- **Select several blocks**: <kbd>Ctrl</kbd>/<kbd>⌘</kbd>-click, <kbd>Shift</kbd>-click or <kbd>Ctrl</kbd>/<kbd>⌘</kbd>+<kbd>A</kbd>, then move them together (drag, ‹ ›, <kbd>Alt</kbd>/<kbd>⌥</kbd>+<kbd>←</kbd>/<kbd>→</kbd>, right-click menu), duplicate or delete them, or change duration, power, cadence or RPE for all of them at once.
- **Export all workouts**: *Export → Export all* writes every workout (or the ones matching your search / filter) as one `.zwo` or `.awt` file each into a folder, or copies them all into Zwift's workouts folder.
- Your existing workouts are not changed: the `.awt` / `.zwo` formats are the same as in 1.0.0.

## Features

- **The Artilody Workout Builder**: Warm Up, Steady, Intervals, Recovery, Cool Down and Ramp blocks with duration, power in % FTP, ramps, cadence / stroke rate and RPE.
- **Easy reordering**: drag the ⋮⋮ grip strip, use the ‹ › buttons, the right-click menu (*Move earlier / later / to start / to end, Duplicate, Delete interval*) or <kbd>Alt</kbd>/<kbd>⌥</kbd>+<kbd>←</kbd>/<kbd>→</kbd>.
- **Cycling and rowing** workouts; watts preview from your cycling / rowing FTP; workout preview with estimated TSS.
- **File library** of your Workouts folder (default `~/Documents/Artilody Workouts`) with search, sport filter, mini power profiles and automatic refresh.
- **Import / export** `.awt` and `.zwo` files, and **copy straight into Zwift's workouts folder**.
- **Unsaved-changes protection**, keyboard shortcuts, dark and light theme.

## Downloads

| Platform | File |
|---|---|
| **Windows 10/11 (64-bit)**, installer | `ArtilodyWorkoutCreator-{{VERSION}}-windows-x64-setup.exe` |
| **Windows 10/11 (64-bit)**, portable ZIP | `ArtilodyWorkoutCreator-{{VERSION}}-windows-x64.zip` |
| **macOS 12 Monterey or newer** (Apple Silicon and Intel) | `ArtilodyWorkoutCreator-{{VERSION}}-macOS-universal.dmg` |
| **Debian 12 Bookworm** (amd64) | `artilody-workout-creator_{{VERSION}}_debian12_amd64.deb` |
| **Ubuntu 24.04 LTS** (amd64) | `artilody-workout-creator_{{VERSION}}_ubuntu24.04_amd64.deb` |
| **Raspberry Pi OS 64-bit (Bookworm)**: Raspberry Pi 3, 4, 5, 400 | `artilody-workout-creator_{{VERSION}}_raspberrypi-os-bookworm_arm64.deb` |
| Checksums | `SHA256SUMS.txt` (verify with `sha256sum -c SHA256SUMS.txt --ignore-missing`) |

## Installation

- **Windows**: run the setup, or extract the ZIP and start `Artilody Workout Creator.exe`. The app is not code-signed; if SmartScreen appears, click **More info → Run anyway**.
- **macOS**: open the DMG and drag the app to **Applications**. It is ad-hoc signed, not notarized: the first time, **right-click (Control-click) the app → Open → Open** (or run `xattr -cr "/Applications/Artilody Workout Creator.app"`).
- **Debian / Ubuntu / Raspberry Pi OS**: `sudo apt install ./artilody-workout-creator_{{VERSION}}_<platform>.deb`, then start *Artilody Workout Creator* from the application menu or run `artilody-workout-creator`.

## Using the workouts

- **Artilody Watts Trainer**: copy the `.awt` file to your training computer and use **Workouts → Import**.
- **Zwift**: export `.zwo` (or use *Copy into Zwift's workouts folder*); the file goes into `Documents/Zwift/Workouts/<your Zwift ID>/`. Zwift has no rowing, so rowing workouts are exported as bike workouts.

## Known limitations

- Builds are tested in CI (unit tests and an automated UI smoke test on every platform), but the packages have not yet been tried on many real machines – please report problems.
- `.zwo` files carry no sport, block names or RPE; use `.awt` to keep everything.

## Feedback

Found a bug or have an idea? Please open an issue: https://github.com/shenphenchoedron-hue/Artilody-Workout-Creator/issues
