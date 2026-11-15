# PIONEER-TORIAZ-SP16-MIDI-PARAMETER-VST
PIONEER TORIAZ SP-16 MIDI PARAMETER EDITOR.
<img width="1484" height="1016" alt="{EB7481CE-B33A-4D5B-8438-C721606047D6}" src="https://github.com/user-attachments/assets/ab4ab84c-da5c-4668-80e4-3c5d63c65ebe" />

# still work inprogress
# Disable midi clock externet on sp-16
# ARP MODE DOESN'T WORK, WILL MAKE IT WORK SOON

make with AI complined with Juce via compliner script, make your own vst's with my compliner script: 
https://github.com/SannyGrooves/JUCE-VST-COMPLINER-SCRIPT

# SP-16 Controller (VST3)

A VST3 instrument plugin that turns notes from your DAW's piano roll into MIDI for the **Pioneer DJ TORAIZ SP-16**. Put it on an instrument track, route its MIDI output to the SP-16's MIDI port, and every control becomes automatable in the host.

> **Status: untested.** This code was written without being compiled or run against real hardware. Expect to fix a few compile errors on the first build.
>
> **No SP-16-specific messages are used.** There is no SysEx and there are no SP-16-specific CCs. The plugin only sends standard MIDI note on/off, pitch bend and channel messages (plus the standard All Notes Off controller when you press PANIC). It assumes the SP-16 responds to ordinary MIDI notes on the channel you choose.

## Features

- **Output MIDI channel** (1-16). Changing it releases all sounding notes first.
- **Chords**: Off, Major, Minor, Dim, Aug, Sus2, Sus4, Maj7, Min7, Dom7, Power.
- **Pitch shift** in semitones (-24..+24).
- **Pitch bend** (fine pitch, -1.0..+1.0 mapped to 0..16383).
- **Arpeggiator**: on/off, mode (Up, Down, Up/Down, Random, As Played), rate (1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32), octaves (1-4) and gate (0.05-1.0). It follows the host tempo (120 BPM if the host gives none).
- **PANIC** button, and an on-screen keyboard for auditioning.
- Every control is a host-automatable parameter (MIDI-learn it in your DAW).
- Notes are always released on the channel they started on: on channel change, arp toggle, transport stop, All Notes Off, PANIC, and when the host resets or deactivates the plugin.

## Interface

The editor uses a custom HISE-style look and feel: rotary arc knobs (MIDI Ch, Pitch Shift, Pitch Bend, Arp Octaves, Arp Gate; double-click resets, type a value in the read-out), glossy chord/mode buttons that light up orange, an ARP button with a green LED, an inset rate selector, a MIDI activity LED, a red PANIC button and a restyled keyboard. All layout and colours live in `PluginEditor.cpp` and `HiseLookAndFeel.cpp` (palette in the `sp16ui` namespace of `HiseLookAndFeel.h`).

## Requirements

- CMake 3.22 or newer, Git, a C++17 compiler.
- Windows: Visual Studio 2022 (or Build Tools) with the C++ workload.
- macOS: Xcode command line tools. Linux: GCC/Clang plus the usual JUCE dependencies (ALSA, X11, freetype, etc.).
- Internet access on the first configure (JUCE 8.0.4 is fetched with `FetchContent`).

## Build

### CMake (any platform)

```
cmake -B build -S .
cmake --build build --config Release --parallel
```

The plugin appears under `build/SP16Controller_artefacts/Release/VST3/`.

### PowerShell script (Windows)

```
.\build.ps1                    # Release build
.\build.ps1 -Config Debug      # Debug build
.\build.ps1 -Clean             # delete the build folder first
.\build.ps1 -Install           # copy to Common Files\VST3 (needs an Administrator PowerShell)
```

The script checks for `cmake`, `git` and the Visual Studio C++ tools, and prints the `winget` command for anything missing.

## Install locations (VST3)

| OS | Folder |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3` |
| macOS | `/Library/Audio/Plug-Ins/VST3` (all users) or `~/Library/Audio/Plug-Ins/VST3` |
| Linux | `~/.vst3` or `/usr/lib/vst3` |

Rescan plugins in your DAW afterwards.

## SP-16 setup

1. Connect the SP-16 to your computer (USB or a MIDI interface) so its MIDI input shows up in your DAW.
2. Create an **instrument track** with "SP-16 Controller" on it.
3. Route the plugin's **MIDI output** to the SP-16's MIDI port. How to do this differs per DAW (usually a "MIDI output" or "send MIDI to" setting on the track or plugin).
4. On the SP-16, enable **MIDI Note IN** and set its receive channel to match the plugin's **MIDI Ch** control. Check the SP-16 manual for where these settings live.
5. Draw notes in the DAW piano roll, or click the on-screen keyboard to test.

## Files

| File | Purpose |
|---|---|
| `CMakeLists.txt` | CMake project, fetches JUCE 8.0.4 |
| `build.ps1` | Windows build/install script |
| `README.md` | This file |
| `README.html` | Same content as one self-contained HTML page |
| `Source/PluginProcessor.h` | Processor class, parameter IDs, shared name lists |
| `Source/PluginProcessor.cpp` | Parameters, MIDI processing, chords, arpeggiator |
| `Source/PluginEditor.h` / `.cpp` | Editor window: layout, panels, attachments |
| `Source/HiseLookAndFeel.h` / `.cpp` | HISE-style look and feel: knobs, glossy buttons, LEDs, panels |

## Notes

- The plugin is declared as an instrument (`IS_SYNTH`) with a silent stereo output, because most DAWs only allow MIDI-out routing from instrument-type plugins.
- Incoming CC, aftertouch, program change and pitch wheel messages are forwarded to the output channel. An incoming pitch wheel can therefore override the Pitch Bend control until that control changes again.
- When two held input notes produce the same output note (overlapping chords), releasing one of them also ends that note for the other.
- When the transport stops, all held notes are released. Some DAWs also send their own note-offs, which is harmless.
- If a DAW deletes the plugin while a note is sounding, no MIDI can be sent after the last processed block. Stop the transport (or press PANIC) before removing the plugin.
- Chords and the arpeggiator allocate small vectors on the audio thread, as the design specifies. This is fine for normal use but not strictly real-time safe.
- Zip the folder: `Compress-Archive -Path SP16Controller -DestinationPath SP16Controller.zip` (Windows) or `zip -r SP16Controller.zip SP16Controller` (macOS/Linux).

