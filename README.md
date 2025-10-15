# Sonic Pi for Agents

An AI-optimized fork of [Sonic Pi](https://sonic-pi.net) designed for seamless interaction with AI coding agents.

Designed with ❤️ by oO. Coded with ✨ by Claude Code


## What is this?

**Sonic Pi for Agents** is a live coding music synthesizer optimized for AI agents (like Claude Code, GitHub Copilot, Cursor, etc.) to compose and modify music code effectively.

Based on **Sonic Pi v4.5.x** with full DSL compatibility.

## Quick Start

```bash
cd app
./mac-build-all.sh              # macOS
./linux-build-all.sh            # Linux
win-build-all.bat               # Windows

# Run
open ./build/gui/Sonic\ Pi\ for\ Agents.app  # macOS
```

## Documentation

- **[sonic-pi/README.md](sonic-pi/README.md)** - Full Sonic Pi documentation
- **[CLAUDE.md](CLAUDE.md)** - Architecture and development guide for Claude Code
- **[sonic-pi/BUILD-MAC.md](sonic-pi/BUILD-MAC.md)** - macOS build instructions
- **[sonic-pi/BUILD-LINUX.md](sonic-pi/BUILD-LINUX.md)** - Linux build instructions
- **[sonic-pi/BUILD-WINDOWS.md](sonic-pi/BUILD-WINDOWS.md)** - Windows build instructions

## Key Differences

- Application name: "Sonic Pi for Agents"
- Version: v0.2.0 (independent from upstream)
- Agent-optimized error messages and logging
- All original Sonic Pi docs preserved in `sonic-pi/`

## License

Same as Sonic Pi. See [sonic-pi/LICENSE.md](sonic-pi/LICENSE.md).

## Credits

- **Fork**: Olivier "oO" Ozoux (olivier@ozoux.com)
- **Original Sonic Pi**: Sam Aaron and [contributors](sonic-pi/CONTRIBUTORS.md)
- **Upstream**: https://github.com/sonic-pi-net/sonic-pi
