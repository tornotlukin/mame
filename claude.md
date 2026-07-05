# CLAUDE.md — MAME LLM Debugger Project

## COMMIT / BRANCH HYGIENE (user directive 2026-07-04)

This fork hosts TWO unrelated work streams — keep their commits SEPARATE from now on:

1. **The LLM-debugger addon** (MCP/, debugremote, debugger UI tweaks) → stays on the
   `llm-debugger` branch.
2. **Game-board mods** (driver changes for game projects, e.g. `src/mame/pacman/*` for
   pac-man-4ever) → each game mod gets its **own branch** so it can be shared/PR'd as a
   pure mod. A **CPS2 4-player fighting-game mod is planned** — when it starts, create
   e.g. `cps2-4p` off upstream and commit its `src/mame/capcom/*` changes THERE, never on
   `llm-debugger`.

Historical note: RESOLVED 2026-07-05 — the branches were surgically split. All
`jrpacman:` commits now live on **`jrpacman-4p`** (off `mame0288`); `llm-debugger` was
rewritten to contain only debugger/docs commits (force-pushed). The game repo's
`drivers/` folder also carries the full files + `pac4eva-mame.patch` as a
branch-independent fallback. A planned **`rp6-android`** branch will merge
`llm-debugger` + `jrpacman-4p` + the MAME4droid myosd OSD overlay as the composition
branch the Android core builds from (see `workshop-mame-android.md`).

Build note: `pac4eva.exe` (the exe the game project launches) is built from THIS tree via
the game repo's `tools/build_mame.sh` — whichever branch is checked out is what it plays.

## Key Documentation

- **MAME Debugger Syntax Reference (for LLM):** `MCP/docs/llm-debugger-reference.md`
  - Consult this whenever writing or generating MAME debugger commands
  - Contains exact syntax, correct/wrong examples, and common patterns
  - Numbers are HEX by default in MAME debugger — do not use `0x` prefix unnecessarily
  - Do NOT use GDB/LLDB/x86 debugger syntax — MAME has its own command set

- **MAME Debugger Documentation (official):** `docs/source/debugger/` (RST files)
  - Full reference for all debugger commands, expression syntax, device specs

- **Workshop File:** `workshop-debugger-llm.md`
  - Tracks all architecture decisions, open questions, and progress for the LLM debugger project

## Project Structure (LLM Debugger additions)

Self-contained addon code lives under `MCP/` (see `MCP/README.md` and
`MCP/INTEGRATION.md` for the full manifest of how it touches core MAME):

- `MCP/bridge/` — Python MCP server + TCP client + tests (the actual MCP code)
- `MCP/docs/llm-debugger-reference.md` — debugger syntax reference for the LLM
- `MCP/launch/start_debugger.bat` — Windows launcher
- `.mcp.json` — at repo root (Claude Code discovers it); points to `MCP/bridge/mcp_server.py`

Core-MAME touch-points (kept in the source tree because they compile into `mame.exe`):

- `src/osd/modules/debugger/debugremote.cpp` / `debugremote_tcp.h` — TCP JSON debug module (new)
- `src/osd/modules/lib/osdobj_common.cpp` — Module registration (modified, +1 line)
- `scripts/src/osd/modules.lua` — Build config (modified, +2 lines)
- `src/osd/modules/debugger/debugwin.cpp`, `win/debugviewinfo.{cpp,h}` — Win32 UI text-selection feature (modified)

## Key Source References

When modifying `debugremote.cpp`, check these for correct API usage:
- `src/osd/modules/debugger/debuggdbstub.cpp` — Reference TCP debug module (socket patterns, wait_for_debugger flow)
- `src/emu/debug/debugcon.h` — `debugger_console` API (`execute_command`, `get_console_textbuf`, `cmderr_to_string`)
- `src/emu/debug/debugbuf.h` — `debug_disasm_buffer::disassemble(pc, instruction, next_pc, size, info)`
- `src/emu/distate.h` — `device_state_entry` (`to_string()` for values, `symbol()` for names, `visible()`, `divider()`)
- `src/emu/debug/textbuf.h` — `text_buffer_clear`, `text_buffer_lines` iterator
- `src/emu/machine.h` — `disable_side_effects()` RAII guard for suppressing memory read side effects

## Build Environment

- **MSYS2 location:** `H:\_DEV\msys64` (pre-packaged MAME build tools from http://mamedev.org/tools/)
- **GCC version:** 11.2.0 (included in the pre-packaged tools)
- **User also has VS 2022** available for builds

## Building MAME (Windows — CRITICAL notes)

### Shell and Environment Setup

Do NOT use `--login` flag with bash — it changes the working directory to `~` and loses the MAME source path. Use plain `-c` instead:

```bash
"H:/_DEV/msys64/usr/bin/bash.exe" -c "<commands>"
```

The following environment variables MUST be set explicitly — they are NOT inherited from the Windows environment when running bash from Claude Code:

```bash
export PATH=/h/_DEV/msys64/mingw64/bin:/h/_DEV/msys64/usr/bin:$PATH
export OS=Windows_NT        # CRITICAL: makefile checks this first, fails with uname detection error if missing
export MSYSTEM=MINGW64      # CRITICAL: tells makefile which toolchain to use
export MINGW_PREFIX=/mingw64
export TEMP=/tmp            # CRITICAL: GCC writes temp files; without this it tries C:\WINDOWS\ which is permission denied
export TMP=/tmp             # Same as TEMP
```

### Full Build Command Template

```bash
"H:/_DEV/msys64/usr/bin/bash.exe" -c "export PATH=/h/_DEV/msys64/mingw64/bin:/h/_DEV/msys64/usr/bin:\$PATH && export OS=Windows_NT && export MSYSTEM=MINGW64 && export MINGW_PREFIX=/mingw64 && export TEMP=/tmp && export TMP=/tmp && make SUBTARGET=<name> SOURCES=<path> REGENIE=1 -j5 2>&1"
```

### Subset Build (faster — only builds specific drivers)

```bash
make SUBTARGET=cps1test SOURCES=src/mame/capcom/cps1.cpp REGENIE=1 -j5
```

- Produces `cps1test.exe` instead of full `mame.exe`
- `REGENIE=1` is required when adding new source files or changing build config
- `-j5` for parallel compilation (CPU cores + 1)

### VS2022 Project Generation

```bash
make vs2022 SUBTARGET=cps1test SOURCES=src/mame/capcom/cps1.cpp
```

Projects go to `build/projects/windows/mame/vs2022/`. Same env vars required.

### Common Build Errors and Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `Unable to detect OS from uname -a` | `OS` env var not set | `export OS=Windows_NT` |
| `Cannot create temporary file in C:\WINDOWS\` | `TEMP`/`TMP` pointing to protected dir | `export TEMP=/tmp && export TMP=/tmp` |
| `No targets specified and no makefile found` | Used `--login` flag, shell is in `~` | Remove `--login`, use `-c` so CWD stays in MAME source |
| `MSYSTEM` not set warnings | env var missing | `export MSYSTEM=MINGW64 && export MINGW_PREFIX=/mingw64` |

## Running the Built Executable

```bash
./cps1test qadjr -debug -debugger remote -debugger_port 12345
```

- Game driver comes right after the executable name, no flag
- `-debug` enables the debugger (single dash, not double)
- `-debugger remote` selects the remote TCP module
- `-debugger_port 12345` sets the listening port
- `-debugger_host localhost` sets the listening address (default: localhost)
