"""
MAME Remote Debugger TCP Client
================================
Handles the TCP connection to MAME's remote debugger module.
Sends JSON-line requests and receives responses/events.
"""

import socket
import json
import threading
import queue


class MameClient:
    def __init__(self, host="localhost", port=12345):
        self.host = host
        self.port = port
        self.sock = None
        self.buffer = ""
        self.request_id = 0
        self.event_queue = queue.Queue()
        self._lock = threading.Lock()

    def connect(self):
        self.close()
        self.buffer = ""
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(10.0)
        self.sock.connect((self.host, self.port))

        # Drain any welcome event
        try:
            self._read_response(timeout=1.0)
        except Exception:
            pass

    def close(self):
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None

    def _ensure_connected(self):
        """Reconnect if the connection was lost."""
        if self.sock is None:
            self.connect()
            return
        # Test if socket is still alive
        try:
            self.sock.getpeername()
        except Exception:
            self.connect()

    def send(self, request: dict) -> dict:
        with self._lock:
            self._ensure_connected()
            self.request_id += 1
            request["id"] = self.request_id
            line = json.dumps(request) + "\n"
            try:
                self.sock.sendall(line.encode("utf-8"))
                return self._read_response()
            except (ConnectionError, OSError):
                # Reconnect and retry once
                self.connect()
                self.request_id += 1
                request["id"] = self.request_id
                line = json.dumps(request) + "\n"
                self.sock.sendall(line.encode("utf-8"))
                return self._read_response()

    def _read_response(self, timeout=10.0) -> dict:
        old_timeout = self.sock.gettimeout()
        self.sock.settimeout(timeout)
        try:
            while True:
                if "\n" in self.buffer:
                    line, self.buffer = self.buffer.split("\n", 1)
                    if line.strip():
                        msg = json.loads(line)
                        # If it's an event (no id), queue it and keep reading
                        if "id" not in msg and msg.get("type") == "event":
                            self.event_queue.put(msg)
                            continue
                        return msg
                data = self.sock.recv(4096).decode("utf-8")
                if not data:
                    raise ConnectionError("Connection closed by MAME")
                self.buffer += data
        finally:
            self.sock.settimeout(old_timeout)

    # Convenience methods for each request type

    def command(self, cmd: str) -> str:
        resp = self.send({"type": "command", "cmd": cmd})
        if resp.get("type") == "error":
            raise RuntimeError(f"Command error: {resp.get('error')}")
        return resp.get("output", "")

    def read_memory(self, addr: int, length: int, space: str = "program") -> dict:
        resp = self.send({
            "type": "read_memory",
            "addr": hex(addr),
            "len": length,
            "space": space,
        })
        return resp

    def write_memory(self, addr: int, data: str, space: str = "program") -> str:
        resp = self.send({
            "type": "write_memory",
            "addr": hex(addr),
            "data": data,
            "space": space,
        })
        return resp.get("output", "")

    def get_registers(self) -> dict:
        resp = self.send({"type": "get_registers"})
        return resp.get("regs", {})

    def disassemble(self, addr: int, count: int = 10) -> str:
        resp = self.send({
            "type": "disassemble",
            "addr": hex(addr),
            "count": count,
        })
        return resp.get("output", "")

    def evaluate(self, expr: str) -> str:
        resp = self.send({"type": "evaluate", "expr": expr})
        return resp.get("result", "")

    def get_state(self) -> dict:
        resp = self.send({"type": "get_state"})
        return resp

    def halt(self) -> str:
        resp = self.send({"type": "break"})
        return resp.get("output", "")
