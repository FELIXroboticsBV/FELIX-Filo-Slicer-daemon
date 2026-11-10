import os

IDENTITY = [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]

def translation(x, y, z=0):
    return [[1, 0, 0, x], [0, 1, 0, y], [0, 0, 1, z], [0, 0, 0, 1]]


# ---------------------------------------------------------------- presets

def test_set_printer(client):
    data = client.ok("set_printer", printer_key="felix_pro_3", nozzle_size=0.35)
    assert data["printer"] == "FELIX Pro 3 0.35mm nozzle"
    assert len(data["bed"]) >= 4
    assert data["printable_height"] > 0

def test_set_printer_unknown_key(client):
    assert client.call("set_printer", printer_key="nope", nozzle_size=0.35)["code"] == 500

def test_set_printer_bad_nozzle(client):
    assert client.call("set_printer", printer_key="felix_pro_3", nozzle_size=0)["code"] == 500

def test_set_filament_food_on_plastic_printer(client):
    client.ok("set_printer", printer_key="felix_pro_3", nozzle_size=0.50)
    resp = client.call("set_filament", filament_key="food_thin", nozzle_key="plastic", extruder=0)
    assert resp["code"] == 500

def test_set_process(client):
    client.ok("set_printer", printer_key="felix_pro_3", nozzle_size=0.35)
    data = client.ok("set_process", process_key="normal")
    assert data["process"] == "FELIX Plastic Normal @ 0.35mm"

def test_get_state(client, pro3, cube):
    data = client.ok("get_state")
    assert data["printer_key"] == "felix_pro_3"
    #assert cube in data["models"]


# ---------------------------------------------------------------- models

def test_add_instance_and_position(client, cube):
    data = client.ok("add_instance", model_key=cube, transform=translation(50, 60))
    assert data["count"] == 2
    assert data["position"][:2] == [50, 60]

def test_set_transform(client, cube):
    data = client.ok("set_transform", objects=[
        {"model_key": cube, "instance_id": 0, "transform": translation(100, 100)}])
    assert data[0]["position"][:2] == [100, 100]

def test_set_transform_out_of_range(client, cube):
    resp = client.call("set_transform", objects=[
        {"model_key": cube, "instance_id": 5, "transform": IDENTITY}])
    assert resp["code"] == "failed"

def test_remove_instance_then_last_removes_model(client, cube):
    client.ok("add_instance", model_key=cube)
    data = client.ok("remove_instance", model_key=cube, instance_id=1)
    assert data["count"] == 1 and data["model_removed"] is False

    data = client.ok("remove_instance", model_key=cube, instance_id=0)
    assert data["model_removed"] is True
    assert cube not in client.ok("get_state")["models"]

def test_unknown_model_key(client):
    assert client.call("remove_model", model_key="/does/not/exist.stl")["code"] == "invalid_parameters"


# ---------------------------------------------------------------- slicing

def test_slice(client, pro3, cube, tmp_path):
    out = tmp_path / "cube.gcode"
    client.ok("set_transform", objects=[
        {"model_key": cube, "instance_id": 0, "transform": translation(100, 100)}])

    client.ok("slice", modelPath=cube, outputPath=str(out))
    status = client.wait_for_slice()

    assert status["state"] == "done", status.get("error")
    assert os.path.getsize(out) > 0
    assert "G1 " in out.read_text()

def test_reslice_unchanged_reuses_gcode(client, pro3, cube, tmp_path):
    client.ok("set_transform", objects=[
        {"model_key": cube, "instance_id": 0, "transform": translation(100, 100)}])
    client.ok("slice", modelPath=cube, outputPath=str(tmp_path / "a.gcode"))
    assert client.wait_for_slice()["state"] == "done"

    data = client.ok("slice", modelPath=cube, outputPath=str(tmp_path / "b.gcode"))
    assert "unchanged" in data["status"]
    assert (tmp_path / "b.gcode").exists()

def test_slice_without_model(client, pro3, tmp_path):
    resp = client.call("slice", modelPath="x", outputPath=str(tmp_path / "x.gcode"))
    assert resp["code"] == "failed"

def test_busy_while_slicing(client, pro3, cube, tmp_path):
    client.ok("set_transform", objects=[
        {"model_key": cube, "instance_id": 0, "transform": translation(90, 90)}])  # force a real slice
    client.ok("slice", modelPath=cube, outputPath=str(tmp_path / "c.gcode"))
    resp = client.call("remove_model", model_key=cube)
    client.wait_for_slice()
    assert resp["code"] == "busy"  # can be flaky if the cube slices faster than the request arrives