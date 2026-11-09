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
#include <boost/filesystem/path.hpp>
#include <json_fwd.hpp>
#include <mutex>
#include <thread>

namespace Slic3r {
class Model;
class PresetBundle;
class Print;
} // namespace Slic3r

namespace felix_daemon {

enum class orca_state { idle, slicing, done, failed, canceled };

struct orca_slicing_progress
{
    int percentage      = 0;
    std::string message = "";
    orca_state state    = orca_state::idle;
    std::string output  = "";
};

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

    /**
     * Sets the printer that will be used for slicing
     * @param  {nlohmann::json} parameters : data received from the request
     * @return {orca_response}             : response from the orca slicer
     */
    orca_response cmd_set_printer(const nlohmann::json& parameters);

    /**
     * Slices the model that was loaded by the session
     * @param  {nlohmann::json} parameters : data received from the request
     * @return {orca_response}             : response from the orca slicer
     */
    orca_response cmd_slice(const nlohmann::json& parameters);

    /**
     * Checks the slicing progress
     * @param  {nlohmann::json} parameters : data received from the request
     * @return {orca_response}             : parameters are going to be empty in this one
     */
    orca_response cmd_check_slice_progress(const nlohmann::json& parameters);

    /**
     * Set the filament for slicing
     * @param  {nlohmann::json} parameters : data received from the request
     * @return {orca_response}             : parameters are going to be empty in this one
     */
    orca_response cmd_set_filament(const nlohmann::json& parameters);

    /**
     * Set the filament for slicing
     * @param  {nlohmann::json} parameters : data received from the request
     * @return {orca_response}             : parameters are going to be empty in this one
     */
    orca_response cmd_set_process(const nlohmann::json& parameters);

    /**
     * Retrieves data about currently set printer, fillament and model
     * @param  {nlohmann::json} parameters : data received from the request
     * @return {orca_response}             : parameters are going to be empty in this one
     */
    orca_response cmd_get_preset_info(const nlohmann::json& parameters);

    /**
     * Checks the status if the worker thread is busy with slicing
     * @return {bool}  : true if status is slicing false otherwise
     */
    bool is_slicing() const;

    /**
     * Sets the state orca is currently in
     * @param  {orca_state} new_state :  new state
     */
    void set_state(orca_state new_state);

    ~orca_session();

private:
    /**
     * All functionality that must happen on the separate thread
     * @param  {std::string} output_path : where the final g-code will be put
     */
    void slice_worker(std::string output_path);

private:
    std::unordered_map<std::string, std::unique_ptr<Slic3r::Model>> m_meshCache;
    std::unique_ptr<Slic3r::PresetBundle> m_presetBundle;
    std::unique_ptr<Slic3r::Print> m_print;

    std::string m_printerKey;
    std::string m_printerPreset;
    boost::filesystem::path m_lastOutput;

    orca_slicing_progress m_progress;

    mutable std::mutex m_slicingMutex;
    std::thread m_workerThread;
};

} // namespace felix_daemon
