// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <json.hpp>
#include <libslic3r/Point.hpp>

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

static int find_int(const json& j, const std::string& parameter)
{
    auto it = j.find(parameter);
    if (it == j.end() || !it->is_number())
        throw std::invalid_argument("missing or invalid '" + parameter + "'");
    return it->get<int>();
}

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

struct cmdParameters
{
    virtual ~cmdParameters()                   = default;
    virtual void parse(const json& parameters) = 0;
};

struct cmdLoadModel_Parameters : public cmdParameters
{
    boost::filesystem::path path;

    void parse(const json& parameters) override;
};

struct cmdSetPrinter_Parameters : public cmdParameters
{
    std::string printer_key;
    float nozzle_size = 0.f;

    void parse(const json& parameters) override;
};

struct cmdSlice_Parameters : public cmdParameters
{
    std::string modelPath;  // from where the model was loaded (cache key)
    std::string outputPath; // where to store the g-code

    void parse(const json& parameters) override;
};

struct cmdSetFilament_Parameters : public cmdParameters
{
    std::string filament_key;
    std::string nozzle_key;
    int extruder = 0;

    void parse(const json& parameters) override;
};

struct cmdSetProcess_Parameters : public cmdParameters
{
    std::string process_key;
    void parse(const json& parameters) override;
};

struct cmdSetTransformParameters : public cmdParameters
{
    struct Item
    {
        Slic3r::Transform3d transform = Slic3r::Transform3d::Identity();
        std::string model_key;
        int instanceid = 0;
    };

    std::vector<Item> items;

    void parse(const json& parameters) override;
};

} // namespace felix_daemon