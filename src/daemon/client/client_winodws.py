"""Tiny test client for the Orca daemon, Windows version of client.py.

Usage:
  python client_winodws.py [--socket PATH]                     interactive mode
  python client_winodws.py [--socket PATH] CMD [PARAMS_JSON]   send one command and exit

Interactive input:
  status                                   command with no params
  load_model {"path": "C:/tmp/part.stl"}   command + params as JSON
  {"cmd": "status"}                        raw line, sent exactly as typed
  quit                                     exit

The daemon listens on an AF_UNIX socket (Windows 10 1803+) at %TEMP%\\felix-daemon,
see socket_path() in src/daemon/paths.hpp. Many Python builds for Windows do not expose
socket.AF_UNIX, so the connection is made through Winsock with ctypes and then handed
to a normal Python socket object.
"""
import argparse
import ctypes
import itertools
import json
import socket
import sys
import threading
import time
import math
import random
import struct
from pathlib import Path

SOCKET_NAME = "felix-daemon"  # daemon_config::socketName (config.ini: socket_name)

AF_UNIX      = 1
SUN_PATH_LEN = 108  # sockaddr_un::sun_path in afunix.h


ASSETS = Path(__file__).resolve().parent / "assets"
MODEL  = str(ASSETS / "dino.stl")
GCODE  = str(ASSETS / "dino.gcode")

BED = (235.0, 244.0)  # FELIX Pro 3 printable area


# ---------------------------------------------------------------------------
# Windows socket plumbing

def temp_directory():
    """The folder the daemon puts its socket in: boost temp_directory_path() uses GetTempPathW."""
    buf = ctypes.create_unicode_buffer(32767)
    if ctypes.windll.kernel32.GetTempPathW(len(buf), buf) == 0:
        raise ctypes.WinError()
    return buf.value.rstrip("\\")


def default_socket():
    try:
        return temp_directory() + "\\" + SOCKET_NAME
    except (AttributeError, OSError):  # not on Windows
        return SOCKET_NAME


class sockaddr_un(ctypes.Structure):
    _fields_ = [("sun_family", ctypes.c_ushort), ("sun_path", ctypes.c_char * SUN_PATH_LEN)]


def connect_af_unix(path):
    """Connected AF_UNIX stream socket, with or without socket.AF_UNIX in this Python."""
    if hasattr(socket, "AF_UNIX"):
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(path)
        return sock

    # Winsock is already started: importing the socket module calls WSAStartup.
    ws2 = ctypes.WinDLL("ws2_32", use_last_error=True)
    SOCKET = ctypes.c_size_t  # UINT_PTR
    ws2.socket.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
    ws2.socket.restype = SOCKET
    ws2.connect.argtypes = [SOCKET, ctypes.c_void_p, ctypes.c_int]
    ws2.connect.restype = ctypes.c_int
    ws2.closesocket.argtypes = [SOCKET]
    invalid_socket = SOCKET(-1).value

    # The daemon binds a narrow (ANSI code page) path, so encode the same way.
    encoded = path.encode("mbcs")
    if len(encoded) >= SUN_PATH_LEN:
        raise OSError(f"socket path is too long ({len(encoded)} >= {SUN_PATH_LEN} bytes)")

    fd = ws2.socket(AF_UNIX, socket.SOCK_STREAM, 0)
    if fd == invalid_socket:
        raise ctypes.WinError(ctypes.get_last_error())
    addr = sockaddr_un(AF_UNIX, encoded)
    if ws2.connect(fd, ctypes.byref(addr), ctypes.sizeof(addr)) != 0:
        err = ctypes.get_last_error()
        ws2.closesocket(fd)
        raise ctypes.WinError(err)
    # Python owns the handle from here on and closes it with closesocket.
    return socket.socket(fileno=fd)


def wait_interruptible(ev, timeout):
    """Event.wait in short slices: on Windows a long Event.wait ignores Ctrl+C."""
    deadline = None if timeout is None else time.monotonic() + timeout
    while not ev.is_set():
        left = 0.2 if deadline is None else min(0.2, deadline - time.monotonic())
        if left <= 0:
            return False
        ev.wait(left)
    return True


# ---------------------------------------------------------------------------
# Model placement

def stl_bbox(path):
    """(min, max) corners of the vertices of a binary or ASCII STL."""
    data = Path(path).read_bytes()
    verts = []
    count = struct.unpack_from("<I", data, 80)[0] if len(data) >= 84 else 0
    if len(data) == 84 + 50 * count:  # binary: 12 floats + 2 bytes per triangle, skip the normal
        for i in range(count):
            f = struct.unpack_from("<12f", data, 84 + 50 * i)
            verts += [f[3:6], f[6:9], f[9:12]]
    else:
        for line in data.decode("utf-8", "replace").splitlines():
            parts = line.split()
            if parts[:1] == ["vertex"]:
                verts.append(tuple(map(float, parts[1:4])))
    return [min(v[k] for v in verts) for k in range(3)], [max(v[k] for v in verts) for k in range(3)]


# the daemon centers the model around its origin, like the scene does, so the matrix places the
# center of the bounding box: lift it by half of the height to put it on the bed
BB_MIN, BB_MAX = stl_bbox(MODEL)
HALF_HEIGHT    = (BB_MAX[2] - BB_MIN[2]) / 2
RADIUS         = math.hypot(BB_MAX[0] - BB_MIN[0], BB_MAX[1] - BB_MIN[1]) / 2  # stays on the bed when rotated


def placement(x, y, angle=0.0):
    c, s = math.cos(angle), math.sin(angle)
    return [c, -s, 0, x,
            s,  c, 0, y,
            0,  0, 1, HALF_HEIGHT,
            0,  0, 0, 1]


def random_transform(model_key=MODEL, instance=1, rotate=False):
    x = random.uniform(RADIUS, BED[0] - RADIUS)
    y = random.uniform(RADIUS, BED[1] - RADIUS)
    a = random.uniform(0, 2 * math.pi) if rotate else 0.0
    return {"objects": [{"model_key": model_key, "instance_id": instance, "transform": placement(x, y, a)}]}


SHORTCUTS = {
    "load":    ("load_model",  {"path": MODEL, "transform": placement(BED[0] / 2, BED[1] / 2)}),
    "printer": ("set_printer", {"printer_key": "felix_pro_3", "nozzle_size": 0.35}),
    "filament": ("set_filament", {"filament_key": "felix_petg", "nozzle_key": "hardened_steel", "extruder": 0}),

    "printer_food": ("set_printer", {"printer_key": "felix_food_single", "nozzle_size": 3.0}),
    "filament_food": ("set_filament", {"filament_key": "food_thick", "nozzle_key": "stainless_steel", "extruder": 0}),

    "process_normal": ("set_process", {"process_key": "normal"}),
    "process_coarse": ("set_process", {"process_key": "coarse"}),

    "remove_model": ("remove_model" , {"model_key": MODEL}),
    "remove_instance":("remove_instance" , {"model_key":MODEL, "instance_id": 0}),

    "move": ("set_transform", lambda: random_transform(instance=0,rotate=True)),
    "move_instance": ("set_transform", lambda: random_transform(instance=1)),
    "move_rotate": ("set_transform", lambda: random_transform(rotate=True)),

    "add_instance" : ("add_instance", {"model_key": MODEL}),

    "slice":   ("slice",       {"modelPath": MODEL, "outputPath": GCODE}),
    "status":  ("slice_status",  {}),
    "info": ("get_state", {}),
}
SEQUENCES = {
    "all": ["load", "printer", "filament", "process_normal", "slice"],
}


def run_shortcut(client, name, timeout):
    for step in SEQUENCES.get(name, [name]):
        cmd, params = SHORTCUTS[step]
        if callable(params):
            params = params()
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
        shown = "<generated>" if callable(params) else json.dumps(params)
        print(f"  {name:<20} {cmd} {shown}")
    for name, steps in SEQUENCES.items():
        print(f"  {name:<20} runs: {' -> '.join(steps)}")
    print("Manual:")
    print("  cmd {json}           e.g. load_model {\"path\": \"C:/tmp/part.stl\"}")
    print("  {raw json}           sent as-is, reply not awaited")


# ---------------------------------------------------------------------------
# Client

class Client:
    def __init__(self, path):
        self.sock = connect_af_unix(path)
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
            if not wait_interruptible(ev, timeout):
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
    ap = argparse.ArgumentParser(description="Test client for the Orca daemon (Windows)")
    ap.add_argument("--socket", default=default_socket(), help="AF_UNIX socket path")
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
