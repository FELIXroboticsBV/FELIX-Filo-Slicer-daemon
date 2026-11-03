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
#include <boost/log/trivial.hpp>
#include <detail/exceptions.hpp>
#include <exception>
#include <libslic3r/Model.hpp>
#include <libslic3r/PresetBundle.hpp>
#include <stdexcept>
#include "utils/build_printer_preset_string.hpp"

namespace felix_daemon {

/*************************************************************************************
    PARAMETERS STRUCTS DEFINITIONS
**************************************************************************************/
struct cmdParameters
{
    virtual void parse(const json& parameters) = 0;
};

struct cmdLoadModel_Parameters : public cmdParameters
{
    boost::filesystem::path path;

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

struct cmdSetPrinter_Parameters : public cmdParameters
{
    std::string printer_key;
    float nozzle_size = 0.f;

    void parse(const json& parameters) override
    {
        auto key = parameters.find("printer_key");
        if (key == parameters.end() || !key->is_string() || key->get<std::string>().empty())
            throw std::invalid_argument("missing or invalid 'printer_key'");
        printer_key = key->get<std::string>();

        auto size = parameters.find("nozzle_size");
        if (size == parameters.end() || !size->is_number())
            throw std::invalid_argument("missing or invalid 'nozzle_size'");
        nozzle_size = size->get<float>();

        if (nozzle_size <= 0.f)
            throw std::invalid_argument("'nozzle_size' must be greater than 0");
    }
};

/*************************************************************************************
    COMMANDS DEFINITIONS
**************************************************************************************/

orca_session::orca_session()
{
    auto result = m_presetBundle->load_system_presets_from_json(Slic3r::ForwardCompatibilitySubstitutionRule::EnableSilent);
    if (result.second != "") {
        BOOST_LOG_TRIVIAL(error) << result.second << "\n \t data_dir: " << Slic3r::data_dir();
        throw std::runtime_error("failed to start the deamon !");
    }
}

orca_session::~orca_session() = default;

orca_response orca_session::cmd_load_model(const nlohmann::json& parameters)
{
    // parse the parameters
    try {
        cmdLoadModel_Parameters ctx;
        ctx.parse(parameters);

        BOOST_LOG_TRIVIAL(info) << "Loading the model \n \t path: ", ctx.path.string();

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

orca_response orca_session::cmd_set_printer(const nlohmann::json& parameters)
{
    try {
        // parse the arguments
        cmdSetPrinter_Parameters p;
        p.parse(parameters);

        // builds the priset name so that orca can find it and app can just supply simplified keys
        const std::string presetName = felix_daemon::utils::build_printer_preset_string(p.printer_key, p.nozzle_size);
        if (presetName.empty())
            return orca_error_response(orca_response_code::failed, "unknown printer key: " + p.printer_key);

        // get the orca preset from the name
        const Slic3r::Preset* preset = m_presetBundle->printers.find_preset(presetName, false);

        if (!preset || !preset->is_system)
            return orca_error_response(orca_response_code::failed, "orca system profile not found: " + presetName);

        m_presetBundle->printers.select_preset_by_name(presetName, true);
        m_presetBundle->update_compatible(Slic3r::PresetSelectCompatibleType::Always);

        const auto& cfg    = preset->config;
        nlohmann::json bed = nlohmann::json::array();
        if (auto* area = cfg.option<Slic3r::ConfigOptionPoints>("printable_area"))
            for (const auto& p : area->values)
                bed.push_back({p.x(), p.y()});

        json res_body;
        res_body["bed"]              = bed;
        res_body["printer"]          = p.printer_key;
        res_body["nozzle_size"]      = p.nozzle_size;
        res_body["nozzle_size"]      = p.nozzle_size;
        res_body["printable_height"] = cfg.opt_float("printable_height");

        return orca_success_response("Successfully set the printer", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

} // namespace felix_daemon
