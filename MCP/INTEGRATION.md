# Integration Manifest — LLM Debugger Addon ↔ MAME core

This is the **complete** list of how the addon touches the MAME source tree. Everything
else lives self-contained under [`MCP/`](.). With this manifest you can apply the addon to
a fresh MAME checkout, or remove it cleanly, by hand.

Three categories: **(A) new files**, **(B) required core edits**, **(C) optional UI feature**.
All of (A)+(B) are required for the MCP/remote debugger to build and run. (C) is bundled as
part of this addon but is independent of the MCP server and can be taken or left.

---

## A. New files added to the MAME tree

Kept in MAME's idiomatic debugger-module location (next to `debuggdbstub.cpp` /
`debugimgui.cpp`) so upstream can adopt them where it expects debugger modules:

| File | ~Lines | Purpose |
|------|--------|---------|
| `src/osd/modules/debugger/debugremote.cpp` | 816 | The `remote` debugger module: TCP server + JSON-line protocol. Self-declares `MODULE_DEFINITION(DEBUG_REMOTE, osd::debug_remote)`. |
| `src/osd/modules/debugger/debugremote_tcp.h` | 585 | TCP/protocol header used by the module. |

> These compile **into `mame.exe`**, so they must live in the source tree and be listed in
> the build (see B1). That's why they aren't physically inside `MCP/`.

## B. Required core edits (the entire coupling — 3 lines)

### B1. `scripts/src/osd/modules.lua` — add module to the build (+2 lines)
Inside `osdmodulesbuild()`, after the `debuggdbstub.cpp` entry:
```lua
		MAME_DIR .. "src/osd/modules/debugger/debugremote.cpp",
		MAME_DIR .. "src/osd/modules/debugger/debugremote_tcp.h",
```

### B2. `src/osd/modules/lib/osdobj_common.cpp` — register the module (+1 line)
In `osd_common_t::register_options()`, after `REGISTER_MODULE(m_mod_man, DEBUG_GDBSTUB);`:
```cpp
	REGISTER_MODULE(m_mod_man, DEBUG_REMOTE);
```
No header change is needed — `REGISTER_MODULE` carries its own extern, and the symbol is
defined by `MODULE_DEFINITION` in `debugremote.cpp`.

That's it for the MCP/remote server: **2 new files + 3 lines.** Enable at runtime with
`-debugger remote -debugger_port <port>`.

## C. Optional UI feature — Win32 debugger text selection / copy (bundled)

Independent of the MCP server (a quality-of-life enhancement to the native Windows
debugger UI: text selection, "copy all", Ctrl+C / Ctrl+A). These are **modifications to
existing files**, so they cannot be relocated — listed here for completeness:

| File | Change |
|------|--------|
| `src/osd/modules/debugger/win/debugviewinfo.cpp` | +~362 lines — selection model, copy, key handling |
| `src/osd/modules/debugger/win/debugviewinfo.h` | +~22 lines — selection state members |
| `src/osd/modules/debugger/debugwin.cpp` | +~64 / -~24 — hybrid Win32 UI + console-history wiring |

To adopt the MCP server **without** this UI feature, simply don't apply the C edits — the
remote module (A + B) is fully functional on its own.

---

## D. Self-contained addon files (no core coupling)

- `MCP/bridge/**` — the Python MCP server + client + tests (`requirements.txt` included).
- `MCP/docs/llm-debugger-reference.md` — debugger syntax reference for the LLM.
- `MCP/launch/start_debugger.bat` — Windows launcher convenience.
- `.mcp.json` — at repo root (Claude Code discovers it there); points to
  `MCP/bridge/mcp_server.py`.

---

## Removing the addon cleanly

1. Delete the two files in **A**.
2. Revert the 3 lines in **B** (B1 ×2, B2 ×1).
3. (Optional) Revert the **C** files if you applied them.
4. Delete `MCP/` and `.mcp.json`.

After steps 1–2 the tree is byte-for-byte stock MAME for the OSD/build, leaving no trace of
the MCP/remote debugger.
