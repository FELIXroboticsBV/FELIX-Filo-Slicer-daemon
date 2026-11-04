// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include "build_printer_preset_string.hpp"
#include <unordered_map>

namespace felix_daemon::utils {

static const std::unordered_map<std::string, std::string> k_printerModels = {
    {"felix_food_single", "FELIX Food Single"},
    {"felix_food_double", "FELIX Food Double"},
    {"felix_food_switch", "FELIX Food Switch"},
    {"felix_pro_3", "FELIX Pro 3"},
    {"felix_pro_l_3", "FELIX Pro L 3.0"},
    {"felix_pro_xl_3", "FELIX Pro XL 3.0"},
    {"felix_tec_4", "FELIX Tec 4"},
};

std::string build_printer_preset_string(const std::string& printer_key, float nozzle_size)
{
    auto it = k_printerModels.find(printer_key);
    if (it == k_printerModels.end())
        return {};

    char size[16];
    std::snprintf(size, sizeof(size), "%.2f", nozzle_size);

    return it->second + " " + size + "mm nozzle";
}
} // namespace felix_daemon::utils