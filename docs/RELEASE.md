**Wander** is a free eight-voice synthesiser: two oscillators and noise, a filter, two envelopes, an LFO,
and drift, laid out like Ableton's Drift.

| Download | For |
|---|---|
| `Wander-macOS.zip` | macOS 11+, Apple Silicon and Intel: VST3, AU and a standalone app |
| `Wander-Windows.zip` | Windows 64-bit: VST3 and a standalone app |
| `Wander-REAPER-JSFX.zip` | REAPER only: the JSFX version with its presets |

Each zip has an `INSTALL.txt`. Short version (if a `VST3` folder does not exist yet, create it):

- **macOS:** `Wander.vst3` to `~/Library/Audio/Plug-Ins/VST3/`, `Wander.component` to
  `~/Library/Audio/Plug-Ins/Components/`, then once in Terminal:
  `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Wander.vst3 ~/Library/Audio/Plug-Ins/Components/Wander.component`
- **Windows:** the folder `Wander.vst3` to `C:\Program Files\Common Files\VST3\`
- Restart the DAW. In Ableton: Plug-Ins > Pavel Konrad > Wander.
