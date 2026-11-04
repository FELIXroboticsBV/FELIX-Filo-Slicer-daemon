// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

/**
  ====================================================================================
  orca_response.hpp
  ====================================================================================

  data which is returned by the orca_session.hpp
*/
#pragma once

#include <json.hpp>
#include <optional>

enum class orca_response_code { ok, failed, not_found, invalid_parameters, busy };

struct orca_response
{
    orca_response_code code;
    bool failed;
    std::string message;
    std::optional<nlohmann::json> json;
};

inline orca_response orca_error_response(orca_response_code code, const std::string& msg) { return {code, true, msg, std::nullopt}; }
inline orca_response orca_success_response(const std::string& msg, nlohmann::json& body)
{
    return {orca_response_code::ok, false, msg, body};
}

using json = nlohmann::json;

// callback for the orca function
using orca_cmd = std::function<orca_response(const json& params)>;