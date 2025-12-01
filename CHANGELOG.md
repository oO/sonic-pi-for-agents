# Changelog

All notable changes to Sonic Pi for Agents will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

For the original Sonic Pi changelog, see [sonic-pi/CHANGELOG.md](sonic-pi/CHANGELOG.md).

## [Unreleased]

## [0.3.2] - 2025-12-01

### Added
- Documented Conductor Pattern solution for temporal awareness (agent reads song position from status.json)
- Documented Cue System design for scheduled execution (comment-based timing annotations)
- Marked related user stories as solved in DEV-SPEC.md

## [0.3.1] - 2025-12-01

### Fixed
- Removed --yjit Ruby flag that caused startup crash on Ruby 2.6 (YJIT requires Ruby 3.1+)
- Added null check in splashClose() to prevent crash when splash screen is disabled

## [0.3.0] - 2025-11-05

### Added
- Auto-save functionality with 300ms debounce timers for each workspace buffer
- Deferred default project loading via QTimer::singleShot to fix constructor race conditions
- External change detection that stops auto-save to avoid conflicts
- Fallback to legacy workspace loading if project creation fails

### Changed
- Replaced immediate loadWorkspaces() call with deferred loadDefaultProject() in constructor

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
