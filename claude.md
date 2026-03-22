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

## Key Source References

When modifying `debugremote.cpp`, check these for correct API usage:
- `src/osd/modules/debugger/debuggdbstub.cpp` — Reference TCP debug module (socket patterns, wait_for_debugger flow)
- `src/emu/debug/debugcon.h` — `debugger_console` API (`execute_command`, `get_console_textbuf`, `cmderr_to_string`)
- `src/emu/debug/debugbuf.h` — `debug_disasm_buffer::disassemble(pc, instruction, next_pc, size, info)`
- `src/emu/distate.h` — `device_state_entry` (`to_string()` for values, `symbol()` for names, `visible()`, `divider()`)
- `src/emu/debug/textbuf.h` — `text_buffer_clear`, `text_buffer_lines` iterator
- `src/emu/machine.h` — `disable_side_effects()` RAII guard for suppressing memory read side effects

## Build & Run

- Build: standard MAME build process (`make` or equivalent)
- Run with remote debugger: `mame <game> -debug -debugger remote -debugger_port 12345`
- Host/port options: `-debugger_host <addr> -debugger_port <port>` (same as GDB stub)
