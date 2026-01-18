
# SPEECHER

> 🔧 **Cross-platform CLI tool for merging text files and converting audio to text**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-green.svg)]()
[![Language](https://img.shields.io/badge/Language-C-blue.svg)]()

---

## 📖 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Project Structure](#-project-structure)
- [Building](#-building)
- [Usage](#-usage)
- [Configuration](#-configuration)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🎯 Overview

**SPEECHER** is a portable, single-executable tool that:
1. **Merges multiple text files** (`.txt`, `.log`, `.csv`, etc.) into one organized file.
2. **Converts audio files** (`.wav`, `.mp3`, `.ogg`) to text using local Whisper AI (offline).

The application is fully self-contained — on first run, it creates all necessary directories and downloads required dependencies (Whisper engine & models) automatically.

---

## ✨ Features

### Core Features
- 📄 **Text File Merger** — Combine multiple text files with customizable separators
- 🎵 **Audio to Text (STT)** — Offline speech recognition using Whisper AI
- ⚙️ **Highly Configurable** — Almost everything is customizable via INI config
- 🌍 **Cross-Platform** — Windows, Linux, macOS support
- 📦 **Portable** — Single executable, auto-creates working environment

### Technical Features
- 🔄 **Auto-setup** — Downloads Whisper engine and models automatically
- 📊 **Progress Bars** — Visual feedback for long operations
- 📝 **Advanced Logging** — Colored output, file logging, rotation
- 🛡️ **Robust Paths** — Handles paths correctly on Windows/Unix
- 🌐 **Multi-language UI** — English, Russian, Turkish, Japanese support

---

## 📁 Project Structure

```
SPEECHER/
├── speecher.exe        # Main executable
├── data/               # Auto-created on first run
│   ├── config.ini      # Configuration file
│   ├── input/          # Place input files here
│   ├── output/         # Results appear here
│   ├── logs/           # Log files
│   └── lang/           # Language files
└── lib/                # Dependencies
    └── whisper/        # Whisper engine & models
```

---

## 🔨 Building

### Prerequisites

| Platform | Requirements |
|----------|-------------|
| **Linux** | `gcc`, `make`, `curl`, `ffmpeg` (optional) |
| **macOS** | `clang` (Xcode CLI), `make`, `ffmpeg` (optional) |
| **Windows** | MinGW-w64 (`gcc`, `make`), `ffmpeg` (optional) |

> **Note:** `ffmpeg` is required for non-WAV audio formats (MP3, OGG, etc).

### Build Commands

```bash
# Clone repository
git clone https://github.com/yourusername/speecher.git
cd speecher

# 1. Setup development environment (auto-install dependencies)
make setup

# 2. Build Commands:

# Build for BOTH Windows and Unix (from Linux)
make dual

# Build for Windows only (from Linux)
make win

# Build for Unix only (Linux / macOS)
make unix

# Clean build files
make clean
```

### 🛠️ Advanced Tools

| Command | Description |
|---------|-------------|
| `make status` | Check installed tools and compiler versions |
| `make help` | Show help menu with all available commands |

---

---

## 🚀 Usage

### First Run

```bash
./speecher
```

On first run, SPEECHER will:
1. Create `data/` directory structure
2. Generate default `config.ini`
3. Prompt to install Whisper (if you select audio features)

### Interactive Menu

```
  ╔══════════════════════════════════════╗
  ║         SPEECHER v0.5.0              ║
  ╚══════════════════════════════════════╝

  >> [1] Merge text files
  >> [2] Merge audio files (STT)
  >> [3] Settings
  >> [4] View logs
  >> [0] Exit

  Select option: >>
```

### Quick Workflow

1. **Text Merge:**
   - Place text files in `data/input/`
   - Select Option `[1]`
   - Result will be in `data/output/`

2. **Audio Transcription:**
   - Place audio files in `data/input/`
   - Select Option `[2]`
   - (First time) Follow prompts to install Whisper and download a model
   - Result will be in `data/output/`

---

## ⚙️ Configuration

Configuration file: `data/config.ini`

```ini
[general]
language = en           # en, ru, tr, ja
show_emoji = true       # Enable/disable emoji icons

[paths]
input_dir = data/input
output_dir = data/output

[whisper]
model = small           # tiny, base, small, medium, large
language = auto         # auto, en, ru, etc.
threads = 4             # Number of CPU threads to use

[text_merger]
sort_order = name_asc   # name_asc, date_desc, size_desc
recursive = false       # Scan subdirectories
```

---

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

- **Code Style:** C99 standard, 4 spaces indentation.
- **Naming:** `snake_case` for functions/variables.
- **Platform:** Ensure code works on both Windows and Unix-like systems.

---

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- [whisper.cpp](https://github.com/ggerganov/whisper.cpp) — High-performance inference of OpenAI's Whisper automatic speech recognition (ASR) model
- [cURL](https://curl.se/) — Command line tool and library for transferring data with URLs

---

<p align="center">
  Made with ❤️ by Heysh1n
</p>
