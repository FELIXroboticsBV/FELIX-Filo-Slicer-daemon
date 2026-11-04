// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

namespace Slic3r {
class PresetBundle;
class DynamicPrintConfig;
} // namespace Slic3r

namespace felix_daemon::utils {
/**
 * Rebuilds the "filament_self_index" list so it matches "filament_extruder_variant".
 *
 * On multi-extruder printers, each filament has one entry per extruder variant in
 * "filament_extruder_variant". "filament_self_index" must have the same length and
 * tells, for each of those entries, which filament (1-based) it belongs to.
 *
 * The GUI keeps these lists in sync. However in headless, they can get out of sync and Orca
 * crashes reading past the end. This fills the list from the selected filament
 * presets, or marks every entry as filament 1 if the sizes still don't match.
 */
void sync_filament_self_index(const Slic3r::PresetBundle& bundle, Slic3r::DynamicPrintConfig& config);
} // namespace felix_daemon::utils