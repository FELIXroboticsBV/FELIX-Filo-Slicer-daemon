# FELIX Filo slicing daemon

> In computing, a daemon (/ˈdiːmən/ DEE-mən or /ˈdeɪmən/ DAY-mən) is a program that runs as a background process, rather than being under the direct control of an interactive user. _(Wikipedia)_

A daemon application that allows other applications to utilise Orca's slicing capabilities without linking directly against Orca Slicer.

## High-level overview

<p align="center">
  <img width="600" src="./resources/readme_images/high-level-overview.png" />
</p>

The project uses a client–server architecture: the daemon acts as the server, and the client is any application that wants to use Orca's slicing capabilities.

Communication happens through a local socket, which supports bidirectional (two-way) communication between two separate processes (programs) on the same computer. The server side (the daemon) is responsible for creating this socket. Once the socket is created, the server listens on it for incoming connections and requests.

Messages are exchanged as JSON with standardized fields.

## Results

<p align="center">
  <img width="600" src="./resources/readme_images/daemon-vs-orcagui.png.png" />
</p>

Comparison between slicing user the daemon (left) vs Orca 4.5 GUI (right).

**Config**

- FELIX Pro3 with 0.35 mm nozzle
- normal quality

As can be seen there is no difference in results.

### Request

Request json composes of `id` field, `cmd` field and `params` field.

- `id` : represents unique identifier for the request, client is responsible to provide the id to the server, thanks to this various requests can be easily identified.

- `cmd`: string with predefined commands such as `slice_status` or `slice`.

- `params`: arbitrary json object containing parameters for the request, server will attempt to extract the parameters from this embeded json object.

### Response

Similar to the request, response is returned in the standardised json format containing various fields.

- `id`: this id is referring ot the id of the request to which server is responding

- `code`: code of the response, `200`OK, `404`NOT FOUND etc.

- `data`: this can be thought of as a body of the response it provides client with data necessary to work with the results of the slicing procedure or model loading.

On error, `data` is omitted and `message` explains the problem.

**Response codes**

| code                 | meaning                                     |
| -------------------- | ------------------------------------------- |
| `ok`                 | command succeeded                           |
| `failed`             | command was valid but could not be executed |
| `not_found`          | requested resource does not exist           |
| `invalid_parameters` | `params` missing or malformed               |
| `busy`               | a slice is already running                  |

### Typical workflow

1. `load_model`: load and cache a mesh
2. `set_printer`: select the FELIX printer and nozzle
3. `set_filament`: select the filament and nozzle/syringe type
4. `set_process`: select the print quality
5. `slice`: start slicing in the background
6. `slice_status`: poll until `state` is `done`, `failed` or `canceled`

> `set_filament` and `set_process` depend on the selected printer. Send them again after every `set_printer`.

---

### `load_model`

Loads a model file and caches it. The `path` is used as the model's ID in later commands.

| param  | type   | required | description                  |
| ------ | ------ | -------- | ---------------------------- |
| `path` | string | yes      | absolute path to STL/3MF/OBJ |

Request:

```json
{
  "id": 1,
  "cmd": "load_model",
  "params": { "path": "/home/user/models/cube.stl" }
}
```

Response:

```json
{
  "id": 1,
  "code": "ok",
  "message": "Successfully loaded the model !",
  "data": [
    {
      "name": "cube.stl",
      "bbox": { "min": [-10, -10, 0], "max": [10, 10, 20] }
    }
  ]
}
```

Errors: `invalid_parameters` (missing path, file not found, unreadable file), `failed` (file contains no objects).

---

### `set_printer`

Selects a FELIX system printer preset.

| param         | type   | required | description                        |
| ------------- | ------ | -------- | ---------------------------------- |
| `printer_key` | string | yes      | see table below                    |
| `nozzle_size` | number | yes      | nozzle diameter in mm, e.g. `0.35` |

| printer_key         | model            | nozzles (mm)                 |
| ------------------- | ---------------- | ---------------------------- |
| `felix_pro_3`       | FELIX Pro 3      | 0.25, 0.35, 0.50, 0.70       |
| `felix_pro_l_3`     | FELIX Pro L 3.0  | 0.25, 0.35, 0.50, 0.70       |
| `felix_pro_xl_3`    | FELIX Pro XL 3.0 | 0.25, 0.35, 0.50, 0.70       |
| `felix_tec_4`       | FELIX Tec 4      | 0.25, 0.35, 0.50, 0.70       |
| `felix_food_single` | Food (single)    | 0.50, 1.00, 2.00, 3.00, 4.00 |
| `felix_food_double` | Food (double)    | 0.50, 1.00, 2.00, 3.00, 4.00 |
| `felix_food_switch` | Food (switch)    | 0.50, 1.00, 2.00, 3.00, 4.00 |

Request:

```json
{
  "id": 2,
  "cmd": "set_printer",
  "params": { "printer_key": "felix_pro_3", "nozzle_size": 0.35 }
}
```

Response:

```json
{
  "id": 2,
  "code": "ok",
  "message": "Successfully set the printer",
  "data": {
    "printer": "FELIX Pro 3 0.35mm nozzle",
    "nozzle_size": 0.35,
    "bed": [
      [0, 0],
      [237, 0],
      [237, 244],
      [0, 244]
    ],
    "printable_height": 235
  }
}
```

Errors: `invalid_parameters` (missing/empty key, nozzle ≤ 0), `failed` (unknown key, no matching system preset).

---

### `set_filament`

Selects a FELIX system filament preset. The nozzle size is taken from the selected printer, so `set_printer` must be called first.

| param          | type    | required | description                                                                     |
| -------------- | ------- | -------- | ------------------------------------------------------------------------------- |
| `filament_key` | string  | yes      | see tables below                                                                |
| `nozzle_key`   | string  | yes      | nozzle material (plastic) or syringe type (food)                                |
| `extruder`     | integer | no       | extruder index, default `0` (`1` for the second extruder on Food Double/Switch) |

**Plastic printers** (`felix_pro_3`, `felix_pro_l_3`, `felix_pro_xl_3`, `felix_tec_4`)

| filament_key                | filament                 |
| --------------------------- | ------------------------ |
| `felix_premium_pla`         | FELIX Premium PLA        |
| `felix_petg`                | FELIX PETG               |
| `felix_flex`                | FELIX FLEX               |
| `felix_wood`                | FELIX Wood               |
| `formfutura_epla`           | FORMFUTURA ePLA          |
| `formfutura_titanx_abs`     | FORMFUTURA TitanX ABS    |
| `formfutura_apollox_asa`    | FORMFUTURA AppoloX ASA   |
| `formfutura_aquasolve_pva`  | FORMFUTURA Aquasolve PVA |
| `ultrafuse_abs_fusion_plus` | Ultrafuse ABS Fusion+    |

| nozzle_key       | nozzles (mm)           |
| ---------------- | ---------------------- |
| `brass`          | 0.35, 0.50             |
| `hardened_steel` | 0.25, 0.35, 0.50, 0.70 |

**Food printers** (`felix_food_single`, `felix_food_double`, `felix_food_switch`)

| filament_key      | `plastic` syringe (mm)       | `stainless_steel` syringe (mm) |
| ----------------- | ---------------------------- | ------------------------------ |
| `food_very_thin`  | 0.50, 1.00, 2.00, 3.00, 4.00 | 0.50, 1.00, 2.00, 3.00, 4.00   |
| `food_thin`       | 0.50, 1.00, 2.00, 3.00, 4.00 | 0.50, 1.00, 2.00, 3.00, 4.00   |
| `food_medium`     | 0.50, 1.00, 2.00, 3.00, 4.00 | 0.50, 1.00, 2.00, 3.00, 4.00   |
| `food_thick`      | –                            | 0.50, 1.00, 2.00, 3.00, 4.00   |
| `food_very_thick` | –                            | 1.00, 2.00, 3.00, 4.00         |

Request:

```json
{
  "id": 3,
  "cmd": "set_filament",
  "params": {
    "filament_key": "felix_petg",
    "nozzle_key": "hardened_steel",
    "extruder": 0
  }
}
```

Response:

```json
{
  "id": 3,
  "code": "ok",
  "message": "Successfully set the filament",
  "data": {
    "filament": "FELIX PETG Hardened Steel @ 0.35mm",
    "extruder": 0,
    "nozzle_size": 0.35,
    "filaments": ["FELIX PETG Hardened Steel @ 0.35mm"]
  }
}
```

`filaments` lists the filament of every extruder slot.

Errors: `invalid_parameters` (missing/empty key, bad or out-of-range `extruder`), `failed` (no printer set, unknown key, food/plastic mismatch, no matching system preset, filament not compatible with the printer).

---

### `set_process`

Selects a FELIX system process (print quality) preset. The nozzle size is taken from the selected printer, so `set_printer` must be called first.

| param         | type   | required | description     |
| ------------- | ------ | -------- | --------------- |
| `process_key` | string | yes      | see table below |

| process_key       | food printers (mm) | plastic printers (mm)  |
| ----------------- | ------------------ | ---------------------- |
| `super_fine`      | –                  | 0.25, 0.35             |
| `very_fine`       | –                  | 0.25, 0.35, 0.50       |
| `fine`            | all                | 0.25, 0.35, 0.50, 0.70 |
| `normal`          | all                | 0.25, 0.35, 0.50, 0.70 |
| `slightly_coarse` | –                  | 0.35, 0.50, 0.70       |
| `coarse`          | all                | 0.35, 0.50, 0.70       |
| `very_coarse`     | –                  | 0.50, 0.70             |
| `super_coarse`    | –                  | 0.70                   |

Request:

```json
{
  "id": 4,
  "cmd": "set_process",
  "params": { "process_key": "normal" }
}
```

Response:

```json
{
  "id": 4,
  "code": "ok",
  "message": "Successfully set the process",
  "data": {
    "process": "FELIX Plastic Normal @ 0.35mm",
    "layer_height": 0.2
  }
}
```

`layer_height` comes from the selected profile. The value above is only an example.

Errors: `invalid_parameters` (missing/empty key), `failed` (no printer set, unknown key, quality not available for this printer type, no matching system preset, process not compatible with the printer).

---

### `slice`

Starts slicing a cached model in the background. Returns immediately; use `slice_status` to follow progress.

| param        | type   | required | description                                 |
| ------------ | ------ | -------- | ------------------------------------------- |
| `modelPath`  | string | yes      | same `path` that was passed to `load_model` |
| `outputPath` | string | yes      | where the G-code will be written            |

Request:

```json
{
  "id": 3,
  "cmd": "slice",
  "params": {
    "modelPath": "/home/user/models/cube.stl",
    "outputPath": "/tmp/cube.gcode"
  }
}
```

Response:

```json
{
  "id": 3,
  "code": "ok",
  "message": "Successfully started to slice the model",
  "data": {
    "status": "slicing...",
    "process": "FELIX Plastic Standard @ 0.35mm",
    "filament": "FELIX PLA Brass @ 0.35mm"
  }
}
```

Errors: `busy` (slice already running), `failed` (model not loaded, no printer set, validation error).

---

### `slice_status`

Returns the state of the current or last slice. Takes no parameters.

Request:

```json
{ "id": 4, "cmd": "slice_status", "params": {} }
```

Response while slicing:

```json
{
  "id": 4,
  "code": "ok",
  "message": "Status check finished !",
  "data": { "state": "slicing", "percent": 45, "text": "Generating perimeters" }
}
```

Response when finished:

```json
{
  "id": 5,
  "code": "ok",
  "message": "Status check finished !",
  "data": {
    "state": "done",
    "percent": 100,
    "text": "",
    "gcode_path": "/tmp/cube.gcode"
  }
}
```

Response on failure:

```json
{
  "id": 6,
  "code": "ok",
  "message": "Status check finished !",
  "data": { "state": "failed", "percent": 30, "text": "...", "error": "..." }
}
```

`state` is one of: `idle`, `slicing`, `done`, `failed`, `canceled`.

### `get_state`

Retrieves information about the current state of the orca.

Takes no parameters

Request:

```json
{ "id": 4, "cmd": "get_state", "params": {} }
```

Response:

```json
{
  "code": 200,
  "data": {
    "filaments": [
      "FELIX PETG Hardened Steel @ 0.35mm",
      ""
    ],
    "models": [
      "/home/simon/Desktop/FelixPrinter/FELIX-Filo-Slicer-deamon/src/daemon/client/assets/dino.stl"
    ],
    "num_extruders": 2,
    "printer": "FELIX Pro 3 0.35mm nozzle",
    "printer_key": "felix_pro_3",
    "process": "FELIX Plastic Normal @ 0.35mm"
  },
  "id": 6
}```

## Building

Only the daemon is built, not the full OrcaSlicer application. This takes two steps: build Orca's dependencies once, then build the `orca-daemon` target with the CMake preset.

### Requirements

- CMake 3.25 or newer (presets version 8 needs CMake 3.28+)
- Ninja
- A C++17 compiler: GCC/Clang on Linux, Xcode command line tools on macOS, Visual Studio 2022 on Windows

### 1. Build the dependencies

Dependencies only need to be built once, or again when `deps/` changes. They are placed in `deps/build`, which is where the preset expects them.

**Linux**

```bash
./build_linux.sh -u   # install system packages (needs sudo, first time only)
./build_linux.sh -d   # build dependencies only
```

**macOS**

```bash
./build_release_macos.sh -d -a arm64   # use -a x86_64 on Intel Macs
```

**Windows** (from _x64 Native Tools Command Prompt for VS 2022_)

```bat
build_release_vs2022.bat deps
```

> On Windows, the dependencies must be built in the same configuration as the daemon (Debug or Release), because MSVC cannot mix the two.

### 2. Build the daemon

From the repository root:

```bash
cmake --preset orca-daemon-build
cmake --build --preset orca-daemon-build
```

This configures with `SLIC3R_GUI=OFF`, `SLIC3R_CAD=OFF` and `ORCA_DAEMON=ON`, and builds only the `orca-daemon` target into `out/build/orca-daemon-build/`.

On Windows, run these commands from the _x64 Native Tools Command Prompt_ so Ninja finds the MSVC compiler.
