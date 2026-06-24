"""
Claude Tool Definitions for MAME Debugger
==========================================
Defines the tools that Claude can use to interact with the MAME debugger,
and handles executing tool calls against the MameClient.
"""

from mame_client import MameClient

TOOLS = [
    {
        "name": "execute_debugger_command",
        "description": "Execute a raw MAME debugger command and return the output. Use docs/llm-debugger-reference.md syntax ONLY. Common commands: bp (breakpoint), wp (watchpoint), g (go/resume), s (step), o (step over), out (step out), print, symlist, history. Numbers are HEX by default.",
        "input_schema": {
            "type": "object",
            "properties": {
                "command": {
                    "type": "string",
                    "description": "The exact MAME debugger command to execute. Examples: 'bp 1234', 'g', 's', 'print pc', 'symlist'"
                }
            },
            "required": ["command"]
        }
    },
    {
        "name": "read_memory",
        "description": "Read bytes from the emulated system's memory. Returns hex data and ASCII representation. Side effects are suppressed (safe for inspection).",
        "input_schema": {
            "type": "object",
            "properties": {
                "address": {
                    "type": "integer",
                    "description": "Start address to read from"
                },
                "length": {
                    "type": "integer",
                    "description": "Number of bytes to read (max 65536)"
                },
                "space": {
                    "type": "string",
                    "enum": ["program", "data", "io", "opcodes"],
                    "description": "Address space to read from (default: program)"
                }
            },
            "required": ["address", "length"]
        }
    },
    {
        "name": "write_memory",
        "description": "Write bytes to the emulated system's memory. Data is a hex string (e.g. 'ff00ab'). Side effects ARE enabled. Will not work on ROM areas.",
        "input_schema": {
            "type": "object",
            "properties": {
                "address": {
                    "type": "integer",
                    "description": "Address to write to"
                },
                "data": {
                    "type": "string",
                    "description": "Hex string of bytes to write (e.g. 'ff00ab12')"
                },
                "space": {
                    "type": "string",
                    "enum": ["program", "data", "io", "opcodes"],
                    "description": "Address space to write to (default: program)"
                }
            },
            "required": ["address", "data"]
        }
    },
    {
        "name": "get_registers",
        "description": "Get all CPU registers and their current values for the visible CPU.",
        "input_schema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "disassemble",
        "description": "Disassemble instructions at a given address. Returns formatted assembly listing with addresses, opcodes, and mnemonics.",
        "input_schema": {
            "type": "object",
            "properties": {
                "address": {
                    "type": "integer",
                    "description": "Address to start disassembling from"
                },
                "count": {
                    "type": "integer",
                    "description": "Number of instructions to disassemble (default 10, max 200)"
                }
            },
            "required": ["address"]
        }
    },
    {
        "name": "evaluate_expression",
        "description": "Evaluate a MAME debugger expression and return the result. Supports arithmetic, register names, memory access (b@addr, w@addr), and functions (min, max, if, abs, bit, s8, s16, s32). Numbers are hex by default; use # prefix for decimal.",
        "input_schema": {
            "type": "object",
            "properties": {
                "expression": {
                    "type": "string",
                    "description": "Debugger expression to evaluate. Examples: 'pc', 'pc+10', 'b@1234', 'a0==0'"
                }
            },
            "required": ["expression"]
        }
    },
    {
        "name": "get_execution_state",
        "description": "Get the current execution state: whether the CPU is stopped or running, the current PC, CPU type, and machine name.",
        "input_schema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "halt_execution",
        "description": "Break into the debugger, halting the emulated CPU. Use this when you need to inspect state while the game is running.",
        "input_schema": {
            "type": "object",
            "properties": {}
        }
    },
]


def execute_tool(client: MameClient, tool_name: str, tool_input: dict) -> str:
    """Execute a tool call against the MAME debugger and return the result as a string."""
    try:
        if tool_name == "execute_debugger_command":
            return client.command(tool_input["command"])

        elif tool_name == "read_memory":
            resp = client.read_memory(
                tool_input["address"],
                tool_input["length"],
                tool_input.get("space", "program"),
            )
            hex_data = resp.get("data", "")
            ascii_data = resp.get("ascii", "")
            addr = resp.get("addr", "")
            # Format as a readable hex dump
            lines = []
            for i in range(0, len(hex_data), 32):
                chunk_hex = hex_data[i:i+32]
                chunk_ascii = ascii_data[i//2:(i+32)//2]
                offset = tool_input["address"] + i // 2
                # Group hex into pairs with spaces
                hex_pairs = " ".join(chunk_hex[j:j+2] for j in range(0, len(chunk_hex), 2))
                lines.append(f"{offset:08X}: {hex_pairs:<48s} {chunk_ascii}")
            return "\n".join(lines) if lines else "(no data)"

        elif tool_name == "write_memory":
            return client.write_memory(
                tool_input["address"],
                tool_input["data"],
                tool_input.get("space", "program"),
            )

        elif tool_name == "get_registers":
            regs = client.get_registers()
            lines = []
            for name, value in regs.items():
                lines.append(f"  {name:8s} = {value}")
            return "\n".join(lines) if lines else "(no registers)"

        elif tool_name == "disassemble":
            return client.disassemble(
                tool_input["address"],
                tool_input.get("count", 10),
            )

        elif tool_name == "evaluate_expression":
            return client.evaluate(tool_input["expression"])

        elif tool_name == "get_execution_state":
            state = client.get_state()
            parts = [
                f"Status:  {'STOPPED' if state.get('stopped') else 'RUNNING'}",
                f"PC:      {state.get('pc', '?')}",
                f"CPU:     {state.get('cpu', '?')}",
                f"Machine: {state.get('machine', '?')}",
            ]
            return "\n".join(parts)

        elif tool_name == "halt_execution":
            return client.halt()

        else:
            return f"Unknown tool: {tool_name}"

    except Exception as e:
        return f"Error: {e}"
