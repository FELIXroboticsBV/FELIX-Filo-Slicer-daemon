// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#pragma once

#include "socket.hpp"
#include <functional>
#include <json.hpp>

namespace felix_daemon {

enum class next_action { keep_running, shutdown };

enum class response_code {
    ok           = 200,
    user_error   = 400,
    not_found    = 404,
    busy         = 409,
    server_error = 500,
};

struct server_response
{
    int id                 = 0;
    next_action nextAction = next_action::keep_running;
    response_code code;
    nlohmann::json body = nlohmann::json::object();
    std::string error;

    std::string stringify() const
    {
        nlohmann::json j = {{"id", id}, {"code", static_cast<int>(code)}};
        if (error.empty())
            j["data"] = body;
        else
            j["error"] = error;
        return j.dump() + "\n";
    }
};

/**
    Definition of the callback function that creates and sends response
*/
using request_trigger = std::function<next_action(client& socket)>;
} // namespace felix_daemon