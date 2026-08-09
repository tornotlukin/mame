# CLAUDE.md — MAME Fork (LLM Debugger + Game Mods + Android)

## Repo / Remotes

- This is a personal fork: `origin` = https://github.com/tornotlukin/mame.git,
  `upstream` = https://github.com/mamedev/mame.git.
- **BASE = THE NEWEST MAME TAG WE HAVE MIGRATED TO — currently `mame0289`** (migrated
  2026-08-09; previously mame0288, before that mame0286).
  > ⚠️ **This number goes stale — do NOT treat it as a blocker.** It records the last
  > migration, not a requirement. If upstream has newer tags, that is normal and expected;
  > it does not mean anything is broken. Determine the real base at any time with:
  > `git describe --tags --abbrev=0 <branch>`. To move the fork to a newer release, follow
  > the documented upgrade procedure (see "Upgrading to a new MAME release" below) — never
  > refuse or stall merely because the version here differs from what upstream now offers.
- Installed play copy: **`H:\mame\mame.exe`** = full build + llm-debugger (built from this
  tree); `H:\mame\mameold.exe` = stock backup. Curated exe: `H:\_DEV\modalicious\`.

## BRANCH MAP / COMMIT HYGIENE (directive 2026-07-04, split executed 2026-07-05)

Work streams are SEPARATE branches, **all siblings based directly on the current base tag**
(never stacked on each other — see "Upgrading to a new MAME release"):

| Branch | Contents | Rule |
|--------|----------|------|
| `llm-debugger` | MCP/ addon, debugremote, debugger UI tweaks, project docs | NO game-driver changes |
| `jrpacman-4p` | `jrpacman:` game-board mods (`src/mame/pacman/*`) for pac-man-4ever | pure mod, PR-able |
| `rp6-android` | **Composition**: merge of the above + MAME4droid myosd OSD overlay + Android build glue | what the Android core builds from (see `workshop-mame-android.md`) |
| (planned) `cps2-4p` | CPS2 4-player fighting-game mod (`src/mame/capcom/*`) | own branch off upstream when started |

New commits go to the branch that owns the stream — never mix. Merge streams only in
`rp6-android` (or future composition branches).

Build note: `pac4eva.exe` (the exe the game project launches) is built from THIS tree via
the game repo's `tools/build_mame.sh` — **whichever branch is checked out is what it
plays**, so check out `pac4eva` (or a composition branch) before building pac4eva; plain
`llm-debugger` has no game mods. The game repo's `drivers/` folder remains a
branch-independent fallback.

## Upgrading to a new MAME release (procedure that worked 0.288 → 0.289)

A newer upstream tag is NORMAL. Migrating is routine — follow this order; it is the order
that avoids the traps we actually hit.

1. **Fetch + pick the tag:** `git fetch upstream --tags`; newest = `git tag | sort -V | tail -3`.
2. **Triage the risk BEFORE touching anything** — for each file our branches modify, ask how
   much upstream churned it: `git diff --shortstat <oldtag> <newtag> -- <file>`. This turns
   an unknown into a short list of expected conflicts (0.289: only 2 files conflicted).
3. **Backup every branch first:** `git tag -f backup-<oldtag>/<branch> <branch>`. Non-negotiable.
4. **Rebase each SOURCE branch** onto the new tag:
   `git rebase --onto <newtag> <oldtag> <branch>` — do `llm-debugger`, `pac4eva`, `vs-4p-mod`.
   Branches are siblings, so order does not matter and conflicts stay isolated per branch.
   > **TRAP (cost us a false start):** if a branch was ever *stacked* on another (vs-4p-mod
   > used to sit on llm-debugger), rebasing it onto the tag replays the parent's commits too
   > and re-hits the same conflicts. Replay only its own commits:
   > `git rebase --onto <newtag> <old-parent-tip> <branch>` — which also converts it into a
   > proper sibling. **Keep every work branch a sibling of the tag; never stack them.**
   > This is safe because the debugger (`MCP/`, `src/osd/**`) and the game mods
   > (`src/mame/**`) touch DISJOINT files — verified; compositions merge conflict-free.
5. **REBUILD the composition branches, don't rebase them** (they are merge-shaped):
   recreate `modalicious` / `rp6-android` from the rebased sources + their own unique commits.
6. **Verify before pushing:** build Modalicious → `modalicious.exe -verifyroms "*"` must be
   7/7; build the Android core + APK. Only then force-push (backups still exist).
7. **Update this file's base-tag line** (top of Repo/Remotes) to the new tag.

Conflict style seen in practice: upstream refactors (e.g. `metrics()` → a local `metrics`
ref) or deletes debug blocks; keep OUR functional change, adopt THEIR form/removal.

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
