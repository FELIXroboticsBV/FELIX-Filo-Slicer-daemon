// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include <iostream>
#include "config.hpp"
#include "router.hpp"
#include "libslic3r/Utils.hpp"
#include <boost/log/utility/setup/console.hpp>
#include "orca_session.hpp"
#include "server.hpp"
#include "paths.hpp"
#include "utils/create_profiles_directories.hpp"

int main(int argc, char** argv)
{
    // configure the logger
    auto logDir = felix_daemon::default_data_directory();
    Slic3r::set_data_dir(logDir.string());
    Slic3r::set_log_path_and_level("orca-daemon.log", 3);
    Slic3r::set_resources_dir(felix_daemon::resources_directory().string());
    boost::log::add_console_log(std::clog, boost::log::keywords::auto_flush = true);

    // pulls the correct vendors
    felix_daemon::utils::sync_system_profiles({"FELIX Printers"});

    // load the config
    felix_daemon::cfg.load(felix_daemon::config_path().string());

    // parse the command line arguments and override config

    // create orca session
    felix_daemon::orca_session orca_session;

    // data transfer
    felix_daemon::router handler(orca_session);

    // create server
    felix_daemon::server server([&handler](int client_fd) { return handler(client_fd); });
    server.start();

    return 0;
}