# Keywave

A lightweight, low-latency background daemon that plays mechanical keyboard and mouse click sounds on Linux.

Works globally across both **Wayland** and **X11** by reading directly from the Linux `evdev` subsystem.

---

## Features

- **Global Input Detection**: Works everywhere across desktop environments and Wayland/X11 compositors.
- **Smart Auto-Detection & Explicit Device Selection**: Automatically detects active keyboards and mice, or lets you specify exact hardware by device name or path.
- **Mechvibes Keyboard Soundpack Compatibility**: Supports standard Mechvibes / Wayvibes custom keyboard soundpacks.
- **Mouse Click Feedback**: Plays customizable click sounds on mouse button presses.
- **Ultra Low Latency**: Native C++17 implementation powered by [`miniaudio`](https://github.com/mackron/miniaudio).

---

## Installation

### Prerequisites

- A modern C++17 compiler (`g++` or `clang++`)
- Make
- ALSA / PulseAudio / PipeWire audio development libraries
- nlohmann_json library
- Permission to read `/dev/input/` (add your user to the `input` group):
  ```bash
  sudo usermod -aG input $USER
  ```
  *(Log out and log back in for group changes to take effect).*

### Build & Install

```bash
# Clone the repository
git clone https://github.com/justanoobcoder/keywave.git
cd keywave

# Build binary
make

# Run automated tests
make test

# Install globally (defaults to /usr/bin and /usr/share/man)
sudo make install
```

---

## Configuration

Keywave looks for a configuration file at `$XDG_CONFIG_HOME/keywave/keywave.conf` or `~/.config/keywave/keywave.conf`.

### Example `keywave.conf`

```ini
# Keywave Configuration File

# Master audio playback volume (float: 0.0 to 1.0)
volume = 0.8

# Path to keyboard soundpack folder (containing config.json)
keyboard_pack = /path/to/keyboard/soundpack/folder

# Path to mouse click sound file
mouse_sound = /path/to/mouse/click/sound/file.mp3

# (Optional) Specific device selection by name (run 'keywave -l' to see connected names)
# If left unset, Keywave will automatically detect your devices.
keyboard_device = ITE Tech. Inc. ITE Device(8176) Keyboard
mouse_device = Logitech M585/M590
```

### Command-Line Arguments

Command-line flags override values specified in the configuration file:

```bash
Usage: keywave [OPTIONS]

Options:
  -c, --config <path>             Path to configuration file
  -v, --volume <float>            Audio playback volume (e.g. 0.8 or 1.0)
  -k, --keyboard-pack <path>      Path to keyboard soundpack directory
  -m, --mouse-sound <path>        Path to mouse sound audio file
  -K, --keyboard-dev <name|path>  Keyboard device name (or /dev/input path)
  -M, --mouse-dev <name|path>     Mouse device name (or /dev/input path)
  -l, --list-devices              List available input devices and exit
  -h, --help                      Show this help message and exit
```

---

## Acknowledgments

This project is inspired by [wayvibes](https://github.com/sahaj-b/wayvibes). 
Special thanks to the original authors for their great work and ideas.

---

## License

Keywave is released under the [MIT License](LICENSE).

Copyright (c) 2026 Nguyen Hong Hiep.
