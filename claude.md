# CLAUDE.md — MAME LLM Debugger Project

## Key Documentation

- **MAME Debugger Syntax Reference (for LLM):** `docs/llm-debugger-reference.md`
  - Consult this whenever writing or generating MAME debugger commands
  - Contains exact syntax, correct/wrong examples, and common patterns
  - Numbers are HEX by default in MAME debugger — do not use `0x` prefix unnecessarily
  - Do NOT use GDB/LLDB/x86 debugger syntax — MAME has its own command set

- **MAME Debugger Documentation (official):** `docs/source/debugger/` (RST files)
  - Full reference for all debugger commands, expression syntax, device specs

- **Workshop File:** `workshop-debugger-llm.md`
  - Tracks all architecture decisions, open questions, and progress for the LLM debugger project

## Project Structure (LLM Debugger additions)

- `src/osd/modules/debugger/debugremote.cpp` — TCP JSON debug module (new)
- `src/osd/modules/lib/osdobj_common.cpp` — Module registration (modified)
- `scripts/src/osd/modules.lua` — Build config (modified)

## Build & Run

- Build: standard MAME build process (`make` or equivalent)
- Run with remote debugger: `mame <game> -debug -debugger remote -debugger_port 12345`
