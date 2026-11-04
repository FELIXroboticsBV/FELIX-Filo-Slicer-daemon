// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include <libslic3r/Preset.hpp>
#include <libslic3r/PresetBundle.hpp>
#include <vector>
namespace felix_daemon::utils {
void sync_filament_self_index(const Slic3r::PresetBundle& bundle, Slic3r::DynamicPrintConfig& config)
{
    auto* variants = config.option<Slic3r::ConfigOptionStrings>("filament_extruder_variant");
    if (!variants)
        return;

    // each filament contributes as many entries as it has variants, all tagged with its 1-based index
    std::vector<int> selfIndex;
    for (size_t i = 0; i < bundle.filament_presets.size(); ++i) {
        size_t count = 1;
        if (const Slic3r::Preset* f = bundle.filaments.find_preset(bundle.filament_presets[i], false))
            if (auto* v = f->config.option<Slic3r::ConfigOptionStrings>("filament_extruder_variant"))
                count = std::max<size_t>(1, v->values.size());
        selfIndex.insert(selfIndex.end(), count, static_cast<int>(i + 1));
    }

    // fallback: sizes still disagree -> treat everything as filament 1 (fine for single-filament tests)
    if (selfIndex.size() != variants->values.size())
        selfIndex.assign(variants->values.size(), 1);

    config.option<Slic3r::ConfigOptionInts>("filament_self_index", true)->values = selfIndex;
}

} // namespace felix_daemon::utils