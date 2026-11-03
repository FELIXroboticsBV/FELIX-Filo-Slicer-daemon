// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#pragma once

#include <functional>
#include <json.hpp>

namespace felix_daemon {

enum class next_action { KeepRuning, Shutdown };

enum class response_code {
    OK           = 200,
    NOT_FOUND    = 404,
    USER_ERROR   = 400,
    SERVER_ERROR = 500

};

struct server_response
{
    int id                 = 0;
    next_action nextAction = next_action::KeepRuning;
    response_code code;
    nlohmann::json data = nlohmann::json::object();
    std::string error;

    std::string stringify() const
    {
        nlohmann::json j = {{"id", id}, {"code", static_cast<int>(code)}};
        if (error.empty())
            j["data"] = data;
        else
            j["error"] = error;
        return j.dump() + "\n";
    }
};
using connection_handler = std::function<next_action(int client_fd)>;
} // namespace felix_daemon