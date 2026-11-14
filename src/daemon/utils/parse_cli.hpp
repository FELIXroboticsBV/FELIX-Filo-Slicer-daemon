// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include "deps/CLI11.hpp"
namespace felix_daemon::utils {
CLI::App* set_up_cli(char** argv);

int override_config(CLI::App* app, int argc, char** argv);
} // namespace felix_daemon::utils