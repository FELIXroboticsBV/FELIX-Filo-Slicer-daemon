// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

/*
  ====================================================================================
  orca_session.hpp
  ====================================================================================

  Class that handles the orca side of things, creates context and caches data
*/

#pragma once

#include "orca_response.hpp"
#include <json_fwd.hpp>

namespace Slic3r {
class Model;
}

namespace felix_daemon {

class orca_session
{
public:
    orca_session();

    /**
     * Loads the model into the orca
     * @param  {nlohmann::json} parameters : data received from the request
     * @return {nlohmann::json}            : body of the response
     */
    orca_response cmd_load_model(const nlohmann::json& parameters);

    ~orca_session();

private:
    std::unordered_map<std::string, std::unique_ptr<Slic3r::Model>> m_meshCache;
};

} // namespace felix_daemon
