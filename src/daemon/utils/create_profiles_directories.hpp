// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace felix_daemon::utils {

/**
 * Puts all the vendors where required
 * USAGE: sync_system_profiles({"OrcaFilamentLibrary", "FELIX Printers""});
 * @param  {std::vector<std::string>} vendors :
 */
void sync_system_profiles(const std::vector<std::string>& vendors);

} // namespace felix_daemon::utils