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
#include "cstdint"

namespace felix_daemon::utils {

struct b_daemon_start_flags
{
    bool allow_cache_dump;

    int to_int()
    {
        int result = 0;
        result |= allow_cache_dump ? daemon_start_flag_bits::allow_request_cache_dump : 0;
        return result;
    }
} tmp_flags;

CLI::App* set_up_cli(char** argv)
{
    static CLI::App
        app("Daemon application for leveraging orca`s slicing capabilities without being limited to their UI and rendering code",
            "FELIX Slicing daemon");

    int allow_request_cache_dump = 0;

    app.add_option("--socket-file, --sf", cfg.socketName, "path to the local unix socket used for communication");
    app.add_flag("--allow-request-cache-dump, --arcp", tmp_flags.allow_cache_dump, "allows to use `dump_request_cache` command");
    argv = app.ensure_utf8(argv);
    return &app;
}

int override_config(CLI::App* app, int argc, char** argv)
{
    try {
        app->parse(argc, argv);
        flags = tmp_flags.to_int();
    } catch (const CLI::CallForHelp& e) {
        app->exit(e);
        return 1;
    } catch (const CLI::CallForAllHelp& e) {
        app->exit(e);
        return 1;
    } catch (const CLI::ParseError& e) {
        return app->exit(e);
    }
    return 0;
}
} // namespace felix_daemon::utils
