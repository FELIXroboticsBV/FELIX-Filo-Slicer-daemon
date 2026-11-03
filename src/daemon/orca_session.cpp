// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include "orca_session.hpp"
#include "orca_response.hpp"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <detail/exceptions.hpp>
#include <exception>
#include <libslic3r/Model.hpp>
#include <stdexcept>

namespace felix_daemon {

/*************************************************************************************
    PARAMETERS STRUCTS DEFINITIONS
**************************************************************************************/
struct cmdParameters
{
    cmdParameters() = default;

    virtual void parse(const json& parameters) = 0;

    virtual ~cmdParameters() = default;
};

struct cmdLoadModel_Parameters : public cmdParameters
{
    boost::filesystem::path path;

    virtual ~cmdLoadModel_Parameters() = default;

    void parse(const json& parameters) override
    {
        auto it = parameters.find("path");

        if (it == parameters.end() || !it->is_string())
            throw std::invalid_argument("missing or invalid 'path'");
        path = it->get<std::string>();

        if (!boost::filesystem::exists(path)) {
            throw std::invalid_argument("model was not found ! ");
        }
    }
};

/*************************************************************************************
    COMMANDS DEFINITIONS
**************************************************************************************/

orca_session::orca_session() = default;

orca_session::~orca_session() = default;

orca_response orca_session::cmd_load_model(const nlohmann::json& parameters)
{
    // parse the parameters
    try {
        cmdLoadModel_Parameters ctx;
        ctx.parse(parameters);

        auto model = std::make_unique<Slic3r::Model>(Slic3r::Model::read_from_file(ctx.path.string()));
        if (model->objects.empty())
            return orca_error_response(orca_response_code::failed, "file contains no objects: " + ctx.path.string());

        nlohmann::json res_body = nlohmann::json::array();
        for (const Slic3r::ModelObject* obj : model->objects) {
            const Slic3r::BoundingBoxf3 bb = obj->raw_mesh_bounding_box(); 
            res_body.push_back({{"name", obj->name},
                                {"bbox", {{"min", {bb.min.x(), bb.min.y(), bb.min.z()}}, {"max", {bb.max.x(), bb.max.y(), bb.max.z()}}}}});
        }
        m_meshCache[ctx.path.string()] = std::move(model);

        return orca_success_response("Successfully loaded hte model !", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

} // namespace felix_daemon
