# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## About Sonic Pi

Sonic Pi is a live coding music synthesizer - a programming environment designed for creating music and sounds through code. It combines Ruby-based DSL for composing music with SuperCollider for audio synthesis, connected through OSC (Open Sound Control) messaging.

## Build Commands

### macOS (Primary Platform)
```bash
cd app
./mac-build-all.sh              # Full build: prebuild + config + cmake
./mac-build-gui.sh              # GUI only
./mac-clean.sh                  # Clean build artifacts

# Run the built app:
open ./build/gui/Sonic\ Pi\ for\ Agents.app
# Or directly:
./build/gui/Sonic\ Pi\ for\ Agents.app/Contents/MacOS/Sonic\ Pi\ for\ Agents
```

### Linux
```bash
cd app
./linux-build-all.sh
./linux-build-gui.sh
./linux-clean.sh
```

### Windows
```bash
cd app
win-build-all.bat
win-build-gui.bat
win-clean.bat
```

### Dependencies
- **macOS**: Xcode (12.1+), Homebrew, Qt6 (6.2+), CMake (3.18+), Elixir (1.13+), pkg-config, vcpkg
  - Install via: `brew install qt@6 cmake elixir pkg-config`
  - vcpkg is automatically cloned and configured by `mac-pre-vcpkg.sh` (run by `mac-build-all.sh`)
- **All platforms**: See platform-specific BUILD-*.md files in `sonic-pi/` for detailed setup

## Testing

### Ruby Server Tests
```bash
cd app/server/ruby/test
rake test
```

If you get minitest errors, install test-unit:
```bash
gem install test-unit
```

### Test Structure
- Tests located in `app/server/ruby/test/`
- Organized by functionality: `lang/core/`, `lang/sound/`, etc.

## Architecture Overview

Sonic Pi is a multi-process, multi-threaded system with three main components:

### 1. GUI (Qt/C++)
- Location: `app/gui/`
- Built with Qt6 and QScintilla for code editing
- Communicates via OSC over TCP/UDP (default: UDP port 4557→4558)
- Handles: code editor, preferences, visual feedback, help system

### 2. Spider Server (Ruby)
- Entry point: `app/server/ruby/bin/spider-server.rb`
- Location: `app/server/ruby/`
- The language runtime and core evaluation engine
- Key modules:
  - `lib/sonicpi/runtime.rb` - Core runtime system
  - `lib/sonicpi/lang/` - Language DSL (core, sound, MIDI, western music theory)
  - `lib/sonicpi/studio.rb` - Audio/synth management
  - `lib/sonicpi/server.rb` - Main server coordination
- Communicates with GUI via OSC and with SuperCollider via OSC (port 4556)
- Manages: code execution, threading, timing, synth triggering, MIDI

### 3. Sound Engine (SuperCollider/Erlang)
- Location: `app/server/beam/` (Erlang scheduler), `app/server/native/` (scsynth binaries)
- SuperCollider (scsynth) handles actual audio synthesis
- Erlang-based scheduler (Tau) manages precise timing
- Synthdefs: `etc/synthdefs/` (both source designs and compiled `.scsynth` files)

### Communication Flow
```
User Code → GUI (Qt) → Spider Server (Ruby) → Tau (Erlang) → scsynth (SuperCollider) → Audio Output
                ↓ OSC                ↓ OSC                ↓ OSC
```

### Key Ruby Components
- **Runtime** (`runtime.rb`): Executes user code in isolated thread contexts
- **Studio** (`studio.rb`): Manages synths, FX, mixer, audio buses
- **Lang Modules**:
  - `lang/core.rb` - Control flow, timing (sleep, sync, cue)
  - `lang/sound.rb` - Sound generation (play, sample, synth, with_fx)
  - `lang/midi.rb` - MIDI input/output
  - `lang/western_theory.rb` - Music theory (scales, chords, notes)
- **OSC** (`osc/`): Custom OSC implementation for message passing
- **SynthInfo** (`synths/synthinfo.rb`): Metadata for all built-in synths and FX

## Project Structure

```
app/
├── gui/                    # Qt GUI application
│   ├── QScintilla_src-*/  # Bundled code editor component
│   ├── fonts/             # Typography assets
│   ├── images/            # Icons and graphics
│   ├── lang/              # Translation files
│   └── theme/             # Visual themes
├── server/
│   ├── ruby/              # Spider Server (language runtime)
│   │   ├── bin/           # Server executables
│   │   ├── lib/sonicpi/   # Core libraries
│   │   └── test/          # Test suite
│   ├── beam/              # Erlang Tau scheduler
│   └── native/            # Platform-specific scsynth binaries
├── external/              # Third-party dependencies
├── cmake/                 # CMake build configuration
└── config/                # User config examples

etc/
├── synthdefs/             # Synth and FX definitions
│   ├── designs/           # Source code (Overtone/SuperCollider)
│   └── compiled/          # Binary .scsynth files
└── examples/              # Example compositions

bin/                       # Utility scripts
```

## Synth Design

### Creating New Synths
Sonic Pi synths are defined as SuperCollider SynthDefs and compiled to binary `.scsynth` files.

**Preferred method**: SuperCollider (see `SYNTH_DESIGN.md`)
- Design synth in SuperCollider
- Use `writeDefFile()` to compile to binary
- Load with `load_synthdefs` in Sonic Pi

**Requirements**:
- Must output stereo to `out_bus` parameter
- Must self-terminate (use envelopes with `doneAction: 2`)

**Integration levels**:
1. **Loose**: Enable "external synths" in preferences, use `load_synthdefs`, call with string: `synth 'mysynth'`
2. **Tight**: Add metadata to `app/server/ruby/lib/sonicpi/synths/synthinfo.rb`, recompile, call with symbol: `synth :mysynth`

### Gated Synths
- Non-standard synths that sustain until explicitly released via `control sth, gate: 0`
- Located in `etc/synthdefs/compiled/gated/`
- Useful for MIDI keyboard integration
- Must be loaded explicitly with `load_synthdefs`

## Development Guidelines

### Code Organization
- Work in `dev` branch (not `stable`)
- All tests should pass before merging
- Keep changes focused and minimal
- Follow existing code style and patterns

### Language Specifics
- Ruby version: See `app/server/ruby/` - uses Ruby 2.1+ features
- Prefer simple, readable code over clever optimizations
- DSL design principle: "simple enough for a 10 year old"

### Common Patterns
- OSC messaging for inter-process communication
- Thread-based concurrency with `SThread` (Sonic Pi's thread wrapper)
- Time synchronization via logical time system (not wall clock)
- Resource management via allocators (buses, buffers, nodes)

## Configuration

### User Config Files
- Location: `~/.sonic-pi/config/`
- `init.rb` - Run on Spider Server startup
- Settings stored in `~/.sonic-pi/store/`
- Examples copied from `app/config/user-examples/`

### Server Ports (Default)
- GUI → Server: 4557
- Server → GUI: 4558
- scsynth: 4556
- OSC cues (external): 4560
- Tau (Erlang): 4561/4562

## Internationalization

Sonic Pi supports multiple languages via Weblate.
- Translation workflow: See `TRANSLATION.md` and `TRANSLATION-WORKFLOW.md`
- Language files: `app/gui/lang/`
- Tutorial translations separate from UI translations

## Important Notes

- **No development deadlines** - quality over speed
- **Minimize tech stack complexity** - prefer existing tools/languages
- **All code should align with project vision** - discuss large changes first
- **Documentation should be conversational** - avoid formal language
- **SuperCollider, not Overtone** - for new synth designs going into distribution

## Resources

- Community: https://in-thread.sonic-pi.net
- Main site: https://sonic-pi.net
- Issues: https://github.com/sonic-pi-net/sonic-pi/issues
- Features board: https://github.com/orgs/sonic-pi-net/projects/1
