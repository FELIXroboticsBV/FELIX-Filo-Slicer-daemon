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
#include <iterator>
#include <libslic3r/Model.hpp>
#include <libslic3r/PresetBundle.hpp>
#include <libslic3r/Print.hpp>
#include <libslic3r/PrintBase.hpp>
#include <mutex>
#include <stdexcept>
#include "utils/build_printer_preset_string.hpp"
#include "utils/sync_filament_self_index.hpp"
#include "orca_request_parameters.hpp"
namespace felix_daemon {

//=========================================================================================
//-----------------------------------------------------------------------------------------
// Class helpers
//-----------------------------------------------------------------------------------------
//=========================================================================================

orca_session::orca_session() : m_presetBundle(std::make_unique<Slic3r::PresetBundle>()), m_print(std::make_unique<Slic3r::Print>())
{
    auto result = m_presetBundle->load_system_presets_from_json(Slic3r::ForwardCompatibilitySubstitutionRule::EnableSilent);
    if (result.second != "") {
        BOOST_LOG_TRIVIAL(error) << result.second << "\n \t data_dir: " << Slic3r::data_dir();
        throw std::runtime_error("failed to start the deamon !");
    }

    // set up the progress call back
    m_print->set_status_callback([this](const Slic3r::PrintBase::SlicingStatus& status) {
        std::lock_guard<std::mutex> lock(m_slicingMutex);
        if (status.percent >= 0) {
            m_progress.percentage = status.percent;
        }
        if (!status.text.empty()) {
            m_progress.message = status.text;
        }
        BOOST_LOG_TRIVIAL(info) << "Slicing progress \n \t percentage: " << m_progress.percentage << "\n \t msg: " << m_progress.message;
    });

    m_plate = std::make_unique<Slic3r::Model>();
}

void orca_session::set_state(orca_state new_state)
{
    std::lock_guard<std::mutex> lock(m_slicingMutex);
    if (new_state == orca_state::slicing) {
        // if starting to slicing, reset the slicing stat
        m_progress = {};
    }
    m_progress.state = new_state;
}

Slic3r::ModelObject* orca_session::find_model(const std::string& model_key)
{
    auto it = m_models.find(model_key);
    return it == m_models.end() ? nullptr : it->second;
}

const char* to_string(orca_state s)
{
    switch (s) {
    case orca_state::idle: return "idle";
    case orca_state::slicing: return "slicing";
    case orca_state::done: return "done";
    case orca_state::failed: return "failed";
    case orca_state::canceled: return "canceled";
    }
    return "unknown";
}

bool orca_session::is_slicing() const
{
    std::lock_guard<std::mutex> lock(m_slicingMutex);
    return m_progress.state == orca_state::slicing;
}

orca_session::~orca_session() = default;

//=========================================================================================
//-----------------------------------------------------------------------------------------
// Commands
//-----------------------------------------------------------------------------------------
//=========================================================================================

/*******************************************************************************
Load model command
********************************************************************************/
orca_response orca_session::cmd_load_model(const nlohmann::json& parameters)
{
    // parse the parameters
    try {
        Cmd_Load_Model_Parameters ctx;
        ctx.parse(parameters);

        BOOST_LOG_TRIVIAL(info) << "Loading the model \n \t path: " << ctx.path.string();

        auto loaded = Slic3r::Model::read_from_file(ctx.path.string());
        if (loaded.objects.empty())
            return orca_error_response(orca_response_code::failed, "file contains no objects: " + ctx.path.string());

        std::vector<std::string> ids;
        for (size_t i = 0; i < loaded.objects.size(); ++i) {
            std::string id = loaded.objects.size() == 1 ? ctx.path.string() : ctx.path.string() + ":" + std::to_string(i);
            ids.push_back(std::move(id));
        }

        nlohmann::json res_body = nlohmann::json::array();
        for (size_t i = 0; i < loaded.objects.size(); ++i) {
            Slic3r::ModelObject* obj = m_plate->add_object(*loaded.objects[i]); // deep copy into the plate
            if (obj->instances.empty())
                obj->add_instance();

            m_models[ids[i]] = obj;

            const Slic3r::BoundingBoxf3 bb = obj->raw_mesh_bounding_box();
            res_body.push_back({{"model_key", ids[i]},
                                {"name", obj->name},
                                {"bbox", {{"min", {bb.min.x(), bb.min.y(), bb.min.z()}}, {"max", {bb.max.x(), bb.max.y(), bb.max.z()}}}}});
        }
        return orca_success_response("Successfully loaded the model", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

orca_response orca_session::cmd_instantiate_object(const nlohmann::json& parameters)
{
    try {
        Cmd_Instantiate_Model_Parameters params;
        params.parse(parameters);

        Slic3r::ModelObject* obj = find_model(params.model_key);
        if (!obj)
            return orca_error_response(orca_response_code::invalid_parameters, "unknown model_key: " + params.model_key);

        if (params.transform.has_value()) {
            auto inst = obj->add_instance();
            inst->set_transformation(Slic3r::Geometry::Transformation(params.transform.value()));
        } else {
            obj->add_instance(*obj->instances.front());
        }

        obj->invalidate_bounding_box();

        const size_t index = obj->instances.size() - 1;
        const auto bb      = obj->instance_bounding_box(index);
        const auto pos     = obj->instances[index]->get_offset();

        json res_body;
        res_body["model_key"] = params.model_key;
        res_body["instance"]  = index;
        res_body["count"]     = obj->instances.size();
        res_body["position"]  = {pos.x(), pos.y(), pos.z()};
        res_body["bbox_min"]  = {bb.min.x(), bb.min.y(), bb.min.z()};
        res_body["bbox_max"]  = {bb.max.x(), bb.max.y(), bb.max.z()};

        return orca_success_response("Successfully added an instance", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

/*******************************************************************************
Set printer command
********************************************************************************/
orca_response orca_session::cmd_set_printer(const nlohmann::json& parameters)
{
    try {
        // parse the arguments
        Cmd_Set_Printer_Parameters p;
        p.parse(parameters);

        // builds the priset name so that orca can find it and app can just supply simplified keys
        const std::string presetName = felix_daemon::utils::build_printer_preset_string(p.printer_key, p.nozzle_size);
        if (presetName.empty())
            return orca_error_response(orca_response_code::failed, "unknown printer key: " + p.printer_key);

        // get the orca preset from the name
        const Slic3r::Preset* preset = m_presetBundle->printers.find_preset(presetName, false);

        if (!preset)
            return orca_error_response(orca_response_code::failed, "orca system profile not found: " + presetName);

        // selects the printer
        m_presetBundle->printers.select_preset_by_name(presetName, true);
        m_presetBundle->update_compatible(Slic3r::PresetSelectCompatibleType::Always);

        const auto* nozzles    = preset->config.option<Slic3r::ConfigOptionFloats>("nozzle_diameter");
        const size_t extruders = nozzles ? nozzles->values.size() : 1;

        // handles printers that have double extruder
        if (auto* nozzle_volume_type = m_presetBundle->project_config.option<Slic3r::ConfigOptionEnumsGeneric>("nozzle_volume_type")) {
            const int def = nozzle_volume_type->values.empty() ? 0 : nozzle_volume_type->values.front();
            nozzle_volume_type->values.resize(extruders, def);
        }

        if (m_presetBundle->filament_presets.size() < extruders)
            m_presetBundle->set_num_filaments(static_cast<unsigned>(extruders));

        Slic3r::DynamicPrintConfig config = m_presetBundle->full_config();
        felix_daemon::utils::sync_filament_self_index(*m_presetBundle, config);

        // set the local variables
        m_printerKey    = p.printer_key;
        m_printerPreset = presetName;

        const auto& cfg    = preset->config;
        nlohmann::json bed = nlohmann::json::array();
        if (auto* area = cfg.option<Slic3r::ConfigOptionPoints>("printable_area")) {
            for (const auto& p : area->values)
                bed.push_back({p.x(), p.y()});
        }

        json res_body;
        res_body["bed"]              = bed;
        res_body["printer"]          = preset->name;
        res_body["nozzle_size"]      = p.nozzle_size;
        res_body["printable_height"] = cfg.opt_float("printable_height");

        return orca_success_response("Successfully set the printer", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

/*******************************************************************************
Slice command
********************************************************************************/
orca_response orca_session::cmd_slice(const nlohmann::json& parameters)
{
    if (is_slicing()) {
        return orca_error_response(
            orca_response_code::busy,
            "Slicing is allready in progress, wait until slicing is finished, to see the status query slicing_status");
    }
    try {
        Cmd_Slice_Parameters params;
        params.parse(parameters);

        if (m_plate->objects.empty()) {
            throw std::runtime_error("No model was loaded !, use cmd load model ");
        }

        if (m_printerKey.empty()) {
            throw std::runtime_error("No printer loaded ! Can not slice without setting printer, call set_printer command");
        }

        // get the configuration of the printer (filament  etc etc )
        const auto& printerCfg = m_presetBundle->printers.get_edited_preset().config;
        auto fullConfig        = m_presetBundle->full_config();

        // adjust the filament list to take into the account double sided printers
        felix_daemon::utils::sync_filament_self_index(*m_presetBundle, fullConfig);

        // join the thread if it has finished
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }

        // apply the configuration to the model
        auto applyStatus = m_print->apply(*m_plate, fullConfig);
        m_print->invalidate_gcode_export_from_worker();

        // nothing has changed
        if (applyStatus == Slic3r::PrintBase::APPLY_STATUS_UNCHANGED && !m_lastOutput.empty() && boost::filesystem::exists(m_lastOutput)) {
            if (m_lastOutput != params.outputPath)
                boost::filesystem::copy_file(m_lastOutput, params.outputPath, boost::filesystem::copy_options::overwrite_existing);

            std::lock_guard<std::mutex> lock(m_slicingMutex);
            m_progress            = {};
            m_progress.state      = orca_state::done;
            m_progress.percentage = 100;
            m_progress.output     = params.outputPath;
            m_lastOutput          = params.outputPath;

            json res_body;
            res_body["status"] = "unchanged, reused previous gcode";
            return orca_success_response("Nothing changed since the last slice", res_body);
        }

        m_print->restart();

        felix_daemon::utils::sync_filament_self_index(*m_presetBundle, fullConfig);

        Slic3r::StringObjectException err = m_print->validate();
        if (!err.string.empty()) {
            return orca_error_response(orca_response_code::failed, err.string);
        }
        set_state(orca_state::slicing);

        // the previous g-code no longer matches the print, it is only reused once this slice succeeds
        m_lastOutput.clear();

        //===================================================================================================================
        // Actual slicing happens here
        // it is in the lambda expression, function continues on the main thread while lambda happens on the separate thread
        m_workerThread = std::thread([this, outputPath = params.outputPath]() {
            try {
                m_print->process();

                Slic3r::GCodeProcessorResult gcodeResult;
                std::string written = m_print->export_gcode(outputPath, &gcodeResult, nullptr);

                // do not use set_state, since it would lock the thread twice
                std::lock_guard<std::mutex> lock(m_slicingMutex);
                m_progress.state      = orca_state::done;
                m_progress.percentage = 100;
                m_progress.output     = written;
                // only read by cmd_slice after joining this thread
                m_lastOutput = written;

            } catch (const Slic3r::CanceledException&) {
                std::lock_guard<std::mutex> lock(m_slicingMutex);
                m_progress.state = orca_state::canceled;
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(m_slicingMutex);
                m_progress.state   = orca_state::failed;
                m_progress.message = e.what();
            }
        });

        json res_body;
        res_body["status"]   = "slicing started...";
        res_body["process"]  = m_presetBundle->prints.get_selected_preset_name();
        res_body["filament"] = m_presetBundle->filament_presets.empty() ? "" : m_presetBundle->filament_presets.front();

        return orca_success_response("Successfully started to slice the model", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::failed, e.what());
    }
}

/*******************************************************************************
Check progress
********************************************************************************/
orca_response orca_session::cmd_check_slice_progress(const nlohmann::json& parameters)
{
    orca_slicing_progress progress;
    {
        std::lock_guard<std::mutex> lock(m_slicingMutex);
        progress = m_progress;
    }

    json body;
    body["state"]   = to_string(progress.state);
    body["percent"] = progress.percentage;
    body["text"]    = progress.message;
    if (progress.state == orca_state::done)
        body["gcode_path"] = progress.output;
    if (progress.state == orca_state::failed)
        body["error"] = progress.message;
    return orca_success_response("Status check finished !", body);
}

/*******************************************************************************
Set filament
********************************************************************************/
orca_response orca_session::cmd_set_filament(const nlohmann::json& parameters)
{
    try {
        Cmd_Set_Filament_Parameters p;
        p.parse(parameters);

        if (m_printerKey.empty())
            return orca_error_response(orca_response_code::failed, "set_printer must be called before set_filament");

        // nozzle size and extruder count come from the selected printer
        auto& printer     = m_presetBundle->printers.get_selected_preset();
        auto nozzles      = printer.config.option<Slic3r::ConfigOptionFloats>("nozzle_diameter");
        auto numExtruders = nozzles ? nozzles->values.size() : 1;

        if (p.extruder >= numExtruders)
            return orca_error_response(orca_response_code::invalid_parameters, "extruder index " + std::to_string(p.extruder) +
                                                                                   " out of range (printer has " +
                                                                                   std::to_string(numExtruders) + ")");

        auto nozzleSize = nozzles ? static_cast<float>(nozzles->values[p.extruder]) : 0.f;

        // build the preset name (also checks food vs plastic)
        const auto presetName = felix_daemon::utils::build_filament_preset_string(m_printerKey, p.filament_key, p.nozzle_key, nozzleSize);
        if (presetName.empty())
            return orca_error_response(orca_response_code::failed, "invalid filament/nozzle combination for " + m_printerKey + ": " +
                                                                       p.filament_key + " / " + p.nozzle_key);

        const Slic3r::Preset* preset = m_presetBundle->filaments.find_preset(presetName, false);
        if (!preset || !preset->is_system)
            return orca_error_response(orca_response_code::failed, "orca system profile not found: " + presetName);

        // is_compatible was refreshed by update_compatible() in set_printer
        if (!preset->is_compatible)
            return orca_error_response(orca_response_code::failed, "filament " + presetName + " is not compatible with " + m_printerPreset);

        // make sure the slot exists, then assign it
        if (m_presetBundle->filament_presets.size() < numExtruders)
            m_presetBundle->set_num_filaments(static_cast<unsigned>(numExtruders));

        m_presetBundle->filament_presets[p.extruder] = presetName;

        // slot 0 is also the collection's selected/edited preset (used by full_config for single extruder)
        if (p.extruder == 0)
            m_presetBundle->filaments.select_preset_by_name(presetName, true);

        Slic3r::DynamicPrintConfig config = m_presetBundle->full_config();
        felix_daemon::utils::sync_filament_self_index(*m_presetBundle, config);

        json res_body;
        res_body["filament"]    = preset->name;
        res_body["extruder"]    = p.extruder;
        res_body["nozzle_size"] = nozzleSize;
        res_body["filaments"]   = m_presetBundle->filament_presets;

        return orca_success_response("Successfully set the filament", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

/*******************************************************************************
Set process
********************************************************************************/
orca_response orca_session::cmd_set_process(const nlohmann::json& parameters)
{
    try {
        Cmd_Set_Process_Parameters p;
        p.parse(parameters);

        if (m_printerKey.empty())
            return orca_error_response(orca_response_code::failed, "set_printer must be called before set_process");

        const auto* nozzles = m_presetBundle->printers.get_selected_preset().config.option<Slic3r::ConfigOptionFloats>("nozzle_diameter");
        const float nozzleSize = nozzles && !nozzles->values.empty() ? static_cast<float>(nozzles->values.front()) : 0.f;

        const std::string presetName = felix_daemon::utils::build_proccess_preset_string(m_printerKey, p.process_key, nozzleSize);
        if (presetName.empty())
            return orca_error_response(orca_response_code::failed, "invalid quality for " + m_printerKey + ": " + p.process_key);

        const Slic3r::Preset* preset = m_presetBundle->prints.find_preset(presetName, false);
        if (!preset)
            return orca_error_response(orca_response_code::failed, "orca system profile not found: " + presetName);
        if (!preset->is_compatible)
            return orca_error_response(orca_response_code::failed, presetName + " is not compatible with " + m_printerPreset);

        m_presetBundle->prints.select_preset_by_name(presetName, true);

        json res_body;
        res_body["process"]      = preset->name;
        res_body["layer_height"] = preset->config.opt_float("layer_height");

        return orca_success_response("Successfully set the process", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

/*******************************************************************************
Preset info
********************************************************************************/
orca_response orca_session::cmd_get_preset_info(const nlohmann::json& parameters)
{
    try {
        auto printer   = m_presetBundle->printers.get_selected_preset_name();
        auto filaments = m_presetBundle->filament_presets;
        auto process   = m_presetBundle->prints.get_selected_preset_name();

        const auto* nozzles = m_presetBundle->printers.get_selected_preset().config.option<Slic3r::ConfigOptionFloats>("nozzle_diameter");
        auto numExtruders   = nozzles ? nozzles->size() : 0;

        auto models = nlohmann::json::array();
        for (const auto& [key, model] : m_models)
            models.push_back({{"model_key", key}, {"instances", model->instances.size()}});

        json res_body;
        res_body["printer_key"]   = m_printerKey;
        res_body["printer"]       = printer;
        res_body["filaments"]     = filaments;
        res_body["process"]       = process;
        res_body["models"]        = models;
        res_body["num_extruders"] = numExtruders;

        return orca_success_response("Preset info", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::failed, e.what());
    }
}

/*******************************************************************************
Set transformation (position, rotation and scale)
********************************************************************************/
orca_response orca_session::cmd_set_transform(const nlohmann::json& parameters)
{
    try {
        Cmd_Set_Transform_Parameters params;
        params.parse(parameters);

        std::vector<Slic3r::ModelInstance*> targets;
        targets.reserve(params.items.size());

        // validate all instances onto which transformation can be applied
        for (const auto& item : params.items) {
            Slic3r::ModelObject* obj = find_model(item.model_key);
            if (obj) {
                if (item.instanceid < 0 || item.instanceid >= (int) obj->instances.size()) {
                    throw std::invalid_argument("instance out of range for: " + item.model_key);
                }
                targets.push_back(obj->instances[item.instanceid]);
            } else {
                throw std::runtime_error("Model not found \n \t key: " + item.model_key);
            }
        }

        // apply the transformation
        nlohmann::json res_body = nlohmann::json::array();
        for (size_t i = 0; i < targets.size(); ++i) {
            Slic3r::ModelInstance* inst = targets[i];
            Slic3r::ModelObject* obj    = inst->get_object();

            inst->set_transformation(Slic3r::Geometry::Transformation(params.items[i].transform));
            obj->invalidate_bounding_box();

            const auto bb  = obj->instance_bounding_box(params.items[i].instanceid);
            const auto pos = inst->get_offset();
            res_body.push_back({{"model_key", params.items[i].model_key},
                                {"instance", params.items[i].instanceid},
                                {"position", {pos.x(), pos.y(), pos.z()}},
                                {"bbox_min", {bb.min.x(), bb.min.y(), bb.min.z()}},
                                {"bbox_max", {bb.max.x(), bb.max.y(), bb.max.z()}}});
        }
        return orca_success_response("Successfully set the transforms", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

/*******************************************************************************
Remove model
********************************************************************************/
orca_response orca_session::cmd_remove_model(const nlohmann::json& parameters)
{
    if (is_slicing())
        return orca_error_response(orca_response_code::busy, "Cannot remove a model while slicing, wait until slicing is finished");

    try {
        Cmd_Remove_Model_Parameters params;
        params.parse(parameters);

        auto it = m_models.find(params.model_key);
        if (it == m_models.end())
            return orca_error_response(orca_response_code::invalid_parameters, "unknown model_key: " + params.model_key);

        Slic3r::ModelObject* obj      = it->second;
        const size_t removedInstances = obj->instances.size();

        m_models.erase(it);

        // deletes the object with all of its instances and volumes
        if (!m_plate->delete_object(obj))
            return orca_error_response(orca_response_code::failed, "model was not found on the plate: " + params.model_key);

        json res_body;
        res_body["model_key"]         = params.model_key;
        res_body["removed_instances"] = removedInstances;
        res_body["remaining_models"]  = m_models.size();

        return orca_success_response("Successfully removed the model", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

/*******************************************************************************
Remove instance
********************************************************************************/
orca_response orca_session::cmd_remove_instance(const nlohmann::json& parameters)
{
    if (is_slicing())
        return orca_error_response(orca_response_code::busy, "Cannot remove an instance while slicing, wait until slicing is finished");

    try {
        Cmd_Delete_Instance_Parameters params;
        params.parse(parameters);

        auto it = m_models.find(params.model_key);
        if (it == m_models.end())
            return orca_error_response(orca_response_code::invalid_parameters, "unknown model_key: " + params.model_key);

        Slic3r::ModelObject* obj = it->second;
        const size_t index       = static_cast<size_t>(params.instanceID);

        if (index >= obj->instances.size())
            return orca_error_response(orca_response_code::invalid_parameters, "instance " + std::to_string(index) + " out of range for " +
                                                                                   params.model_key + " (model has " +
                                                                                   std::to_string(obj->instances.size()) + ")");

        json res_body;
        res_body["model_key"] = params.model_key;
        res_body["instance"]  = index;

        // last instance, an object without instances is invalid, so remove the whole model
        if (obj->instances.size() == 1) {
            m_models.erase(it);
            if (!m_plate->delete_object(obj))
                return orca_error_response(orca_response_code::failed, "model was not found on the plate: " + params.model_key);

            res_body["count"]         = 0;
            res_body["model_removed"] = true;
            return orca_success_response("Removed the last instance, model was removed from the plate", res_body);
        }

        obj->delete_instance(index);
        obj->invalidate_bounding_box();

        res_body["count"]         = obj->instances.size();
        res_body["model_removed"] = false;
        return orca_success_response("Successfully removed the instance", res_body);

    } catch (std::exception& e) {
        return orca_error_response(orca_response_code::invalid_parameters, e.what());
    }
}

} // namespace felix_daemon
