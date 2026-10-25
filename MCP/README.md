# MCP / LLM Debugger Addon for MAME

This folder contains the **self-contained** parts of the LLM debugger addon — an
MCP (Model Context Protocol) server that lets an LLM drive MAME's debugger remotely
over TCP. It is kept separate from the MAME source tree so the addon can be understood,
maintained across upstream rebases, and adopted cleanly.

> The addon is **not fully contained in this folder** — a small, unavoidable set of
> changes lives inside the MAME source tree (a new C++ debugger module + a few edit
> lines). Those are catalogued exactly in [`INTEGRATION.md`](INTEGRATION.md).

## What's here

```
MCP/
  README.md            # this file
  INTEGRATION.md       # the complete manifest of core-MAME touch-points
  bridge/              # the MCP server (Python) — the actual "MCP" code
    mcp_server.py        # FastMCP server exposing debugger tools to the LLM
    mame_client.py       # TCP/JSON client to MAME's remote debugger
    bridge.py            # bridge helpers
    claude_tools.py      # tool definitions
    test_connection.py   # standalone connection test
    requirements.txt     # Python deps
  docs/
    llm-debugger-reference.md   # MAME debugger syntax reference for the LLM
  launch/
    start_debugger.bat   # convenience launcher (Windows)
```

`.mcp.json` lives at the **repo root** (Claude Code discovers it there) and points at
`MCP/bridge/mcp_server.py`.

## Architecture (how it starts — see also docs/)

1. **MAME side**: built with the `remote` debugger module (C++, in the MAME tree). When
   the debugger first stops the machine it opens a TCP server (default `localhost:12345`).
2. **MCP server** (`MCP/bridge/mcp_server.py`): launched by Claude Code per `.mcp.json`.
   It connects to MAME's TCP port **lazily, on the first tool call**, and auto-reconnects.
3. The LLM calls MCP tools → MCP server → TCP/JSON → MAME debugger → results back.

## Run

```
# 1. Launch MAME with the remote debugger
mame <game> -debug -debugger remote -debugger_port 12345

# 2. Claude Code (with this repo's .mcp.json) connects on the first MCP tool call.
```

`launch/start_debugger.bat` is a Windows convenience wrapper (edit the exe/game/rompath
inside it to taste).

## Python deps

```
pip install -r MCP/bridge/requirements.txt
```
