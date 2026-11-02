// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include <iostream>
#include "libslic3r/Utils.hpp"

int main()
{
    // configure the logger
    Slic3r::set_data_dir(Slic3r::data_dir()); // must come first
    Slic3r::set_log_path_and_level("orca-daemon.log", 3);
    return 0;
}