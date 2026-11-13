// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include "parse_cli.hpp"
#include "config.hpp"
#include "deps/CLI11.hpp"
#include <boost/log/trivial.hpp>
#include <exception>

namespace felix_daemon::utils {
CLI::App* set_up_cli(char** argv)
{
    static CLI::App
        app("Daemon application for leveraging orca`s slicing capabilities without being limited to their UI and rendering code",
            "FELIX Slicing daemon");

    app.add_option("--socket-file, --sf", cfg.socketName, "path to the local unix socket used for communication");
    app.add_option("--test, --test", cfg.socketName, "path to the local unix socket used for communication");

    argv = app.ensure_utf8(argv);
    return &app;
}

int override_config(CLI::App* app, int argc, char** argv)
{
    CLI11_PARSE(*app, argc, argv);
    return 0;
}
} // namespace felix_daemon::utils
