// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include <string>
namespace felix_daemon::utils {

/**
 * Provides functionality to build FELIX profiles in orca from the specific parameters
 * @param  {std::string} printer_key  : printer key as defined in the map in the c++ file
 * @param  {float} nozzle_size        : nozzle size in milometers
 * @return {std::string}              : orca compatible name for the printer
 */
std::string build_printer_preset_string(const std::string& printer_key, float nozzle_size);
} // namespace felix_daemon::utils