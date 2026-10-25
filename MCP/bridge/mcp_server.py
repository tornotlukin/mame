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
    # Ensure connected (auto-reconnects if connection was lost)
    _client._ensure_connected()
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


# =========================================================================
# Section 1: Cheat Search
# =========================================================================

@mcp.tool()
def cheat_init(format: str = "ub", address: int = -1, length: int = -1) -> str:
    """Initialize cheat search for finding game values (lives, score, etc.).

    Format string: <sign><width>[swap]
      sign: u (unsigned) or s (signed)
      width: b (8-bit), w (16-bit), d (32-bit), q (64-bit)
      swap: s (byte-swap, optional)

    Examples: 'ub' = unsigned byte, 'sw' = signed word, 'uds' = unsigned dword swapped.
    If address/length omitted, searches all writable RAM.
    """
    if address >= 0 and length > 0:
        return get_client().command(f"cheatinit {format},{address:x},{length:x}")
    else:
        return get_client().command(f"cheatinit {format}")


@mcp.tool()
def cheat_next(condition: str, value: int = -1) -> str:
    """Filter cheat candidates by comparing to PREVIOUS values.

    Conditions:
      all — update values, no filtering
      equal / eq — equal to previous (or to value if given)
      notequal / ne — not equal
      decrease / - — decreased (or decreased by value)
      increase / + — increased (or increased by value)
      decreaseorequal / deeq — decreased or same
      increaseorequal / ineq — increased or same
      smallerof / < — less than value (required)
      greaterof / > — greater than value (required)
      changedby / ~ — changed by exactly value (required)
    """
    if value >= 0:
        return get_client().command(f"cheatnext {condition},{value:x}")
    else:
        return get_client().command(f"cheatnext {condition}")


@mcp.tool()
def cheat_nextf(condition: str, value: int = -1) -> str:
    """Filter cheat candidates by comparing to INITIAL values (from cheat_init).

    Same conditions as cheat_next but compares against the original snapshot.
    """
    if value >= 0:
        return get_client().command(f"cheatnextf {condition},{value:x}")
    else:
        return get_client().command(f"cheatnextf {condition}")


@mcp.tool()
def cheat_list() -> str:
    """Show current cheat search candidates. Shows address, start value, and current value."""
    return get_client().command("cheatlist")


@mcp.tool()
def cheat_undo() -> str:
    """Undo the last cheat_next or cheat_nextf filter."""
    return get_client().command("cheatundo")


# =========================================================================
# Section 2: Breakpoint/Watchpoint Management
# =========================================================================

@mcp.tool()
def set_breakpoint(address: int, condition: str = "", action: str = "") -> str:
    """Set an execution breakpoint at address.

    Optional condition: expression evaluated each hit, breaks only if true.
    Optional action: debugger command executed when breakpoint fires.
    If action contains commas/semicolons, it will be wrapped in braces automatically.
    """
    cmd = f"bp {address:x}"
    if condition:
        cmd += f",{condition}"
        if action:
            if ',' in action or ';' in action:
                cmd += f",{{ {action} }}"
            else:
                cmd += f",{action}"
    return get_client().command(cmd)


@mcp.tool()
def clear_breakpoint(bp_id: int = -1) -> str:
    """Clear a breakpoint by ID, or clear all breakpoints if no ID given."""
    if bp_id >= 0:
        return get_client().command(f"bpclear {bp_id}")
    else:
        return get_client().command("bpclear")


@mcp.tool()
def enable_breakpoint(bp_id: int = -1) -> str:
    """Enable a breakpoint by ID, or enable all if no ID given."""
    if bp_id >= 0:
        return get_client().command(f"bpenable {bp_id}")
    else:
        return get_client().command("bpenable")


@mcp.tool()
def disable_breakpoint(bp_id: int = -1) -> str:
    """Disable a breakpoint by ID, or disable all if no ID given."""
    if bp_id >= 0:
        return get_client().command(f"bpdisable {bp_id}")
    else:
        return get_client().command("bpdisable")


@mcp.tool()
def list_breakpoints() -> str:
    """List all breakpoints with their IDs, addresses, conditions, and actions."""
    return get_client().command("bplist")


@mcp.tool()
def set_watchpoint(address: int, length: int, type: str, condition: str = "", action: str = "") -> str:
    """Set a memory watchpoint.

    type: 'r' (read), 'w' (write), or 'rw' (both).
    length: number of bytes to watch (e.g. 1 for byte, 2 for word).
    Optional condition: use wpaddr (access address) and wpdata (written value).
    Optional action: command to run when triggered.
    """
    cmd = f"wp {address:x},{length:x},{type}"
    if condition:
        cmd += f",{condition}"
        if action:
            if ',' in action or ';' in action:
                cmd += f",{{ {action} }}"
            else:
                cmd += f",{action}"
    return get_client().command(cmd)


@mcp.tool()
def clear_watchpoint(wp_id: int = -1) -> str:
    """Clear a watchpoint by ID, or clear all watchpoints if no ID given."""
    if wp_id >= 0:
        return get_client().command(f"wpclear {wp_id}")
    else:
        return get_client().command("wpclear")


@mcp.tool()
def enable_watchpoint(wp_id: int = -1) -> str:
    """Enable a watchpoint by ID, or enable all if no ID given."""
    if wp_id >= 0:
        return get_client().command(f"wpenable {wp_id}")
    else:
        return get_client().command("wpenable")


@mcp.tool()
def disable_watchpoint(wp_id: int = -1) -> str:
    """Disable a watchpoint by ID, or disable all if no ID given."""
    if wp_id >= 0:
        return get_client().command(f"wpdisable {wp_id}")
    else:
        return get_client().command("wpdisable")


@mcp.tool()
def list_watchpoints() -> str:
    """List all watchpoints with their IDs, addresses, types, conditions, and actions."""
    return get_client().command("wplist")


# =========================================================================
# Section 3: Execution Navigation
# =========================================================================

@mcp.tool()
def step_out() -> str:
    """Step out of the current subroutine. Returns new PC."""
    client = get_client()
    client.command("out")
    state = client.get_state()
    return f"PC: {state.get('pc', '?')}"


@mcp.tool()
def run_to_vblank() -> str:
    """Resume execution until the next vertical blanking interval."""
    return get_client().command("gv")


@mcp.tool()
def set_register(name: str, value: str) -> str:
    """Set a CPU register or temp variable to a value.

    name: register name (pc, a0, d0, sp, etc.) or temp variable (temp0-temp9).
    value: hex value to set (numbers are hex by default).
    """
    return get_client().command(f"do {name}={value}")


# =========================================================================
# Section 4: Save/Load State
# =========================================================================

@mcp.tool()
def save_state(filename: str) -> str:
    """Save emulation state to a file. Extension .sta is added automatically."""
    return get_client().command(f"ss {filename}")


@mcp.tool()
def load_state(filename: str) -> str:
    """Load emulation state from a file. Extension .sta is added automatically."""
    return get_client().command(f"sl {filename}")


@mcp.tool()
def rewind() -> str:
    """Load the most recent save state. Provides undo/reverse execution."""
    return get_client().command("rw")


# =========================================================================
# Section 5: System Info & History
# =========================================================================

@mcp.tool()
def get_symbols(cpu: str = "") -> str:
    """List all available symbols/registers for a CPU.

    If no CPU specified, shows symbols for the visible CPU + global symbols.
    """
    if cpu:
        return get_client().command(f"symlist {cpu}")
    else:
        return get_client().command("symlist")


@mcp.tool()
def get_history(cpu: str = "", length: int = 0) -> str:
    """Show recently visited PC addresses with disassembly.

    Shows the most recent instructions that were executed.
    """
    cmd = "history"
    if cpu:
        cmd += f" {cpu}"
        if length > 0:
            cmd += f",{length:x}"
    return get_client().command(cmd)


@mcp.tool()
def pc_at_memory(address: int) -> str:
    """Show which PC address last wrote to the specified memory address.

    Useful for finding what code modifies a particular memory location.
    Requires trackmem to be enabled first.
    """
    return get_client().command(f"pcatmem {address:x}")


@mcp.tool()
def get_time() -> str:
    """Get the total elapsed emulated time."""
    return get_client().command("time")


@mcp.tool()
def screenshot(filename: str = "") -> str:
    """Take a screenshot of the emulated display.

    Saved to the configured snapshot directory.
    """
    if filename:
        return get_client().command(f"snap {filename}")
    else:
        return get_client().command("snap")


if __name__ == "__main__":
    mcp.run()
