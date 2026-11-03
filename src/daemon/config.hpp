// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#pragma once

#include <string>
namespace felix_daemon {

struct daemon_config
{
    std::string socketName      = "felix-daemon";
    int max_pending_connections = 5;
    void load(const std::string& filename);
    void save(const std::string& filename);

private:
    bool loaded = false;
};

/**
    Global variable that contains parsed information of the config.ini
*/
inline daemon_config cfg;
} // namespace felix_daemon