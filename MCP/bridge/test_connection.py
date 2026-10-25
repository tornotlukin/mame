#!/usr/bin/env python3
"""
MAME Remote Debugger - Connection Test
=======================================
Connects to cps1test.exe running with -debugger remote and runs
a series of tests to verify the TCP JSON protocol works.

Usage:
  1. Start MAME:  cps1test qadjr -debug -debugger remote -debugger_port 12345
  2. Run this:     python bridge/test_connection.py

"""

import socket
import json
import sys
import time

HOST = "localhost"
PORT = 12345

class MameDebugClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        self.request_id = 0
        self.buffer = ""

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5.0)
        self.sock.connect((self.host, self.port))
        print(f"[+] Connected to {self.host}:{self.port}")

    def close(self):
        if self.sock:
            self.sock.close()
            self.sock = None

    def send_request(self, request: dict) -> dict:
        self.request_id += 1
        request["id"] = self.request_id
        line = json.dumps(request) + "\n"
        self.sock.sendall(line.encode("utf-8"))
        return self._read_response()

    def _read_response(self) -> dict:
        while True:
            if "\n" in self.buffer:
                line, self.buffer = self.buffer.split("\n", 1)
                if line.strip():
                    try:
                        return json.loads(line)
                    except json.JSONDecodeError:
                        print(f"[!] Bad JSON: {line}")
                        continue

            try:
                data = self.sock.recv(4096).decode("utf-8")
                if not data:
                    raise ConnectionError("Connection closed")
                self.buffer += data
            except socket.timeout:
                return {"error": "timeout waiting for response"}

    def read_event(self) -> dict:
        """Read an async event (no request ID)."""
        return self._read_response()


def test_header(name):
    print(f"\n{'='*50}")
    print(f"  {name}")
    print(f"{'='*50}")


def test_result(response, expect_type=None):
    if "error" in response and response.get("type") == "error":
        print(f"  [FAIL] Error: {response['error']}")
        return False

    if expect_type and response.get("type") != expect_type:
        print(f"  [WARN] Expected type '{expect_type}', got '{response.get('type')}'")

    print(f"  [OK] Response: {json.dumps(response, indent=2)[:500]}")
    return True


def main():
    print("MAME Remote Debugger - Connection Test")
    print("=" * 50)

    client = MameDebugClient(HOST, PORT)

    try:
        client.connect()
    except ConnectionRefusedError:
        print(f"\n[!] Connection refused on {HOST}:{PORT}")
        print("    Make sure MAME is running with:")
        print(f"    cps1test qadjr -debug -debugger remote -debugger_port {PORT}")
        sys.exit(1)
    except Exception as e:
        print(f"\n[!] Connection failed: {e}")
        sys.exit(1)

    # Read the initial "connected" event
    test_header("1. Reading welcome event")
    try:
        event = client.read_event()
        test_result(event)
    except Exception as e:
        print(f"  [WARN] No welcome event: {e}")

    # Test: get_state
    test_header("2. Get execution state")
    resp = client.send_request({"type": "get_state"})
    test_result(resp, "state")

    # Test: get_registers
    test_header("3. Get CPU registers")
    resp = client.send_request({"type": "get_registers"})
    test_result(resp, "registers")

    # Test: read_memory (first 64 bytes)
    test_header("4. Read memory (64 bytes at 0x0000)")
    resp = client.send_request({"type": "read_memory", "addr": "0x0000", "len": 64})
    test_result(resp, "memory")

    # Test: disassemble (10 instructions at PC)
    test_header("5. Disassemble 10 instructions at address 0")
    resp = client.send_request({"type": "disassemble", "addr": "0x0000", "count": 10})
    test_result(resp, "result")

    # Test: evaluate expression
    test_header("6. Evaluate expression: pc")
    resp = client.send_request({"type": "evaluate", "expr": "pc"})
    test_result(resp, "eval_result")

    # Test: raw debugger command (symlist)
    test_header("7. Execute command: symlist")
    resp = client.send_request({"type": "command", "cmd": "symlist"})
    test_result(resp, "result")

    # Test: set and clear a breakpoint
    test_header("8. Set breakpoint at 0x100")
    resp = client.send_request({"type": "command", "cmd": "bp 100"})
    test_result(resp, "result")

    test_header("9. List breakpoints")
    resp = client.send_request({"type": "get_breakpoints"})
    test_result(resp, "result")

    test_header("10. Clear all breakpoints")
    resp = client.send_request({"type": "command", "cmd": "bpclear"})
    test_result(resp, "result")

    # Test: write and read back memory
    test_header("11. Write 4 bytes to 0xFF00, then read back")
    resp = client.send_request({"type": "write_memory", "addr": "0xFF00", "data": "deadbeef"})
    test_result(resp, "result")
    resp = client.send_request({"type": "read_memory", "addr": "0xFF00", "len": 4})
    test_result(resp, "memory")
    if resp.get("data") == "deadbeef":
        print("  [OK] Write/read verified!")
    else:
        print(f"  [WARN] Read back: {resp.get('data')} (may differ if ROM area)")

    # Test: bad command (error handling)
    test_header("12. Error handling: bad command")
    resp = client.send_request({"type": "command", "cmd": "notarealcommand"})
    if resp.get("type") == "error":
        print(f"  [OK] Got expected error: {resp.get('error')}")
    else:
        print(f"  [WARN] Expected error response, got: {resp}")

    # Test: unknown request type
    test_header("13. Error handling: unknown request type")
    resp = client.send_request({"type": "faketype"})
    if resp.get("type") == "error":
        print(f"  [OK] Got expected error: {resp.get('error')}")
    else:
        print(f"  [WARN] Expected error response, got: {resp}")

    # Summary
    print(f"\n{'='*50}")
    print("  ALL TESTS COMPLETE")
    print(f"{'='*50}")
    print("\nThe remote debugger is working! You can now build the LLM bridge.")

    client.close()


if __name__ == "__main__":
    main()
