import itertools, json, os, shlex, socket, subprocess, time
import pytest

SOCKET_PATH = "/run/user/1000/felix-daemon"
DAEMON_CMD  = "./out/build/orca-daemon-relwithdebinfo/src/daemon/orca-daemon"


class DaemonClient:
    def __init__(self, path):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(path)
        self.file = self.sock.makefile("rwb")
        self.ids  = itertools.count(1)

    def call(self, cmd, **params):
        """Send a request and return the full response."""
        req_id = next(self.ids)
        self.file.write((json.dumps({"id": req_id, "cmd": cmd, "params": params}) + "\n").encode())
        self.file.flush()
        line = self.file.readline()
        if not line:
            raise ConnectionError("daemon closed the connection")
        resp = json.loads(line)
        assert resp["id"] == req_id, f"response id {resp['id']} != request id {req_id}"
        return resp

    def ok(self, cmd, **params):
        """Send a request, assert it succeeded and return its data."""
        resp = self.call(cmd, **params)
        assert resp["code"] == 200, f"{cmd} -> {resp['code']}: {resp.get('message')}"
        return resp.get("data")

    def wait_for_slice(self, timeout=120):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            status = self.ok("slice_status")
            if status["state"] in ("done", "failed", "canceled"):
                return status
            time.sleep(0.2)
        pytest.fail(f"slice did not finish within {timeout}s")

    def close(self):
        self.file.close()
        self.sock.close()


def _connect(path, proc, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc and proc.poll() is not None:
            pytest.fail(f"daemon exited during startup with code {proc.returncode}")
        try:
            return DaemonClient(path)
        except (FileNotFoundError, ConnectionRefusedError):
            time.sleep(0.2)
    pytest.fail(f"could not connect to {path} within {timeout}s")


@pytest.fixture(scope="session")
def client():
    proc = None
    if DAEMON_CMD:
        if os.path.exists(SOCKET_PATH):
            os.remove(SOCKET_PATH)  # stale socket from a crashed run
        proc = subprocess.Popen(shlex.split(DAEMON_CMD))

    c = _connect(SOCKET_PATH, proc)
    yield c
    c.close()

    if proc:
        proc.terminate()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()


def _write_cube_stl(path, size=20.0):
    """ASCII STL cube, centered in XY, standing on z = 0."""
    h = size / 2
    v = [(-h, -h, 0), (h, -h, 0), (h, h, 0), (-h, h, 0),
         (-h, -h, size), (h, -h, size), (h, h, size), (-h, h, size)]
    faces = [(0, 2, 1), (0, 3, 2), (4, 5, 6), (4, 6, 7), (0, 1, 5), (0, 5, 4),
             (1, 2, 6), (1, 6, 5), (2, 3, 7), (2, 7, 6), (3, 0, 4), (3, 4, 7)]
    with open(path, "w") as f:
        f.write("solid cube\n")
        for a, b, c in faces:
            f.write(" facet normal 0 0 0\n  outer loop\n")
            for i in (a, b, c):
                f.write(f"   vertex {v[i][0]} {v[i][1]} {v[i][2]}\n")
            f.write("  endloop\n endfacet\n")
        f.write("endsolid cube\n")


@pytest.fixture
def cube(client, tmp_path):
    """Loads a cube onto the plate and removes it again after the test."""
    path = tmp_path / "cube.stl"
    _write_cube_stl(path)
    data = client.ok("load_model", path=str(path))
    key = data[0]["model_key"]
    yield key
    client.call("remove_model", model_key=key)  # may already be gone, ignore the result


@pytest.fixture
def pro3(client):
    """FELIX Pro 3, 0.35 hardened steel, PLA, normal quality."""
    client.ok("set_printer", printer_key="felix_pro_3", nozzle_size=0.35)
    client.ok("set_filament", filament_key="felix_premium_pla", nozzle_key="hardened_steel", extruder=0)
    client.ok("set_process", process_key="normal")