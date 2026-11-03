// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

/**
  ====================================================================================
  data_interpreter.hpp
  ====================================================================================

  interprets the message that the client sent to the orca
*/

#pragma once

#include <json_fwd.hpp>
#include <string>
#include "connection_handler.hpp"

using json    = nlohmann::json;
using command = std::function<json(int client_fd, const json& params)>;

namespace felix_daemon {
class data_interpreter
{
public:
    data_interpreter();

    next_action operator()(int client_fd);

private:
    bool wasReadSuccessful(size_t n);
    bool handleMessage(int client_fd, const std::string& msg);
    void send_all(int clinet_fd, const std::string& msg);

    void process_request(int clinet_fd, const std::string& msg);

    server_response error(int id, const std::string& what);

private:
    std::unordered_map<std::string, command> m_commands;
};
} // namespace felix_daemon