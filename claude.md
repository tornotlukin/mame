# CLAUDE.md — MAME Fork (LLM Debugger + Game Mods + Android)

## Repo / Remotes

- This is a personal fork: `origin` = https://github.com/tornotlukin/mame.git,
  `upstream` = https://github.com/mamedev/mame.git. Base: **mame0288** tag.
- Installed play copy: **`H:\mame\mame.exe`** = full 0.288 build + llm-debugger
  (built from this tree); `H:\mame\mameold.exe` = stock 0.288 backup.

## BRANCH MAP / COMMIT HYGIENE (directive 2026-07-04, split executed 2026-07-05)

Work streams are SEPARATE branches, all based on `mame0288`:

| Branch | Contents | Rule |
|--------|----------|------|
| `llm-debugger` | MCP/ addon, debugremote, debugger UI tweaks, project docs | NO game-driver changes |
| `pac4eva` | Pac-Man 4 EVA — **one added file**, `src/mame/pacman/pac4eva.cpp` (+ its `mame.lst` stanza) | additions only; touches NO stock source |
| `vs-4p-mod` | CPS2 4-player fighting-game mods (`src/mame/capcom/*`) — xmvsf/mshvsf/mvsc 2v2 + mvscduo | in-place driver edits |
| `modalicious` | **Composition**: `pac4eva` + `vs-4p-mod` + the Modalicious subtarget (`src/mame/modalicious.lst`, `scripts/target/mame/modalicious.lua`) | builds `mamemodalicious.exe` — the curated mods-only exe |
| `rp6-android` | **Composition**: `llm-debugger` + the game mods + MAME4droid myosd OSD overlay + Android build glue | what the Android core builds from (see `workshop-mame-android.md`) |

New commits go to the branch that owns the stream — never mix. Merge streams only in the
composition branches (`modalicious`, `rp6-android`).

> **RENAMED 2026-07-25: `jrpacman-4p` → `pac4eva`** (old remote branch deleted). The name no
> longer described the contents: the mod used to edit `jrpacman.cpp`, `pacman.h` and
> `pacman_v.cpp` in place, but it is now a **single additive file**, `pac4eva.cpp`, and those
> three stock files are byte-identical to upstream (`git diff e68ee468568^ -- src/mame/pacman/`
> lists only `pac4eva.cpp`). Also corrected here: the CPS2 branch is `vs-4p-mod` and has been
> active for a while (this table still called it "planned `cps2-4p`"), and `modalicious` was
> missing entirely.

Build note: `pac4eva.exe` (the fast dev exe the game project launches) is built from THIS tree
via the game repo's `tools/build_mame.sh` — **whichever branch is checked out is what it
plays**, so check out `pac4eva` (or a composition branch) before building; plain `llm-debugger`
has no game mods. The curated exe is `mamemodalicious.exe`, built from `modalicious`.

Because the Pac-Man 4 EVA driver is now purely additive, installing it anywhere is a copy plus
one `mame.lst` stanza — no merge, no conflict resolution:
```
cp <game-repo>/drivers/pac4eva.cpp src/mame/pacman/pac4eva.cpp
# mame.lst:  @source:pacman/pac4eva.cpp
#            pac4eva
```
(The game repo's old `pac4eva-mame.patch` is gone — it described the retired in-place modset.)

## Android Project (RP6 / MAME4droid)

Goal: MAME on the Retroid Pocket 6 with full controller access, built from OUR fork
(this tree) — full plan/status in **`workshop-mame-android.md`**.

- MAME4droid clones (inspection/app shell): `H:\_DEV\mame4droid\MAME4droid-Current`
  (the one that matters) and `MAME4droid_Native` (legacy 0.139, ignore).
- Native architecture: APK's Gradle builds only a thin JNI shim; it `dlopen()`s
  **`libMAME4droid.so`** = MAME core + `src/osd/myosd/` overlay, built out-of-band from
  a MAME tree (ours, on `rp6-android`) with DIY lua/makefile glue + NDK.
- Android SDK: `H:\_DEV\android\SDK` (platform 36, build-tools, adb). NDK pinned by the
  app: **28.2.13676358** (r28c) — installed side-by-side with NDK 30 (ignored).
- Key input facts: funnel is Java `GameController.emulatorInputValues[]` +
  `MYOSD_NUM_JOY=4` cap; native `input.cpp` already registers 6-axis MAME devices;
  L3/R3 commented out at `input.cpp:172`.

## Related Tooling (outside this repo)

- **`/mame-rom` skill** (user-global, `~/.claude/skills/mame-rom/`): parses any game's
  ROM layout from this tree's source + drives the live debugger; per-game findings
  persist in `docs/roms/<game>.md` here.
- Locations config for that skill: `.claude/mame-rom-read-loc.md` (project) or
  `~/.claude/mame-rom-read-loc.md` (global).

## Key Documentation

- **MAME Debugger Syntax Reference (for LLM):** `MCP/docs/llm-debugger-reference.md`
  - Consult this whenever writing or generating MAME debugger commands
  - Contains exact syntax, correct/wrong examples, and common patterns
  - Numbers are HEX by default in MAME debugger — do not use `0x` prefix unnecessarily
  - Do NOT use GDB/LLDB/x86 debugger syntax — MAME has its own command set

- **MAME Debugger Documentation (official):** `docs/source/debugger/` (RST files)
  - Full reference for all debugger commands, expression syntax, device specs

- **Workshop Files** (persistent design/progress trackers):
  - `workshop-debugger-llm.md` — LLM debugger architecture decisions & progress
  - `workshop-rom-reading.md` — design history of the `/mame-rom` skill (built, global)
  - `workshop-mame-android.md` — RP6/Android port: decisions, Phase-0 findings, plan
- **Full MAME 0.289 manual (converted PDF):** `docs/MAME289DOCS.md` — ~250K tokens,
  NEVER read whole; grep then read ranges (index/recipes:
  `~/.claude/skills/mame-rom/reference/mame-docs-index.md`)

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
