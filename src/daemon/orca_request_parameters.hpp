// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <json.hpp>

namespace felix_daemon {
using namespace nlohmann;
//=========================================================================================
//-----------------------------------------------------------------------------------------
// Parameters struct
//-----------------------------------------------------------------------------------------
//=========================================================================================

static std::string find_string(const json& j, const std::string& parameter)
{
    auto it = j.find(parameter);
    if (it == j.end() || !it->is_string() || it->get<std::string>().empty())
        throw std::invalid_argument("missing or invalid '" + parameter + "'");
    return it->get<std::string>();
}

static float find_float(const json& j, const std::string& parameter)
{
    auto it = j.find(parameter);
    if (it == j.end() || !it->is_number())
        throw std::invalid_argument("missing or invalid '" + parameter + "'");
    return it->get<float>();
}

struct cmdParameters
{
    virtual ~cmdParameters()                   = default;
    virtual void parse(const json& parameters) = 0;
};

struct cmdLoadModel_Parameters : public cmdParameters
{
    boost::filesystem::path path;

    void parse(const json& parameters) override
    {
        path = find_string(parameters, "path");
        if (!boost::filesystem::exists(path))
            throw std::invalid_argument("model not found: " + path.string());
    }
};

struct cmdSetPrinter_Parameters : public cmdParameters
{
    std::string printer_key;
    float nozzle_size = 0.f;

    void parse(const json& parameters) override
    {
        printer_key = find_string(parameters, "printer_key");
        nozzle_size = find_float(parameters, "nozzle_size");
        if (nozzle_size <= 0.f)
            throw std::invalid_argument("'nozzle_size' must be greater than 0");
    }
};

struct cmdSlice_Parameters : public cmdParameters
{
    std::string modelPath;  // from where the model was loaded (cache key)
    std::string outputPath; // where to store the g-code

    void parse(const json& parameters) override
    {
        modelPath  = find_string(parameters, "modelPath");
        outputPath = find_string(parameters, "outputPath");
    }
};

struct cmdSetFilament_Parameters : public cmdParameters
{
    std::string filament_key;
    std::string nozzle_key;
    int extruder = 0;

    void parse(const json& parameters) override
    {
        filament_key = find_string(parameters, "filament_key");
        nozzle_key   = find_string(parameters, "nozzle_key");
        extruder     = find_float(parameters, "extruder");
    }
};

struct cmdSetProcess_Parameters : public cmdParameters
{
    std::string process_key;
    void parse(const json& parameters) override { process_key = find_string(parameters, "process_key"); }
};

} // namespace felix_daemon