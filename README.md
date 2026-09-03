# Keywave

A lightweight, low-latency background daemon that plays mechanical keyboard and mouse click sounds on Linux.

Works globally across both **Wayland** and **X11** by reading directly from the Linux `evdev` subsystem.

---

## Features

- **Global Input Detection**: Works everywhere across desktop environments and Wayland/X11 compositors.
- **Mechvibes Soundpack Compatibility**: Supports standard Mechvibes / Wayvibes custom keyboard soundpacks.
- **Mouse Click Feedback**: Plays customizable click sounds on mouse button presses.
- **Configurable**: Configured via `$XDG_CONFIG_HOME/keywave/keywave.conf` (or `~/.config/keywave/keywave.conf`) with full CLI override support.
- **Ultra Low Latency**: Native C++17 implementation powered by `miniaudio`.

---

## Installation

### Prerequisites

- A modern C++17 compiler (`g++` or `clang++`)
- Make
- ALSA / PulseAudio / PipeWire audio development libraries
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

# Master audio playback volume (float: 0.0 to 1.0 or higher)
volume = 0.8

# Path to keyboard soundpack folder (containing config.json)
keyboard_pack = /path/to/soundpack

# Path to mouse click sound file
mouse_sound = /path/to/mouse-click.mp3
```

### Command-Line Arguments

Command-line flags override values specified in the configuration file:

```bash
Usage: keywave [OPTIONS]

Options:
  -c, --config <path>         Path to configuration file
  -v, --volume <float>        Audio playback volume (e.g. 0.8 or 1.0)
  -k, --keyboard-pack <path>  Path to keyboard soundpack directory
  -m, --mouse-sound <path>    Path to mouse sound audio file
  -h, --help                  Show this help message and exit
```

---

## License

Keywave is released under the [MIT License](LICENSE).
Copyright (c) 2026 Nguyễn Hồng Hiệp.
