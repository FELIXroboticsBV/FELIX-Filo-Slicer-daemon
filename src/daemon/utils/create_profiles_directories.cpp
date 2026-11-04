// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include "create_profiles_directories.hpp"
#include <libslic3r/Utils.hpp>

namespace felix_daemon::utils {
void copy_directories(const fs::path& src, const fs::path& dst)
{
    fs::create_directories(dst);
    for (const auto& entry : fs::recursive_directory_iterator(src)) {
        const fs::path target = dst / fs::relative(entry.path(), src);
        if (fs::is_directory(entry.path()))
            fs::create_directories(target);
        else
            fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing);
    }
}

// copies <resources>/profiles/<vendor>.json and <vendor>/ into <data_dir>/system
void sync_system_profiles(const std::vector<std::string>& vendors)
{
    const fs::path src = fs::path(Slic3r::resources_dir()) / "profiles";
    const fs::path dst = fs::path(Slic3r::data_dir()) / "system";
    fs::create_directories(dst);

    for (const auto& vendor : vendors) {
        const fs::path index = src / (vendor + ".json");
        if (!fs::exists(index))
            throw std::runtime_error("profile index not found: " + index.string());

        fs::copy_file(index, dst / index.filename(), fs::copy_options::overwrite_existing);
        if (fs::exists(src / vendor))
            copy_directories(src / vendor, dst / vendor);
    }
}
} // namespace felix_daemon::utils