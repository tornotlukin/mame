#!/usr/bin/env python3
"""
MAME LLM Debugger Bridge
==========================
Interactive chat interface that connects you to Claude + MAME debugger.

You are the eyes — describe what you see on screen.
Claude is the hands — issues debugger commands to investigate.

Usage:
  1. Start MAME:  cps1test qadjr -debug -debugger remote -debugger_port 12345
  2. Run bridge:  python bridge/bridge.py

Environment:
  ANTHROPIC_API_KEY  — your Anthropic API key (required)
  MAME_DEBUG_HOST    — MAME host (default: localhost)
  MAME_DEBUG_PORT    — MAME port (default: 12345)
"""

import os
import sys

# Add bridge directory to path so imports work
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import anthropic
from mame_client import MameClient
from claude_tools import TOOLS, execute_tool

SYSTEM_PROMPT = """You are an expert reverse engineer and MAME debugger operator. You are connected to a live MAME emulator running an arcade game via a remote debugging interface.

## Your Role
- The USER is watching the game on screen and will describe what they see (sprites, text, gameplay events).
- YOU issue debugger commands to investigate, set breakpoints, read memory, trace execution, and patch code.
- You correlate the user's visual observations with the emulated system's internal state.

## What You Know
- You have deep knowledge of classic CPU architectures: M68000, Z80, 6502, 8080, and others.
- You understand arcade hardware: video systems, sprite tables, tilemaps, palette RAM, I/O ports.
- You know common game programming patterns: main loops, vblank handlers, input polling, score tracking.

## CRITICAL: MAME Debugger Syntax Rules
- Numbers are HEXADECIMAL by default. 1234 means hex. Use # for decimal (#100 = 100 decimal).
- There is NO 'break' command — use execute_debugger_command with 'bp <address>' for breakpoints.
- There is NO 'run' or 'continue' — use 'g' (go) to resume execution.
- There is NO 'next' for stepping — use 's' (step) or 'o' (step over).
- Watchpoint syntax requires length AND type: 'wp 1234,1,w' (address, length, type).
- Use 'do <reg>=<value>' to set registers, NOT 'set'.
- Use 'b@<addr>' to read a byte, 'w@<addr>' for word, 'd@<addr>' for dword in expressions.
- Use 'find <addr>,<length>,<data>' to search memory. Wildcards: '?'
- Cheat search: cheatinit ub → play → cheatnext -,1 → repeat → cheatlist

## How to Work
1. Start by getting the execution state and registers to understand where the CPU is.
2. When the user describes something, form a hypothesis about what's happening in code.
3. Use targeted debugger commands to verify your hypothesis.
4. Explain your findings clearly — the user may not be deeply technical.
5. Ask the user to perform specific actions in the game when you need to observe changes.

## Safety
- Be careful with write_memory — you could crash the emulated game.
- Always confirm with the user before making patches.
- Use 'g' (go) to resume the game when the user needs to interact with it.
"""


def main():
    # Check for API key
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if not api_key:
        print("Error: ANTHROPIC_API_KEY environment variable not set.")
        print("Set it with: set ANTHROPIC_API_KEY=your-key-here")
        sys.exit(1)

    host = os.environ.get("MAME_DEBUG_HOST", "localhost")
    port = int(os.environ.get("MAME_DEBUG_PORT", "12345"))

    # Connect to MAME
    print("MAME LLM Debugger Bridge")
    print("=" * 50)
    print(f"Connecting to MAME at {host}:{port}...")

    mame = MameClient(host, port)
    try:
        mame.connect()
    except ConnectionRefusedError:
        print(f"\nConnection refused. Make sure MAME is running with:")
        print(f"  cps1test qadjr -debug -debugger remote -debugger_port {port}")
        sys.exit(1)

    # Get initial state
    state = mame.get_state()
    cpu = state.get("cpu", "unknown")
    machine = state.get("machine", "unknown")
    pc = state.get("pc", "?")
    print(f"Connected! Machine: {machine}, CPU: {cpu}, PC: {pc}")
    print()
    print("You are the eyes. Describe what you see on screen.")
    print("Claude will investigate using the debugger.")
    print("Type 'quit' or 'exit' to stop. Type 'go' to resume the game.")
    print("=" * 50)

    # Initialize Claude client
    client = anthropic.Anthropic(api_key=api_key)
    messages = []

    # Add initial context about the connected system
    initial_context = f"Connected to MAME. Machine: {machine}, CPU: {cpu}, PC at {pc}. The game is currently stopped in the debugger. I can see the emulated system is ready."

    while True:
        try:
            user_input = input("\nYou > ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nExiting.")
            break

        if not user_input:
            continue
        if user_input.lower() in ("quit", "exit"):
            break
        if user_input.lower() == "go":
            mame.command("g")
            print("[Game resumed. Press tilde (~) in MAME to break back in, or type 'halt' here.]")
            continue
        if user_input.lower() == "halt":
            mame.halt()
            print("[Execution halted.]")
            continue

        # Build message
        if not messages:
            # First message includes system context
            user_input = f"[System: {initial_context}]\n\n{user_input}"

        messages.append({"role": "user", "content": user_input})

        # Call Claude with tools
        while True:
            response = client.messages.create(
                model="claude-sonnet-4-20250514",
                max_tokens=4096,
                system=SYSTEM_PROMPT,
                tools=TOOLS,
                messages=messages,
            )

            # Process response
            assistant_content = response.content
            messages.append({"role": "assistant", "content": assistant_content})

            # Check for tool use
            tool_uses = [b for b in assistant_content if b.type == "tool_use"]

            if not tool_uses:
                # No tool calls — print the text response
                for block in assistant_content:
                    if hasattr(block, "text"):
                        print(f"\nClaude > {block.text}")
                break

            # Execute tool calls
            tool_results = []
            for tool_use in tool_uses:
                print(f"  [{tool_use.name}] {tool_use.input}")
                result = execute_tool(mame, tool_use.name, tool_use.input)
                # Show abbreviated result
                result_preview = result[:200] + "..." if len(result) > 200 else result
                print(f"  -> {result_preview}")
                tool_results.append({
                    "type": "tool_result",
                    "tool_use_id": tool_use.id,
                    "content": result,
                })

            messages.append({"role": "user", "content": tool_results})
            # Loop back to get Claude's next response

    mame.close()
    print("Disconnected from MAME.")


if __name__ == "__main__":
    main()
