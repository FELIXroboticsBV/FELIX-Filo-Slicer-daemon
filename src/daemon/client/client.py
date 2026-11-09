"""Tiny test client for the Orca daemon.
!!! this is AI slop as i do not have time to write proper test client 

Usage:
  orca_client.py [--socket PATH]                     interactive mode
  orca_client.py [--socket PATH] CMD [PARAMS_JSON]   send one command and exit

Interactive input:
  status                                   command with no params
  load_model {"path": "/tmp/part.stl"}     command + params as JSON
  {"cmd": "status"}                        raw line, sent exactly as typed
  quit                                     exit
"""
import argparse
import itertools
import json
import socket
import sys
import threading
from pathlib import Path

DEFAULT_SOCKET = "/run/user/1000/felix-daemon"


ASSETS = Path(__file__).resolve().parent / "assets"
MODEL  = str(ASSETS / "dino.stl")
GCODE  = str(ASSETS / "dino.gcode")

SHORTCUTS = {
    "load":    ("load_model",  {"path": MODEL}),
    "printer": ("set_printer", {"printer_key": "felix_pro_3", "nozzle_size": 0.35}),
    "filament": ("set_filament", {"filament_key": "felix_petg", "nozzle_key": "hardened_steel", "extruder": 0}),
    
    "printer_food": ("set_printer", {"printer_key": "felix_food_single", "nozzle_size": 3.0}),
    "filament_food": ("set_filament", {"filament_key": "food_thick", "nozzle_key": "stainless_steel", "extruder": 0}),

    "process_normal": ("set_process", {"process_key": "normal"}),
    "process_coarse": ("set_process", {"process_key": "coarse"}),
    
    "slice":   ("slice",       {"modelPath": MODEL, "outputPath": GCODE}),
    "status":  ("slice_status",  {}),
    "info": ("get_preset_info", {}),
}
SEQUENCES = {
    "all": ["load", "printer", "filament", "process_normal", "slice"],
}

def run_shortcut(client, name, timeout):
    for step in SEQUENCES.get(name, [name]):
        cmd, params = SHORTCUTS[step]
        reply = client.request(cmd, params, timeout)
        if not reply or "error" in reply:
            return reply
    return reply

def print_help():
    print("Commands (local, not sent):")
    print("  help                 show this help")
    print("  quit / exit          leave the client")
    print("Shortcuts:")
    for name, (cmd, params) in SHORTCUTS.items():
        print(f"  {name:<20} {cmd} {json.dumps(params)}")
    for name, steps in SEQUENCES.items():
        print(f"  {name:<20} runs: {' -> '.join(steps)}")
    print("Manual:")
    print("  cmd {json}           e.g. load_model {\"path\": \"/tmp/part.stl\"}")
    print("  {raw json}           sent as-is, reply not awaited")

class Client:
    def __init__(self, path):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(path)
        self.ids = itertools.count(1)
        self.lock = threading.Lock()
        self.waiting = {}   # id -> threading.Event
        self.replies = {}   # id -> reply dict
        self.closed = threading.Event()
        threading.Thread(target=self._reader, daemon=True).start()

    # Background thread: prints everything the server sends (replies and events).
    def _reader(self):
        buf = b""
        while True:
            try:
                chunk = self.sock.recv(4096)
            except OSError:
                break
            if not chunk:
                break
            buf += chunk
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                self._handle_line(line.decode("utf-8", "replace").rstrip("\r"))
        print("-- server closed the connection")
        self.closed.set()
        with self.lock:
            for ev in self.waiting.values():
                ev.set()

    def _handle_line(self, line):
        if not line:
            return
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            print(f"<< (not JSON) {line}")
            return
        tag = "<< event" if "event" in msg else "<<"
        print(tag, json.dumps(msg, indent=2))
        rid = msg.get("id")
        with self.lock:
            if rid in self.waiting:
                self.replies[rid] = msg
                self.waiting[rid].set()

    def send_raw(self, text):
        data = text.rstrip("\n") + "\n"
        print(">>", data, end="")
        self.sock.sendall(data.encode())

    # Send a request and wait for the reply with the matching id.
    def request(self, cmd, params=None, timeout=None):
        rid = next(self.ids)
        ev = threading.Event()
        with self.lock:
            self.waiting[rid] = ev
        msg = {"id": rid, "cmd": cmd}
        if params is not None:
            msg["params"] = params
        self.send_raw(json.dumps(msg, separators=(",", ":")))  # compact, one line
        try:
            if not ev.wait(timeout):
                print(f"!! no reply with id {rid} within {timeout}s")
        except KeyboardInterrupt:
            print("\n!! stopped waiting")
        with self.lock:
            self.waiting.pop(rid, None)
            return self.replies.pop(rid, None)

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def parse_input(line):
    """'cmd {json}' -> (cmd, params). Raises ValueError on bad JSON."""
    cmd, _, rest = line.strip().partition(" ")
    rest = rest.strip()
    params = json.loads(rest) if rest else None
    if params is not None and not isinstance(params, dict):
        raise ValueError("params must be a JSON object")
    return cmd, params


def interactive(client, timeout):
    print("Connected. Type 'help' for options.")
    while not client.closed.is_set():
        try:
            line = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not line:
            continue
        if line in ("quit", "exit"):
            break
        if line in ("help", "?"):
            print_help()
            continue
        if line in SHORTCUTS or line in SEQUENCES:
            run_shortcut(client, line, timeout)
            continue
        if line.startswith("{"):
            client.send_raw(line)       # raw: sent as-is, not waited on
            continue
        try:
            cmd, params = parse_input(line)
        except ValueError as e:
            print(f"!! bad params: {e}")
            continue
        client.request(cmd, params, timeout)


def main():
    ap = argparse.ArgumentParser(description="Test client for the Orca daemon")
    ap.add_argument("--socket", default=DEFAULT_SOCKET, help="unix socket path")
    ap.add_argument("--timeout", type=float, default=None,
                    help="seconds to wait for a reply (default: wait forever)")
    ap.add_argument("cmd", nargs="?", help="command to send (omit for interactive)")
    ap.add_argument("params", nargs="?", help="params as a JSON object")
    args = ap.parse_args()

    try:
        client = Client(args.socket)
    except OSError as e:
        sys.exit(f"cannot connect to {args.socket}: {e}")

    try:
        if args.cmd is None:
            interactive(client, args.timeout)
            return 0
        if args.cmd in SHORTCUTS or args.cmd in SEQUENCES:
            reply = run_shortcut(client, args.cmd, args.timeout)
        else:
            params = json.loads(args.params) if args.params else None
            reply = client.request(args.cmd, params, args.timeout)
        return 0 if reply and "error" not in reply else 1
    finally:
        client.close()


if __name__ == "__main__":
    sys.exit(main())