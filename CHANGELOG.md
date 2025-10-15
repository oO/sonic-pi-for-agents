# Changelog

All notable changes to Sonic Pi for Agents will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

For the original Sonic Pi changelog, see [sonic-pi/CHANGELOG.md](sonic-pi/CHANGELOG.md).

## [Unreleased]

## [0.1.1] - 2025-10-15

### Added
- Added DEV-SPEC.md - comprehensive development specification for agent-first architecture

### Changed
- Updated startup.html to display "Sonic Pi for Agents" branding
- Disabled splash screen on launch for faster startup

## [0.2.0] - 2025-10-15

### Added
- Added livecode.log functionality to capture GUI output pane messages to disk
- Added `LivecodeLogPath` to SonicPiPath enum in API
- Integrated log file with rotation system via GetLogs function
- Implemented logging for both Info and Multi messages in qt_api_client
- Updated CLAUDE.md with correct build paths and vcpkg information
- Reorganized documentation structure (moved original docs to `sonic-pi/`)

## [0.1.0] - 2025-10-15

### Added
- Initial fork from Sonic Pi v4.5.x
- Renamed application to "Sonic Pi for Agents"
- Added new README.md for the fork
- Added CHANGELOG.md for tracking fork changes
- Added CLAUDE.md for AI agent guidance
- Independent version numbering starting at v0.1.0

### Changed
- Application binary name: `Sonic Pi for Agents.app` (macOS)
- Updated build paths to reflect new application name

[Unreleased]: https://github.com/yourusername/sonic-pi-for-agents/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/yourusername/sonic-pi-for-agents/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/yourusername/sonic-pi-for-agents/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/yourusername/sonic-pi-for-agents/releases/tag/v0.1.0
