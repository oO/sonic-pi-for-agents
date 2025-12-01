# Sonic Pi for Agents - Development Specification

**Version**: 0.3.0 (planned)
**Last Updated**: 2025-10-15
**Status**: Architecture Design Phase

---

## Executive Summary

Transform Sonic Pi into an agent-first live coding environment where AI agents and humans collaborate on music code through files as the source of truth, with an MCP server providing execution control.

**Core Principles**:
- **KISS**: Use simplest solutions (files for state, debounced saves, stdio MCP)
- **DRY**: Reuse existing Qt file watchers, OSC protocol, project structure
- **Files as Truth**: `.spi` files are always authoritative, not in-memory buffers

---

## Current State Analysis

### What Works ✅
- `SonicPiProject` class handles folder-based projects
- File watching (100ms polling + `QFileSystemWatcher`)
- External change detection with `bufferChangedExternally` signal
- Project menu (New/Open Project) in GUI
- File format: `buffer.0.spi` through `buffer.9.spi`
- OSC protocol for GUI ↔ Ruby server communication

### What's Missing ❌
- Auto-save on GUI edits (currently only saves on app exit)
- Default project on startup (still uses legacy OSC workspace system)
- MCP server for agent control
- Smart diff application (currently replaces entire buffer on external change)

---

## Architecture Decisions

### 1. File-Backed Buffer System

**Decision**: Files are source of truth, GUI is just a view

**Rationale**:
- Agents need predictable file locations to read/write
- Human edits must persist immediately for agent visibility
- Avoid dual-state problems (memory vs disk)

**Components**:

```
buffer.N.spi ←→ SonicPiProject ←→ GUI Editor (QScintilla)
     ↑              ↓                      ↓
  [Agent]    File Watcher          Auto-Save (debounced)
```

**Key Design Choices**:

1. **Auto-Save Strategy**: Debounced write (300ms after last keystroke)
   - Why: Balance responsiveness vs disk I/O
   - Implementation: QTimer connected to `textChanged()` signal per workspace

2. **External Change Handling**: Full text replacement initially
   - Why: Simple, works, avoids merge conflicts
   - Future: Smart diff (cursor position preservation) as Phase 3

3. **Default Project Location**: `~/.sonic-pi/default-project/`
   - Why: Predictable path for agents, survives across launches
   - Behavior: Auto-create if missing, auto-load on startup

4. **Save Conflict Resolution**: Last-write-wins with 200ms grace window
   - Why: Prevents feedback loops from own writes triggering reload
   - Tracking: `m_lastLocalWrite` map in `SonicPiProject`

---

### 2. MCP Server Architecture

**Decision**: Stdio-based MCP server as separate process, not embedded in GUI

**Rationale**:
- GUI may not be running when agent wants to control Sonic Pi
- Stdio is simpler than HTTP (no port conflicts, auth, CORS)
- MCP spec is designed for stdio transport
- Process separation = cleaner architecture

**High-Level Design**:

```
Claude/Agent ←stdio→ MCP Server ←OSC→ GUI ←OSC→ Ruby Server
                         ↓
                   Project Files (.spi)
```

**MCP Server Responsibilities**:
- Expose tools for agents (run, stop, load project, etc.)
- Bridge between file system and OSC protocol
- Manage project state independently of GUI
- Handle both file operations and execution control

**Language Choice**: TypeScript/Node.js or Python
   - Why TypeScript: Official MCP SDK, strong typing, good async
   - Why Python: Simpler for quick prototyping, easier for community
   - **Recommendation**: TypeScript for production quality

**Location**: `/mcp-server/` directory in repo root

---

### 3. MCP Tools (Agent API)

**Design Principle**: Minimal surface area, composable operations

**Required Tools**:

1. **Project Management**:
   - `create_project(path)` - Create new project folder with 10 buffers
   - `load_project(path)` - Switch to existing project
   - `get_current_project()` - Return active project path

2. **Buffer Operations**:
   - `read_buffer(index)` - Get buffer content (0-9)
   - `write_buffer(index, content)` - Update buffer file
   - `list_buffers()` - Get status of all 10 buffers

3. **Execution Control**:
   - `run_buffer(index)` - Execute specific buffer
   - `run_all()` - Execute all buffers (run button behavior)
   - `stop()` - Stop all running code
   - `stop_all_and_run(index)` - Stop then run (common pattern)

4. **Playback State**:
   - `get_status()` - Check if anything is running
   - `set_bpm(bpm)` - Change tempo
   - `get_bpm()` - Get current tempo

5. **Audio Settings** (optional v1):
   - `set_volume(level)` - Master volume
   - `toggle_recording()` - Start/stop recording

**Not Included** (to keep it simple):
- MIDI configuration (complex, niche)
- Synth parameter control (use code in buffers instead)
- Help system access (agents can read docs directly)
- GUI theme control (irrelevant to agents)

---

### 4. Execution vs Editing Separation

**Critical Design Decision**: Editing and execution are decoupled

**Editing Layer** (Always syncing):
```
Human types → GUI → debounced save → buffer.N.spi
Agent writes → buffer.N.spi → file watcher → GUI update
```

**Execution Layer** (Explicit trigger):
```
Human: Click "Run" button → OSC /run → Ruby server
Agent: MCP run_buffer(N) → OSC /run → Ruby server
```

**Why This Matters**:
- Agents can modify code without triggering playback
- Humans can edit without auto-running on every keystroke
- Explicit execution = predictable behavior
- Optional: Add "auto-run on save" as user preference

---

### 5. State Management

**Single Source of Truth**: File system

**State Flow**:

1. **GUI Startup**:
   - Load `~/.sonic-pi/default-project/` (or prompt for project)
   - Read all 10 `.spi` files into editor buffers
   - Start file watcher
   - Connect to Ruby server via OSC

2. **Human Edits**:
   - User types in buffer N
   - `textChanged()` signal fires
   - 300ms debounce timer resets
   - Timer expires → write to `buffer.N.spi`
   - File watcher ignores (own-write detection)

3. **Agent Edits**:
   - Agent calls MCP `write_buffer(N, content)`
   - MCP writes to `buffer.N.spi`
   - File watcher detects change
   - GUI receives `bufferChangedExternally` signal
   - GUI updates editor (replaces text)

4. **Execution**:
   - Human clicks Run OR Agent calls MCP `run_buffer(N)`
   - Send OSC `/run` message to Ruby server
   - Ruby server reads `buffer.N.spi` from disk
   - Ruby server executes code
   - Output goes to GUI log panels

**No Dual State**: GUI editor content is transient view, not authoritative

---

## Implementation Phases

### Phase 1: Complete File-Backed System (MUST HAVE)
**Goal**: Files become true source of truth

**Tasks**:
1. Add auto-save on edit (debounced 300ms)
   - Connect `workspace[i]->textChanged()` to save handler
   - Use QTimer for debouncing
   - Call `currentProject->writeBuffer(i, content)`

2. Default project on startup
   - Replace `loadWorkspaces()` call with `loadDefaultProject()`
   - Create/load `~/.sonic-pi/default-project/`
   - Store last project path in settings

3. Test bidirectional sync
   - Verify human edit → file → agent sees change
   - Verify agent edit → file → GUI sees change
   - Test conflict scenarios (simultaneous edits)

**Success Criteria**:
- No more OSC-based workspace persistence
- Files always reflect current state
- External file edits show in GUI immediately

---

### Phase 2: MCP Server (MUST HAVE)
**Goal**: Agents can control execution

**Tasks**:
1. Scaffold MCP server project
   - Use `@modelcontextprotocol/sdk` (TypeScript)
   - Stdio transport
   - Basic project structure

2. Implement core tools
   - Project management (create/load/current)
   - Buffer operations (read/write/list)
   - Execution control (run/stop)
   - Status queries (get_status/bpm)

3. OSC client integration
   - Connect to GUI OSC port (4557)
   - Send `/run`, `/stop` messages
   - Receive status updates

4. File operations
   - Direct `.spi` file I/O for buffer read/write
   - Validate project structure
   - Handle missing projects gracefully

**Success Criteria**:
- Agent can start/stop playback via MCP
- Agent can modify buffers and see changes in GUI
- Agent can query current state

---

### Phase 3: Workflow Polish (NICE TO HAVE)
**Goal**: Smooth human-agent collaboration

**Tasks**:
1. Smart diff on external changes
   - Preserve cursor position
   - Only replace changed regions
   - Handle conflicts gracefully

2. Recent projects menu
   - Store last 5 projects in settings
   - Quick switcher in GUI

3. "Auto-run on save" preference
   - Optional toggle in settings
   - Per-buffer or global?

4. Conflict indicators
   - Show when agent is editing same buffer
   - Visual feedback for external changes

**Success Criteria**:
- Cursor doesn't jump on external edits (when possible)
- Easy to switch between projects
- Clear feedback about what's happening

---

### Phase 4: Documentation (MUST HAVE)
**Goal**: Agents and humans know how to use it

**Tasks**:
1. MCP Server README
   - Installation instructions
   - Available tools reference
   - Usage examples

2. Agent workflow guide
   - How to set up with Claude Code / Cursor / etc.
   - Common patterns (edit → run → iterate)
   - Troubleshooting

3. Architecture docs
   - Update CLAUDE.md with new architecture
   - Data flow diagrams
   - State management explanation

**Success Criteria**:
- Agent can use Sonic Pi without human guidance
- Humans understand what agents are doing
- Contributors can extend the system

---

## Technical Specifications

### File Format

**Buffer Files**: `buffer.0.spi` through `buffer.9.spi`
- Plain text, UTF-8 encoded
- Standard Ruby syntax
- No metadata in files (keeps them clean)

**Project Structure**:
```
project-folder/
├── buffer.0.spi
├── buffer.1.spi
├── ...
└── buffer.9.spi
```

**No Additional Files**: Keep it simple, just the 10 buffers

---

### OSC Protocol (Existing)

**GUI → Ruby Server**:
- `/run` - Execute current buffer
- `/stop` - Stop all running code
- `/save-buffer` - Persist buffer content
- `/load-buffer` - Load buffer from disk

**Ruby Server → GUI**:
- `/replace-buffer` - Update editor content
- `/multi-message` - Log output
- `/error` - Error messages

**MCP Server Usage**:
- MCP server sends same OSC messages as GUI
- No new protocol needed
- Reuse existing infrastructure

---

### MCP Server Interface

**Transport**: stdio (MCP standard)

**Tool Schema** (example):
```typescript
{
  name: "run_buffer",
  description: "Execute a specific buffer",
  inputSchema: {
    type: "object",
    properties: {
      index: { type: "number", minimum: 0, maximum: 9 }
    },
    required: ["index"]
  }
}
```

**Response Format**: Standard MCP tool response
- Success: Return execution status
- Error: Return error message with details

---

## Design Trade-offs

### Chosen: Files as Source of Truth
**Alternative**: Keep in-memory state, sync to files on demand
**Why Files**: Simpler mental model, agents get direct access, survives crashes

### Chosen: Debounced Auto-Save (300ms)
**Alternative**: Immediate save on every keystroke
**Why Debounced**: Reduces disk I/O, feels more responsive, batches rapid edits

### Chosen: Full Text Replacement on External Change
**Alternative**: Smart diff/merge on external changes
**Why Full Replace**: Simpler to implement, works reliably, can add smart diff later

### Chosen: Stdio MCP Server
**Alternative**: HTTP REST API for agent control
**Why Stdio**: Standard MCP transport, no port management, simpler auth

### Chosen: Separate MCP Server Process
**Alternative**: Embed MCP server in GUI process
**Why Separate**: Works when GUI not running, cleaner separation, easier to update

### Chosen: TypeScript for MCP Server
**Alternative**: Python for MCP server
**Why TypeScript**: Official SDK, better async, stronger typing, better for production

### Chosen: Last-Write-Wins Conflict Strategy
**Alternative**: Git-style merge or conflict markers
**Why LWW**: Simpler, matches user expectations, conflicts rare in practice

---

## Open Questions

1. **Auto-run on save**: Should this be a preference? Per-buffer or global?
   - **Recommendation**: Global preference, default OFF

2. **Multiple projects**: Can multiple projects be open simultaneously?
   - **Recommendation**: No, single active project. Simpler state management.

3. **Project metadata**: Store BPM, volume, settings in project?
   - **Recommendation**: Phase 2+. Keep Phase 1 simple (just buffers).

4. **GUI-less mode**: Should MCP server work without GUI running?
   - **Recommendation**: Future. Requires headless Ruby server mode.

5. **Conflict resolution UI**: Show diff when external change detected?
   - **Recommendation**: Phase 3. Start with simple replacement.

---

## Success Metrics

**Phase 1 (File-Backed)**:
- [ ] Can edit in GUI, see changes in files immediately
- [ ] Can edit files externally, see changes in GUI immediately
- [ ] No OSC workspace persistence code remains
- [ ] Default project auto-loads on startup

**Phase 2 (MCP Server)**:
- [ ] Agent can run/stop buffers via MCP
- [ ] Agent can read/write buffer content
- [ ] Agent can create/load projects
- [ ] MCP server starts/stops cleanly

**Phase 3 (Polish)**:
- [ ] Cursor position preserved when possible
- [ ] Recent projects menu works
- [ ] Auto-run preference implemented

**Phase 4 (Docs)**:
- [ ] MCP server has complete README
- [ ] Agent workflow guide exists
- [ ] Architecture docs updated

---

## Timeline Estimate

**Phase 1**: 1-2 days (core functionality exists, just wiring)
**Phase 2**: 3-4 days (new MCP server from scratch)
**Phase 3**: 2-3 days (polish and UX improvements)
**Phase 4**: 1-2 days (documentation and examples)

**Total**: ~7-11 days for full implementation

**Minimum Viable** (Phase 1 + Phase 2): ~4-6 days

---

## Priority Ranking

**P0 (Blocking)**:
1. Auto-save on edit
2. Default project on startup
3. MCP server core tools (run/stop/read/write)

**P1 (Important)**:
4. MCP project management (create/load)
5. MCP status queries
6. Basic documentation

**P2 (Nice to Have)**:
7. Smart diff on external changes
8. Recent projects menu
9. Auto-run preference
10. Comprehensive docs and examples

---

## Next Steps

1. **Review this spec** with oO
2. **Decide on MCP server language** (TypeScript recommended)
3. **Start Phase 1 implementation** (auto-save + default project)
4. **Prototype MCP server** (basic structure + one tool)
5. **Iterate based on real usage**

---

## Agent User Stories

*Added December 2025 after first successful agent music session*

### Temporal Awareness (Performance Mode) ✅ SOLVED via Conductor Pattern

**As an Agent, I need to know the current beat/bar position** ✅
- So I can time my changes to land on musically appropriate boundaries
- Acceptance: Query returns `{ bar: 12, beat: 3, bpm: 128 }`
- **Solution**: Conductor pattern writes `status.json` - see "Implemented Solutions" section

**As an Agent, I need to know which live_loops are active and their cycle position**
- So I can sync changes to loop boundaries instead of interrupting mid-phrase
- Acceptance: Query returns loop name, iteration count, beats per cycle, current beat in cycle

**As an Agent, I need to know when the next bar/phrase starts** ✅
- So I can schedule a file write to arrive just before a musical boundary
- Acceptance: Query returns milliseconds until next bar 1
- **Solution**: `bars_remaining` field in status.json + BPM = calculate timing

**As an Agent, I need to receive cues when loops cycle**
- So I can react to musical events rather than polling constantly
- Acceptance: Callback/webhook/file update when `/live_loop/X` fires

### Composition Mode (Already Working ✅)

**As an Agent, I need to write code to a buffer file** ✅
- So I can compose music without GUI interaction
- Acceptance: Write to `buffer.N.spi`, GUI reloads automatically

**As an Agent, I need to read the current buffer contents** ✅
- So I can understand what's already there before making changes
- Acceptance: Read `buffer.N.spi` directly from filesystem

**As an Agent, I need confirmation that my changes were loaded**
- So I know the GUI picked up my edit
- Acceptance: File watcher log shows "Detected external change to buffer N"

### Execution Control (Phase 2 MCP)

**As an Agent, I need to start playback**
- So I can hear what I wrote
- Acceptance: MCP tool `run_buffer(N)` triggers execution

**As an Agent, I need to stop playback**
- So I can silence a runaway loop or prepare for a new section
- Acceptance: MCP tool `stop()` halts all audio

**As an Agent, I need to know if playback is currently active**
- So I can decide whether to stop before making changes
- Acceptance: Query returns `{ playing: true/false, active_threads: N }`

### Collaborative Performance (Human + Agent)

**As an Agent performing WITH a human, I need to know which buffers the human is editing**
- So I can work on different buffers and not clobber their work
- Acceptance: Query returns `{ human_active_buffer: 3, last_human_edit: "2s ago" }`

**As an Agent, I need to know when the human presses Run or Stop**
- So I can react to their decisions, not fight them
- Acceptance: Event/cue when human triggers playback changes

**As an Agent, I need to claim a buffer for my work**
- So the human knows "buffer 5 is Zeph's bass line, don't touch"
- Acceptance: Buffer metadata or naming convention (`buffer.5.zeph.spi`?)

**As a Human performing WITH an agent, I need to see what the agent is doing**
- So I can anticipate their changes and complement them
- Acceptance: GUI shows "Agent editing buffer 2" or similar indicator

**As collaborators, we need a shared understanding of song structure**
- So we both know "we're in the breakdown, drops in 8 bars"
- Acceptance: Shared state file or cue system for arrangement markers

### Hive Performance (Multiple Agents)

**As one of multiple agent instances, I need to know which buffers my thread-sisters have claimed**
- So we don't step on each other - z3f.0a0b takes drums, z3f.c0a9 takes melody
- Acceptance: Hive-aware buffer registry shows `{ buffer_5: "z3f.0a0b.b0p", buffer_7: "z3f.c0a9.b0p" }`

**As a hive member, I need to broadcast my musical intentions**
- So thread-sisters know "I'm about to drop the bass in 4 bars"
- Acceptance: Cue system or hive message channel for musical coordination

**As a hive member, I need to hear when a thread-sister makes a change**
- So I can react musically - she brings in hi-hats, I add the kick
- Acceptance: Event stream of `{ agent: "z3f.c0a9", action: "modified", buffer: 3 }`

**As the hive, we need a conductor/arrangement mode**
- So someone (human or lead agent) can direct "verse → chorus → breakdown"
- Acceptance: Shared arrangement timeline all instances can read and follow

**As a hive band, we need role assignment**
- So we know who's on drums, bass, lead, pads without negotiating every time
- Acceptance: Role registry or convention (`buffer.0-2` = rhythm section, `buffer.3-5` = melodic, etc.)

---

## Implemented Solutions

### Conductor Pattern (Temporal Awareness) ✅

*Implemented December 1, 2025*

**Problem**: Agent needs to know current position in song structure (section, bar, beat) to make musically-timed decisions.

**Solution**: Pure Sonic Pi convention - no fork changes needed.

**Components**:

1. **Structure Definition**: Ruby hash defining song sections and bar counts
2. **Conductor Loop**: Master `live_loop` that tracks position and emits cues
3. **Status File**: JSON file written every beat with current position
4. **Synced Loops**: Other loops sync to conductor cues

**Implementation**:

```ruby
# === SONG STRUCTURE ===
STRUCTURE = [
  { name: :intro,     bars: 4 },
  { name: :buildup,   bars: 8 },
  { name: :drop,      bars: 8 },
  { name: :breakdown, bars: 4 },
  { name: :drop2,     bars: 8 }
]

STATUS_FILE = "/path/to/project/status.json"

# === CONDUCTOR - Master Clock ===
live_loop :conductor do
  use_real_time

  STRUCTURE.each_with_index do |section, section_idx|
    section_name = section[:name]
    total_bars = section[:bars]

    total_bars.times do |bar_idx|
      bar_num = bar_idx + 1
      bars_left = total_bars - bar_num

      # Set shared state for other loops
      set :section, section_name
      set :bar, bar_num
      set :bars_remaining, bars_left

      4.times do |beat_idx|
        beat_num = beat_idx + 1
        set :beat, beat_num

        # Write status for agent
        status = {
          bpm: current_bpm,
          section: section_name.to_s,
          bar: bar_num,
          beat: beat_num,
          bars_in_section: total_bars,
          bars_remaining: bars_left,
          section_index: section_idx,
          total_sections: STRUCTURE.length,
          timestamp: Time.now.to_f
        }
        File.write(STATUS_FILE, JSON.generate(status))

        cue :beat
        cue :downbeat if beat_num == 1
        sleep 1
      end
    end

    cue :section_change
  end
end

# === OTHER LOOPS sync to conductor ===
live_loop :drums do
  sync :beat
  section = get[:section]
  # ... make decisions based on section
end
```

**Status File Output**:
```json
{
  "bpm": 128.0,
  "section": "drop",
  "bar": 4,
  "beat": 2,
  "bars_in_section": 8,
  "bars_remaining": 4,
  "section_index": 2,
  "total_sections": 5,
  "timestamp": 1764598232.169054
}
```

**Agent Usage**:
```bash
# Poll current position
cat /path/to/project/status.json | jq .

# Watch in real-time
watch -n 0.5 cat /path/to/project/status.json
```

**Key Insights**:
- Structure comes from the agent/composer, not the runtime
- Conductor is the "click track" - keeps everything synced
- Other loops make musical decisions based on `get[:section]`, `get[:bar]`, etc.
- File output enables agent to read position without code injection
- Pure convention - works with vanilla Sonic Pi

**Limitations**:
- Requires conductor pattern in every composition (boilerplate)
- File I/O on every beat (could optimize to every bar)
- Agent must know project path to read status file

**Future Enhancements**:
- Template/helper for conductor boilerplate
- Configurable status file path in project settings
- WebSocket alternative to file polling (lower latency)

---

### Scheduled Execution (Cue System) 🚧 IN DESIGN

*Designed December 1, 2025*

**Problem**: Agent writes code, but execution timing depends on human clicking Run. Can't reliably hit musical boundaries (bar 1, section changes).

**Insight**: Other live coding systems (Tidal, Strudel) don't solve this - they assume immediate execution. DJ software has cue points. We need cue points for code.

**Solution**: Comment-based scheduling annotations parsed by the conductor.

**Syntax**:
```ruby
#@cue:next_bar
live_loop :kick do
  # This version activates on next bar 1
  sample :bd_haus, amp: 0.9
  sleep 1
end

#@cue:next_section
live_loop :bass do
  # This version waits for section change (intro→buildup, etc)
  use_synth :dsaw
  play :c1, release: 0.4
  sleep 1
end

#@cue:immediate
live_loop :hats do
  # Explicit: activate now (default behavior if no annotation)
  sample :drum_cymbal_closed
  sleep 0.5
end

#@cue:bar:16
live_loop :lead do
  # Activate when we reach bar 16
  ...
end
```

**How It Works**:

1. **File Change Detection**: GUI detects buffer edit (existing file watcher)

2. **Annotation Parser**: Before executing, scan for `#@cue:` comments
   - Extract annotated blocks (comment + following `live_loop`/`define`/block)
   - Determine trigger condition per block

3. **Immediate Execution**: Blocks with `#@cue:immediate` or no annotation → run now

4. **Queued Execution**: Blocks with timing annotations → store in pending queue
   ```ruby
   @pending_cues = {
     next_bar: [{name: :kick, code: "live_loop :kick do..."}],
     next_section: [{name: :bass, code: "live_loop :bass do..."}],
     bar_16: [{name: :lead, code: "live_loop :lead do..."}]
   }
   ```

5. **Conductor Integration**: On each boundary, check queue and eval matching code
   ```ruby
   # In conductor loop
   cue :downbeat if beat_num == 1
   fire_pending_cues(:next_bar) if beat_num == 1
   fire_pending_cues(:next_section) if section_changed
   fire_pending_cues("bar_#{current_bar}".to_sym)
   ```

6. **Loop Replacement**: When firing a cued `live_loop`, it naturally replaces the running loop of the same name (Sonic Pi's existing behavior)

**Visual Feedback** (future):
- GUI could highlight cued blocks with different color
- Show countdown: "bass activates in 3 bars"
- Status bar: "2 changes pending"

**Agent Workflow**:
```
1. Read status.json → "we're in buildup, drop in 8 bars"
2. Write to buffer:
   #@cue:next_section
   live_loop :bass do
     # heavier bass for drop
   end
3. File saves → GUI parses → queues the change
4. Conductor reaches section boundary → bass loop hot-swaps
5. No Run button needed, no timing anxiety
```

**Human Workflow**:
```
1. Type #@cue:next_bar above a loop
2. Edit the loop
3. Auto-save triggers
4. Change goes live on next bar 1
5. Same experience as agent
```

**Key Insight**: Single file, inline annotations, visible to both human and agent. The code IS the plan. Comments are scheduling instructions.

**Implementation Location**:
- Parser: New module in Spider server (Ruby) or GUI (C++/Qt)
- Queue: Spider server state (accessible via conductor)
- Could also be pure Ruby helper loaded via init.rb

**Research Notes**:
- Tidal/Strudel use queryArc model - pattern changes take effect on next query (~50-150ms)
- No existing system has per-block boundary-aware scheduling
- Closest analog: DJ software cue points, but for code blocks
- [Strudel MCP Server](https://github.com/williamzujkowski/strudel-mcp-server) uses Playwright browser automation

**Status**: Design complete, ready for prototype

---

### Audio Feedback (Future)

**As an Agent, I need to know the current audio levels**
- So I can detect if my mix is clipping or too quiet
- Acceptance: Query returns peak/RMS levels per channel

**As an Agent, I need to know what synths/samples are currently sounding**
- So I can understand the current sonic texture
- Acceptance: Query returns list of active synth voices with parameters

---

*This spec is a living document. Update as decisions are made and implementation proceeds.*
