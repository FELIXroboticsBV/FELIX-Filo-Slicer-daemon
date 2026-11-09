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
 * @param  {float} nozzle_size        : nozzle size in mm
 * @return {std::string}              : orca compatible name for the printer
 */
std::string build_printer_preset_string(const std::string& printer_key, float nozzle_size);

/**
 * Provides functionality to build FELIX filament preset from specified parameters
 * @param  {std::string} printer_key  : key of the printer that is currently in use
 * @param  {std::string} filament_key : key of the filament that should be set
 * @param  {std::string} nozzle_key   : what kind of nozzle is going to be used for printing
 * @param  {float} nozzle_size        : nozzle size in mm
 * @return {std::string}              : string that can be used in orca to set the filament
 */
std::string build_filament_preset_string(const std::string& printer_key,
                                         const std::string& filament_key,
                                         const std::string& nozzle_key,
                                         float nozzle_size);

/**
 * Provides functionlity to build FELIX process(quality) preset from specified parameters
 * @param  {std::string} printer_key : key of the printer that is currently in use
 * @param  {std::string} quality_key : key for the quality preset
 * @param  {float} nozzle_size       : nozzle size in mm
 * @return {std::string}             : string that can be used in orca to set the process preset
 */
std::string build_proccess_preset_string(const std::string& printer_key, const std::string& quality_key, float nozzle_size);
} // namespace felix_daemon::utils