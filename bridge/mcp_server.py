#!/usr/bin/env python3
"""
MAME Debugger MCP Server
=========================
Exposes MAME remote debugger tools as an MCP server.
Claude Code or Claude Desktop connects to this server and
gets direct access to debugger tools through your Max subscription.

Usage:
  1. Start MAME:  cps1test qadjr -debug -debugger remote -debugger_port 12345
  2. Run server:  python bridge/mcp_server.py
  3. Claude Code connects automatically (configure in settings)

Environment:
  MAME_DEBUG_HOST  — MAME host (default: localhost)
  MAME_DEBUG_PORT  — MAME port (default: 12345)
"""

import os
import sys
import json

# Add bridge directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from mcp.server.fastmcp import FastMCP
from mame_client import MameClient

# Create MCP server
mcp = FastMCP("mame-debugger")

# Global client — connects on first tool use
_client: MameClient | None = None


def get_client() -> MameClient:
    global _client
    if _client is None:
        host = os.environ.get("MAME_DEBUG_HOST", "localhost")
        port = int(os.environ.get("MAME_DEBUG_PORT", "12345"))
        _client = MameClient(host, port)
        _client.connect()
    return _client


@mcp.tool()
def execute_debugger_command(command: str) -> str:
    """Execute a raw MAME debugger command.

    MAME debugger syntax — NOT GDB/LLDB syntax:
    - bp 1234 (breakpoint), wp 1234,1,w (watchpoint), g (go/resume)
    - s (step), o (step over), out (step out)
    - print <expr>, symlist, history, bplist, wplist
    - find <addr>,<len>,<data> (search memory)
    - cheatinit/cheatnext/cheatlist (cheat search)
    - Numbers are HEX by default. Use # for decimal.
    """
    return get_client().command(command)


@mcp.tool()
def read_memory(address: int, length: int, space: str = "program") -> str:
    """Read bytes from emulated memory. Returns formatted hex dump.

    Side effects are suppressed (safe for inspection).
    Space: program, data, io, or opcodes.
    """
    client = get_client()
    resp = client.read_memory(address, length, space)
    hex_data = resp.get("data", "")
    ascii_data = resp.get("ascii", "")

    lines = []
    for i in range(0, len(hex_data), 32):
        chunk_hex = hex_data[i:i + 32]
        chunk_ascii = ascii_data[i // 2:(i + 32) // 2]
        offset = address + i // 2
        hex_pairs = " ".join(chunk_hex[j:j + 2] for j in range(0, len(chunk_hex), 2))
        lines.append(f"{offset:08X}: {hex_pairs:<48s} {chunk_ascii}")
    return "\n".join(lines) if lines else "(no data)"


@mcp.tool()
def write_memory(address: int, data: str, space: str = "program") -> str:
    """Write hex bytes to emulated memory.

    Data is a hex string like 'ff00ab12'. Side effects ARE enabled.
    Will not work on ROM areas. Be careful — can crash the emulated game.
    """
    return get_client().write_memory(address, data, space)


@mcp.tool()
def get_registers() -> str:
    """Get all CPU registers and their current values for the visible CPU."""
    regs = get_client().get_registers()
    lines = [f"  {name:8s} = {value}" for name, value in regs.items()]
    return "\n".join(lines) if lines else "(no registers)"


@mcp.tool()
def disassemble(address: int, count: int = 10) -> str:
    """Disassemble instructions at address.

    Returns formatted listing: address, opcodes, and mnemonics.
    Max 200 instructions per call.
    """
    return get_client().disassemble(address, count)


@mcp.tool()
def evaluate_expression(expression: str) -> str:
    """Evaluate a MAME debugger expression.

    Supports: register names, arithmetic, memory access (b@addr, w@addr, d@addr),
    functions (min, max, if, abs, bit, s8, s16, s32).
    Numbers are hex by default. Use # for decimal.
    """
    return get_client().evaluate(expression)


@mcp.tool()
def get_execution_state() -> str:
    """Get current execution state: stopped/running, PC, CPU type, machine name."""
    state = get_client().get_state()
    return "\n".join([
        f"Status:  {'STOPPED' if state.get('stopped') else 'RUNNING'}",
        f"PC:      {state.get('pc', '?')}",
        f"CPU:     {state.get('cpu', '?')}",
        f"Machine: {state.get('machine', '?')}",
    ])


@mcp.tool()
def halt_execution() -> str:
    """Break into the debugger, halting the emulated CPU.

    Use when you need to inspect state while the game is running.
    """
    return get_client().halt()


@mcp.tool()
def resume_execution(address: str = "") -> str:
    """Resume game execution.

    Optionally provide an address to set a temporary breakpoint
    (execution will stop when that address is reached).
    """
    cmd = f"g {address}" if address else "g"
    return get_client().command(cmd)


@mcp.tool()
def step_instruction(count: int = 1) -> str:
    """Single-step one or more instructions. Returns new PC and registers."""
    client = get_client()
    client.command(f"s {count}" if count > 1 else "s")
    # Return state after stepping
    state = client.get_state()
    regs = client.get_registers()
    pc = state.get("pc", "?")
    reg_summary = ", ".join(f"{k}={v}" for k, v in list(regs.items())[:8])
    return f"PC: {pc}\n{reg_summary}"


@mcp.tool()
def step_over(count: int = 1) -> str:
    """Step over subroutine calls. Returns new PC."""
    client = get_client()
    client.command(f"o {count}" if count > 1 else "o")
    state = client.get_state()
    return f"PC: {state.get('pc', '?')}"


@mcp.tool()
def search_memory(address: int, length: int, data: str) -> str:
    """Search memory for a byte pattern.

    Data can be: hex bytes (b.03), strings ("TEXT"), or wildcards (?).
    Uses MAME 'find' command syntax.
    """
    return get_client().command(f"find {address:x},{length:x},{data}")


if __name__ == "__main__":
    mcp.run()
