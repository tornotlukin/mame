# Workshop: MAME Debugger × LLM Live Integration

**Started:** 2026-03-22
**Status:** In Progress

---

## The Idea

A **standalone, separate project** (not inside the MAME tree) that provides LLM-powered debugging for MAME. The user acts as the "eyes" — describing what's happening on screen — while the LLM acts as the "hands" — setting breakpoints, reading memory, tracing execution, and even live-patching code/data.

**Distribution model:** The project ships as a standalone repo containing:
1. **Drop-in replacement files** for `src/emu/debug/` and `src/osd/modules/debugger/` — users copy them over the originals in their MAME source tree and rebuild. No other MAME folders are touched.
2. **A Python LLM bridge** that connects to the enhanced debugger over TCP and talks to Claude.

The changes to the debugger files are **purely additive** — all existing functionality stays intact. We just add a TCP server module and a small output-capture hook.

---

## Open Questions

- [ ] WebSocket vs raw TCP? (WebSocket is easier for tooling, raw TCP is simpler in C++)
- [ ] Should the MAME server push async events (breakpoint hit, frame boundaries) or only respond to requests?
- [ ] What message format? JSON lines? Simple text protocol?
- [ ] Should the bridge support multiple simultaneous connections?
- [ ] How to handle MAME's single-threaded debugger model with async socket I/O?
- [ ] What Claude tools should the bridge expose? (need to define the tool schema)
- [ ] Should the user interact with Claude via the bridge CLI, or could this be an MCP server?

---

## Answers & Decisions

| # | Question | Decision | Date |
|---|----------|----------|------|
| 1 | Core interaction model | All three: chat-driven commands, autonomous exploration, live patching | 2026-03-22 |
| 2 | Where the LLM interface lives | LLM generates commands that MAME picks up via a server; user is the eyes, LLM is the hands | 2026-03-22 |
| 3 | LLM provider | Claude API (Anthropic) | 2026-03-22 |
| 4 | End goal | Personal power tool first; if it works, figure out how to contribute upstream | 2026-03-22 |
| 5 | Transport | TCP/WebSocket — real-time bidirectional connection | 2026-03-22 |
| 6 | C++ modification depth | Deep — willing to modify core debugger, add modules, build new frontend | 2026-03-22 |
| 7 | Project structure | Completely separate from MAME tree. Standalone repo with drop-in replacement debugger files + Python bridge. Users copy files into their MAME source, rebuild. No other MAME folders affected. | 2026-03-22 |
| 8 | UI approach | Don't change underlying structure — add new views, improve interactivity and readability on top of whatever frontend is already built. Use existing frontend toolkit (Win32/Qt/ImGui). | 2026-03-22 |
| 9 | New views wanted | LLM chat/log panel, annotated memory view, execution timeline, symbol/label manager, detachable/multi-window panel layout | 2026-03-22 |
| 10 | Readability pain points | Better color/highlighting for changed values, console output filtering, less dense layout with breathing room, proper text selection + multi-select + copy/paste of output lines | 2026-03-22 |
| 11 | Two-LLM architecture | Worker LLM (Haiku) is just a babysitter — launches MAME, maintains TCP connection, relays commands/results, keeps process alive. Main LLM (Opus/Sonnet) does ALL the thinking — assembly RE, game architecture, strategy, debugger commands. Needed because you can't run a long-lived process and do interactive LLM work in the same session. | 2026-03-22 |

---

## Architecture (Draft)

```
┌─────────────────────────────────────────────────────────┐
│                    USER (the eyes)                       │
│  Watches the game in MAME. Tells the main LLM what's   │
│  happening: "sprite flickered", "score changed to 5000" │
└──────────────┬──────────────────────────────────────────┘
               │ chat input
               ▼
┌─────────────────────────────────────────────────────────┐
│           MAIN LLM — Claude Opus/Sonnet (the brain)     │
│                                                         │
│  The smart one. Reasons about assembly (Z80, 6502,      │
│  68000, etc.), game architecture, reverse engineering.   │
│  Decides what debugger commands to issue.                │
│                                                         │
│  Tools available:                                       │
│  ├── execute_command(cmd) → raw debugger command        │
│  ├── read_memory(addr, len, space)                      │
│  ├── write_memory(addr, data, space)                    │
│  ├── set_breakpoint(addr, condition?, action?)          │
│  ├── clear_breakpoint(id)                               │
│  ├── set_watchpoint(addr, len, type, cond?)             │
│  ├── get_registers(cpu?)                                │
│  ├── step / step_over / step_out / go                   │
│  ├── disassemble(addr, count)                           │
│  ├── get_state() → execution state, PC, etc.            │
│  └── evaluate_expression(expr)                          │
│                                                         │
│  Commands go through the bridge → TCP → MAME            │
│  Results come back so Main LLM can reason about them    │
└──────────────┬──────────────────────────────────────────┘
               │ tool calls (via bridge process)
               ▼
┌─────────────────────────────────────────────────────────┐
│        WORKER LLM — Claude Haiku (the babysitter)       │
│                                                         │
│  NOT smart. Just keeps things running:                  │
│  ├── Launched MAME with -debug flag                     │
│  ├── Maintains the TCP connection to MAME               │
│  ├── Relays commands from Main LLM → MAME              │
│  ├── Relays results from MAME → Main LLM               │
│  ├── Keeps the process alive                            │
│  └── Restarts MAME if it crashes                        │
│                                                         │
│  No thinking. No strategy. Just a process manager.      │
└──────────────┬──────────────────────────────────────────┘
               │ TCP/WebSocket
               ▼
┌─────────────────────────────────────────────────────────┐
│              MAME + DEBUGGER (live on screen)            │
│                                                         │
│  User sees this running in real-time:                   │
│  ├── Game window — the user watches gameplay here       │
│  ├── Debugger window — commands appear live as the      │
│  │   Main LLM issues them                              │
│  ├── Memory/disasm/registers update in real-time        │
│  └── TCP server module accepts commands, sends results  │
│                                                         │
│  User sees breakpoints fire, memory highlight,          │
│  commands scroll in the console — all driven by the LLM │
└─────────────────────────────────────────────────────────┘
```

**Why two LLMs?**
You can't run a long-lived process (MAME) and have an interactive LLM conversation in the same CLI session. The worker (Haiku) owns the MAME process and connection. The main LLM (Opus/Sonnet) is free to think deeply about assembly code, game architecture, and reverse engineering strategy — which is critical because these games are complex.

---

## Component Breakdown

### Standalone Project Structure

```
mame-llm-debugger/                    # ← Separate repo, NOT inside MAME
├── README.md
├── install.py                        # Copies files into a MAME source tree
├── uninstall.py                      # Restores originals (backed up by install)
│
├── mame-patches/                     # Drop-in replacement files
│   ├── src/emu/debug/                # Full copy of debug/ with additions:
│   │   ├── [all original files]      #   Unchanged originals
│   │   └── debugcon.cpp              #   Modified: adds output capture hook
│   └── src/osd/modules/debugger/
│       ├── [all original files]      #   Unchanged originals
│       ├── debugremote.cpp           #   NEW: TCP server module
│       └── debugremote.h             #   NEW: TCP server header
│
├── bridge/                           # Python LLM bridge (runs separately)
│   ├── bridge.py                     # Main entry — chat loop
│   ├── mame_client.py                # TCP client for MAME connection
│   ├── claude_tools.py               # Claude API tool definitions
│   ├── event_handler.py              # Async event handling
│   ├── config.py                     # Settings (port, API key, model)
│   └── requirements.txt              # anthropic, etc.
│
└── scripts/
    └── modules.lua.patch             # Patch to register the new module in build
```

**Install flow:**
```bash
python install.py /path/to/mame       # Backs up originals, copies patched files
cd /path/to/mame && make              # Rebuild MAME with LLM debugger support
```

**Uninstall flow:**
```bash
python uninstall.py /path/to/mame     # Restores backed-up originals
```

### Component 1: MAME Debug Server Module

**Location (in standalone repo):** `mame-patches/src/osd/modules/debugger/debugremote.cpp` (new file)

**What it does:**
- Implements `debug_module` interface (like GDB stub already does)
- Starts a TCP listener on a configurable port (default 12345)
- Accepts JSON-line protocol messages
- Routes commands to `debugger_console::execute_command()`
- Captures console output (currently goes to text_buffer) and returns it
- Pushes async events when breakpoints hit, execution stops, etc.

**Key challenges:**
- MAME's debugger is single-threaded — socket I/O must integrate with the debugger's event loop
- The GDB stub (`debuggdbstub.cpp`) already solves this pattern — it does socket I/O in `wait_for_debugger()` which is called when execution stops
- Need to also capture output from commands — currently `debugger_console` prints to a `text_buffer`, need to intercept or redirect

**Existing reference:** The GDB stub is the closest existing pattern. It:
- Opens a TCP socket in `init_debugger()`
- Reads/writes in `wait_for_debugger()` (called on stop)
- Translates GDB protocol ↔ MAME debugger commands

### Component 2: Wire Protocol

**Format:** JSON Lines (one JSON object per line, newline-delimited)

**Request (bridge → MAME):**
```json
{"id": 1, "type": "command", "cmd": "bp 0x1234"}
{"id": 2, "type": "read_memory", "addr": "0x1000", "len": 256, "space": "program"}
{"id": 3, "type": "get_registers"}
{"id": 4, "type": "disassemble", "addr": "0xC000", "count": 20}
{"id": 5, "type": "evaluate", "expr": "pc + 0x10"}
```

**Response (MAME → bridge):**
```json
{"id": 1, "type": "result", "output": "Breakpoint 1 set at 0x1234"}
{"id": 2, "type": "memory", "addr": "0x1000", "data": "4c00c0a9..."}
{"id": 3, "type": "registers", "cpu": "maincpu", "regs": {"PC": "0xC000", "A": "0x42", ...}}
```

**Async events (MAME → bridge, no request ID):**
```json
{"type": "event", "event": "breakpoint_hit", "id": 1, "pc": "0x1234", "cpu": "maincpu"}
{"type": "event", "event": "stopped", "reason": "step", "pc": "0x1235"}
```

### Component 3: LLM Bridge

**Language:** Python (good Claude SDK support, easy prototyping)

**Structure:**
```
llm-debugger/
├── bridge.py          # Main entry — connects to MAME, runs chat loop
├── mame_client.py     # TCP client, sends/receives JSON messages
├── claude_tools.py    # Tool definitions for Claude API
├── event_handler.py   # Handles async events from MAME
└── config.py          # Port, API key, model settings
```

**Claude tool definitions (sketch):**
```python
tools = [
    {
        "name": "execute_debugger_command",
        "description": "Execute a raw MAME debugger command. Use for any command not covered by other tools.",
        "input_schema": {
            "type": "object",
            "properties": {
                "command": {"type": "string", "description": "The debugger command to execute"}
            }
        }
    },
    {
        "name": "read_memory",
        "description": "Read bytes from the emulated system's memory",
        "input_schema": {
            "type": "object",
            "properties": {
                "address": {"type": "string"},
                "length": {"type": "integer"},
                "space": {"type": "string", "enum": ["program", "data", "io"]}
            }
        }
    },
    # ... more tools
]
```

**Chat loop:**
1. User types observation ("the character just jumped")
2. Bridge sends message + conversation history to Claude with tools
3. Claude decides what debugger commands to issue (tool_use)
4. Bridge executes tools against MAME via TCP
5. Tool results fed back to Claude
6. Claude responds with findings/next steps
7. Repeat

---

## Debugger UI Refinements

The UI improvements ride alongside the LLM integration — same drop-in replacement files, same project. No structural rewrites — we enhance what's already there.

### New Views / Panels

**1. LLM Chat/Log Panel**
- Shows the LLM's reasoning, tool calls, and results inline in the debugger
- User can see what commands the LLM issued and why
- Could also allow typing to the LLM directly from inside the debugger (instead of only the external bridge)
- Scrollable, filterable log with timestamps

**2. Annotated Memory View**
- Enhanced memory view where regions can be labeled: `0x0040 = "Player X"`, `0x0076 = "Score (BCD)"`
- Labels can come from the user, the LLM, or imported symbol files
- Color-coded regions by category (player, enemies, system, I/O)
- Annotations persist to a file and reload on next session

**3. Execution Timeline**
- Visual scrollable timeline of events: breakpoint hits, watchpoint triggers, LLM actions
- Each event shows PC, CPU state snapshot, what triggered it
- Click an event to jump the debugger state to that point
- Useful for "what happened 50 frames ago when the score glitched"

**4. Symbol/Label Manager**
- Central panel to name addresses, manage labels, group them by category
- Import/export symbol maps (JSON, MAME .sym format, etc.)
- LLM can auto-populate labels as it discovers what memory locations do
- Search/filter by name or address range

**5. Detachable / Multi-Window Panels**
- Any panel can be popped out into its own window for multi-monitor setups
- Drag panels to rearrange layout
- Save/restore layout configurations
- Keeps the main window uncluttered — only show what you need

### Interactivity Improvements

**Text Selection & Copy/Paste**
- Select individual lines or multi-select (Shift+click, Ctrl+click) in console/disasm/memory views
- Right-click → Copy selection (formatted)
- Copy as: plain text, hex dump, C array, assembler `.db` statements
- Select a range in memory view by click-drag

**Color & Highlighting**
- Changed values flash/highlight (registers that just changed, memory that was written)
- Active breakpoint lines get a distinct background color
- Current PC line is clearly marked
- Watchpoint-triggered addresses pulse briefly
- Configurable color scheme (light/dark themes)

**Console Output Filtering**
- Filter bar at top of console: type to filter output lines in real-time
- Category filters: show/hide LLM commands, breakpoint notifications, user commands, errors
- Log levels: verbose / normal / quiet
- "Pin" important lines so they stay visible at the top

**Layout & Density**
- Adjustable line spacing and padding
- Collapsible sections in register view (group by: general purpose, flags, special)
- Font size control (Ctrl+scroll)
- Monospace font with clear glyph distinction (0/O, 1/l/I)

---

## Implementation Plan (Rough Order)

### Phase 1: Foundation — TCP Server + Basic Bridge
1. Create `debugremote.cpp` as new debug module (modeled on GDB stub)
2. TCP listener, accept one connection, JSON-line protocol
3. Command execution + output capture
4. Python bridge with basic Claude tool definitions
5. Proof of concept: user chats, Claude issues debugger commands

### Phase 2: UI — Readability & Interactivity
1. Text selection + multi-select + copy/paste across all views
2. Color/highlighting for changed values, active breakpoints, current PC
3. Console output filtering (search bar, category toggles)
4. Layout density controls (spacing, font size, collapsible sections)

### Phase 3: UI — New Views
1. LLM chat/log panel (shows LLM reasoning + actions)
2. Symbol/label manager (name addresses, import/export, LLM auto-labels)
3. Annotated memory view (color-coded labeled regions)
4. Execution timeline (scrollable event history)
5. Detachable/multi-window panel support

### Phase 4: Rich LLM Tools
1. Structured memory read/write (not just raw commands)
2. Register dumps in structured format
3. Disassembly output parsing
4. Expression evaluation
5. LLM-driven auto-labeling of discovered memory locations

### Phase 5: Autonomous Mode
1. Claude can set breakpoints, run, observe, iterate
2. Pattern: "find where the score is stored" → LLM searches autonomously
3. Safety limits (max commands per turn, confirmation for writes)

### Phase 6: Live Patching
1. Memory write tools
2. ROM patching (write to program space)
3. Undo/rollback support
4. "Make the player invincible" → LLM figures out how

---

## Conversation Log

> **User:** I want to rework the MAME debugger to be live editable with an LLM.

> **Claude:** Asked about interaction model, UI location, LLM provider, and end goal.

> **User:** All interaction modes (chat + autonomous + patching). The LLM should create commands that the debugger picks up while the user runs the game. User is the eyes telling the LLM what's happening, LLM does commands based on feedback. Claude API. Personal tool first, potentially upstream.

> **Claude:** Asked about transport mechanism and C++ modification comfort level.

> **User:** TCP/WebSocket for real-time communication. Deep C++ modifications — whatever it takes.

> **Claude:** Created this workshop file with full architecture draft.

> **User:** I don't want to affect the MAME project folder. I want something separate that users can just swap out the debugger parts — it wouldn't affect the other folders of MAME.

> **Claude:** Researched the debugger's build boundaries. Found that `src/emu/debug/` (35 files) and `src/osd/modules/debugger/` (95 files) are the only two folders involved. The interfaces don't need to change — changes are purely additive. Restructured the project as a standalone repo with drop-in replacement files + install/uninstall scripts.

> **User:** I'd also like to refine the debugger UI and interactivity. Not changing the underlying structure — adding new views, better text selection and copy/paste, better readability of live outputs. Use whatever frontend it's already built with.

> **Claude:** Asked about specific views and readability pain points.

> **User:** Wants: LLM chat/log panel, annotated memory view, execution timeline, symbol/label manager, and detachable multi-window panels for better screen management. Readability: better color/highlighting, console filtering, less dense layout, and proper text/line selection with multi-select and copy/paste.

> **User:** Refined the LLM architecture: a lesser LLM (Haiku) runs and babysits the debugger process because you can't run a program on the CLI and do LLM stuff at the same time. The best/main LLM (Opus/Sonnet) needs to do the actual commands and thinking because the complexity of assembly-built games demands it. The worker just keeps the debugger up and relays — no thinking.

> **Claude:** Updated architecture to two-LLM model: Worker (Haiku) = process manager/relay, Main (Opus/Sonnet) = the brain that does all reasoning about assembly, game architecture, and reverse engineering strategy. User watches MAME live and reports what they see.

> **User:** When the LLM creates debugging commands it would forget the MAME debugger's specific syntax. Need an instructional doc for the LLM to read.

> **Claude:** Created `docs/llm-debugger-reference.md` — a comprehensive syntax reference built specifically for LLM consumption. Covers every command with exact syntax, correct/wrong examples, a "things that don't exist" table to prevent GDB/LLDB hallucinations, common patterns, expression syntax, memory access operators, cheat search workflow, and critical rules (hex-by-default, comma separators, brace-wrapped actions).

> **User:** We need a skill for the LLM to know the command-line syntax to launch MAME into the remote debugger. Add it to the workshop.

> **Claude:** Added "Needed: MAME Launch Skill" section covering: MAME CLI syntax, single-dash options, `-debug -debugger remote -debugger_port`, ROM paths, common mistakes LLMs make (double-dash, wrong flag names, missing `-debugger remote`). Skill will live as a Claude Code skill for the worker LLM.

---

## Key Insights

- **GDB stub is the template.** MAME already has a TCP-based debug interface (`debuggdbstub.cpp`). The new module follows the same pattern but with a JSON protocol instead of GDB's binary protocol.
- **Output capture is the hard part.** The debugger console prints to a `text_buffer` that views read from. Intercepting this for remote clients needs careful design — possibly a callback or redirect mechanism.
- **Single-threaded model matters.** MAME's debugger only processes input when execution is stopped (in `wait_for_debugger`). For "go" commands, we need a way to also accept "stop" commands asynchronously — the GDB stub handles this with non-blocking socket reads in the execution loop.
- **The user-as-eyes model is powerful.** This isn't just "LLM reads all state" — the user provides semantic context ("that's the score display", "the enemy just spawned") that no amount of memory reading can replace. The LLM then correlates that with execution state.
- **Drop-in replacement is clean.** Only two MAME folders are touched: `src/emu/debug/` and `src/osd/modules/debugger/`. The rest of MAME uses the debugger through `debugger.h`'s public API only. As long as we keep those interfaces identical (which we do — changes are additive), the swap is seamless.
- **Minimal actual modifications.** Most files in the drop-in folders are unchanged copies. The real changes are: (1) one new file `debugremote.cpp`, (2) a small hook addition in `debugcon.cpp` for output capture, (3) a build script patch to register the new module.

---

## Needed: MAME Launch Skill

The worker LLM (Haiku) needs a skill that tells it how to launch MAME with the remote debugger. Without this, the LLM will guess at command-line syntax and get it wrong.

**Skill name:** `mame-debug-launch` (or similar)

**What it needs to cover:**
- MAME command-line syntax: `mame <driver> [options]`
- How to enable debug mode: `-debug`
- How to select the remote debugger: `-debugger remote`
- How to set the port: `-debugger_port <port>` (default: 12345)
- How to set the host: `-debugger_host <addr>` (default: localhost)
- ROM paths and how MAME finds ROMs: `-rompath <path>`
- Common drivers/games for testing
- Full example: `mame pacman -debug -debugger remote -debugger_port 12345`
- How to load with a debug script: `-debugscript <file>`
- How to verify MAME is listening (the console will print "remote debugger: listening on...")
- Error scenarios: ROM not found, missing CHDs, debugger module not built

**Where it lives:** As a Claude Code skill in the project or user skills directory, so the worker LLM can invoke it.

**Key syntax the LLM gets wrong without this:**
- Using `--debug` instead of `-debug` (MAME uses single-dash for long options)
- Using `--port` instead of `-debugger_port`
- Forgetting that `-debugger remote` is required (defaults to platform-native debugger otherwise)
- Not knowing that the game/driver name comes right after `mame` with no flag

---

## Next Steps

- [ ] Decide on remaining open questions (message format, async events, threading)
- [x] Study `debuggdbstub.cpp` in detail as implementation reference
- [x] Prototype the MAME TCP server module
- [ ] Create `mame-debug-launch` skill for the worker LLM
- [ ] Build minimal Python bridge with one or two tools
- [ ] Test with a simple scenario (e.g., "find where lives are stored")

---

## Resources & Links

- `src/osd/modules/debugger/debuggdbstub.cpp` — GDB stub (closest existing pattern)
- `src/osd/modules/debugger/debug_module.h` — debug module interface
- `src/emu/debug/debugcon.h/cpp` — console/command system
- `src/emu/debug/debugcpu.h/cpp` — CPU debug hooks
- `src/emu/debug/debugcmd.h/cpp` — built-in command implementations
- `src/emu/debug/express.h/cpp` — expression parser
- `src/emu/debug/points.h/cpp` — breakpoint/watchpoint classes
- Anthropic Claude API docs: tool_use / function calling
- `docs/llm-debugger-reference.md` — LLM syntax reference (fed to Claude as context when issuing commands)
