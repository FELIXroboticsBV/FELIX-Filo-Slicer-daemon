// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

/*
  ====================================================================================
  orca_request_parameters.hpp
  ====================================================================================

  Set of structures to convert json parapets into the data safe representation on the c++ side.
*/

#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <json.hpp>
#include <libslic3r/Point.hpp>

namespace felix_daemon {
using namespace nlohmann;

/*-----------------------------------------------------------------------------------------
 Helpers
-----------------------------------------------------------------------------------------*/
static std::string find_string(const json& j, const std::string& parameter)
{
    auto it = j.find(parameter);
    if (it == j.end() || !it->is_string() || it->get<std::string>().empty())
        throw std::invalid_argument("missing or invalid '" + parameter + "'");
    return it->get<std::string>();
}

//-----------------------------------------------------------------------------------------

static float find_float(const json& j, const std::string& parameter)
{
    auto it = j.find(parameter);
    if (it == j.end() || !it->is_number())
        throw std::invalid_argument("missing or invalid '" + parameter + "'");
    return it->get<float>();
}

//-----------------------------------------------------------------------------------------

static int find_int(const json& j, const std::string& parameter)
{
    auto it = j.find(parameter);
    if (it == j.end() || !it->is_number())
        throw std::invalid_argument("missing or invalid '" + parameter + "'");
    return it->get<int>();
}

//-----------------------------------------------------------------------------------------

static Slic3r::Transform3d find_mat4(const json& j, const std::string& parameter)
{
    auto it = j.find(parameter);
    if (it == j.end() || !it->is_array() || it->size() != 16)
        throw std::invalid_argument("missing or invalid '" + parameter + "' (expected 16 numbers)");

    Eigen::Matrix<double, 4, 4, Eigen::RowMajor> m;
    for (int i = 0; i < 16; ++i) {
        const auto& v = (*it)[i];
        if (!v.is_number() || !std::isfinite(v.get<double>()))
            throw std::invalid_argument("'" + parameter + "' must contain only finite numbers");
        m(i / 4, i % 4) = v.get<double>();
    }

    if (!m.row(3).isApprox(Eigen::RowVector4d(0, 0, 0, 1)))
        throw std::invalid_argument("'" + parameter + "' bottom row must be 0 0 0 1");
    if (std::abs(m.topLeftCorner<3, 3>().determinant()) < 1e-9)
        throw std::invalid_argument("'" + parameter + "' is degenerate (zero scale)");

    Slic3r::Transform3d t;
    t.matrix() = m;
    return t;
}

//-----------------------------------------------------------------------------------------

/*-----------------------------------------------------------------------------------------
 Parameters structs
-----------------------------------------------------------------------------------------*/
struct Cmd_Parameters
{
    virtual ~Cmd_Parameters()                  = default;
    virtual void parse(const json& parameters) = 0;
};

struct Cmd_Load_Model_Parameters : public Cmd_Parameters
{
    boost::filesystem::path path;

    void parse(const json& parameters) override;
};

struct Cmd_Set_Printer_Parameters : public Cmd_Parameters
{
    std::string printer_key;
    float nozzle_size = 0.f;

    void parse(const json& parameters) override;
};

struct Cmd_Slice_Parameters : public Cmd_Parameters
{
    std::string modelPath;  // from where the model was loaded (cache key)
    std::string outputPath; // where to store the g-code

    void parse(const json& parameters) override;
};

struct Cmd_Set_Filament_Parameters : public Cmd_Parameters
{
    std::string filament_key;
    std::string nozzle_key;
    int extruder = 0;

    void parse(const json& parameters) override;
};

struct Cmd_Set_Process_Parameters : public Cmd_Parameters
{
    std::string process_key;
    void parse(const json& parameters) override;
};

struct Cmd_Set_Transform_Parameters : public Cmd_Parameters
{
    // each item is an instance of a specific model
    struct Item
    {
        Slic3r::Transform3d transform = Slic3r::Transform3d::Identity();
        std::string model_key;
        int instanceid = 0;
    };

    std::vector<Item> items;

    void parse(const json& parameters) override;
};

struct Cmd_Instantiate_Model_Parameters : public Cmd_Parameters
{
    std::string model_key;
    std::optional<Slic3r::Transform3d> transform;

    void parse(const json& parameters) override;
};

struct Cmd_Remove_Model_Parameters : public Cmd_Parameters
{
    std::string model_key;
    void parse(const json& parameters) override;
};

struct Cmd_Delete_Instance_Parameters : public Cmd_Parameters
{
    std::string model_key;
    int instanceID;
    void parse(const json& parameters) override;
};

} // namespace felix_daemon