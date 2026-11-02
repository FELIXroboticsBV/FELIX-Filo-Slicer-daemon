// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include <iostream>
#include "config.hpp"
#include "libslic3r/Utils.hpp"
#include <boost/log/utility/setup/console.hpp>
#include "server.hpp"
#include "paths.hpp"

int main()
{
    // configure the logger
    auto logDir = felix_daemon::default_data_directory();
    Slic3r::set_data_dir(logDir.string());
    Slic3r::set_log_path_and_level("orca-daemon.log", 3);
    boost::log::add_console_log(std::clog, boost::log::keywords::auto_flush = true);

    // load the config
    felix_daemon::cfg.load(felix_daemon::config_path().string());

    // create server
    felix_daemon::server server;

    return 0;
}