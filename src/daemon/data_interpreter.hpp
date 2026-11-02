// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#pragma once

#include <string>
#include "connection_handler.hpp"

namespace felix_daemon {
class data_interpreter
{
public:
    HandlerResult operator()(int client_fd);

private:
    bool handleMessage(int client_fd, const std::string& msg);
};
} // namespace felix_daemon