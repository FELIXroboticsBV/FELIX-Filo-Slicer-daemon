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

struct FilamentEntry
{
    const char* name;
    bool food;
};
struct NozzleEntry
{
    const char* name;
    bool food;
};

static const std::unordered_map<std::string, FilamentEntry> k_filaments = {
    // Food (paste consistency)
    {"food_very_thin", {"Food Very Thin", true}},
    {"food_thin", {"Food Thin", true}},
    {"food_medium", {"Food Medium", true}},
    {"food_thick", {"Food Thick", true}},
    {"food_very_thick", {"Food Very Thick", true}},
    // Plastic
    {"felix_premium_pla", {"FELIX Premium PLA", false}},
    {"felix_petg", {"FELIX PETG", false}},
    {"felix_flex", {"FELIX FLEX", false}},
    {"felix_wood", {"FELIX Wood", false}},
    {"formfutura_epla", {"FORMFUTURA ePLA", false}},
    {"formfutura_titanx_abs", {"FORMFUTURA TitanX ABS", false}},
    {"formfutura_apollox_asa", {"FORMFUTURA AppoloX ASA", false}},
    {"formfutura_aquasolve_pva", {"FORMFUTURA Aquasolve PVA", false}},
    {"ultrafuse_abs_fusion_plus", {"Ultrafuse ABS Fusion+", false}},
};

static const std::unordered_map<std::string, NozzleEntry> k_nozzles = {
    // Food: syringe type
    {"plastic", {"Plastic Syringe", true}},
    {"stainless_steel", {"Stainless Steel Syringe", true}},
    // Plastic: nozzle material
    {"brass", {"Brass", false}},
    {"hardened_steel", {"Hardened Steel", false}},
};

// Clean name -> actual name in the vendor profile (double spaces)
static const std::unordered_map<std::string, std::string> k_filamentNameOverrides = {
    {"Food Very Thin Stainless Steel Syringe @1.00mm", "Food Very Thin Stainless Steel  Syringe  @1.00mm"},
    {"Food Very Thin Stainless Steel Syringe @2.00mm", "Food Very Thin Stainless Steel  Syringe  @2.00mm"},
    {"Food Very Thin Stainless Steel Syringe @3.00mm", "Food Very Thin Stainless Steel  Syringe  @3.00mm"},
    {"Food Very Thin Stainless Steel Syringe @4.00mm", "Food Very Thin Stainless Steel  Syringe  @4.00mm"},
    {"FELIX FLEX Brass @ 0.35mm", "FELIX FLEX  Brass @ 0.35mm"},
    {"FELIX FLEX Brass @ 0.50mm", "FELIX FLEX  Brass @ 0.50mm"},
    {"FELIX Wood Brass @ 0.35mm", "FELIX Wood  Brass @ 0.35mm"},
};

static bool is_food_printer(const std::string& printer_key) { return printer_key.rfind("felix_food_", 0) == 0; }

std::string build_filament_preset_string(const std::string& printer_key,
                                         const std::string& filament_key,
                                         const std::string& nozzle_key,
                                         float nozzle_size)
{
    // printer model does not exists
    if (k_printerModels.find(printer_key) == k_printerModels.end())
        return {};

    auto fil = k_filaments.find(filament_key);
    auto noz = k_nozzles.find(nozzle_key);
    // filament or nozzle does not exist
    if (fil == k_filaments.end() || noz == k_nozzles.end())
        return {};

    const bool food = is_food_printer(printer_key);
    // filament is not suitable for the food printer
    if (fil->second.food != food || noz->second.food != food)
        return {};

    char size[16];
    std::snprintf(size, sizeof(size), "%.2f", nozzle_size);

    // Food: "Food Thin Plastic Syringe @0.50mm"   (no space after @)
    // Plastic: "FELIX PETG Brass @ 0.35mm"        (space after @)
    // this corrects it
    std::string name = std::string(fil->second.name) + " " + noz->second.name + (food ? " @" : " @ ") + size + "mm";

    auto result = k_filamentNameOverrides.find(name);
    return result != k_filamentNameOverrides.end() ? result->second : name;
}

struct QualityEntry
{
    const char* name;
    bool food;
    bool plastic;
};

static const std::unordered_map<std::string, QualityEntry> k_qualities = {
    {"super_fine", {"Super Fine", false, true}},
    {"very_fine", {"Very Fine", false, true}},
    {"fine", {"Fine", true, true}},
    {"normal", {"Normal", true, true}},
    {"slightly_coarse", {"Slightly Coarse", false, true}},
    {"coarse", {"Coarse", true, true}},
    {"very_coarse", {"Very Coarse", false, true}},
    {"super_coarse", {"Super Coarse", false, true}},
};

std::string build_proccess_preset_string(const std::string& printer_key, const std::string& quality_key, float nozzle_size)
{
    // printer model does not exist
    if (k_printerModels.find(printer_key) == k_printerModels.end())
        return {};

    auto q = k_qualities.find(quality_key);
    // quality key is invalid
    if (q == k_qualities.end())
        return {};

    // the quality profile is not suited for the food printer
    const bool food = is_food_printer(printer_key);
    if (food ? !q->second.food : !q->second.plastic)
        return {};

    char size[16];
    std::snprintf(size, sizeof(size), "%.2f", nozzle_size);

    // "FELIX Food Coarse @ 0.50mm" / "FELIX Plastic Super Fine @ 0.25mm"
    return std::string("FELIX ") + (food ? "Food" : "Plastic") + " " + q->second.name + " @ " + size + "mm";
}

} // namespace felix_daemon::utils