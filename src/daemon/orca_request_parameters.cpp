#include "orca_request_parameters.hpp"

namespace felix_daemon {
using namespace nlohmann;
void cmdLoadModel_Parameters::parse(const json& parameters)
{
    path = find_string(parameters, "path");
    if (!boost::filesystem::exists(path))
        throw std::invalid_argument("model not found: " + path.string());
}

void cmdSetPrinter_Parameters::parse(const json& parameters)
{
    printer_key = find_string(parameters, "printer_key");
    nozzle_size = find_float(parameters, "nozzle_size");
    if (nozzle_size <= 0.f)
        throw std::invalid_argument("'nozzle_size' must be greater than 0");
}

void cmdSlice_Parameters::parse(const json& parameters)
{
    modelPath  = find_string(parameters, "modelPath");
    outputPath = find_string(parameters, "outputPath");
}

void cmdSetFilament_Parameters::parse(const json& parameters)
{
    filament_key = find_string(parameters, "filament_key");
    nozzle_key   = find_string(parameters, "nozzle_key");
    extruder     = find_float(parameters, "extruder");
}

void cmdSetProcess_Parameters::parse(const json& parameters) { process_key = find_string(parameters, "process_key"); }

void cmdSetTransformParameters::parse(const json& parameters)
{
    if (!parameters.contains("objects") || !parameters["objects"].is_array() || parameters["objects"].empty())
        throw std::invalid_argument("objects must be a non-empty array");

    for (const auto& obj : parameters["objects"]) {
        Item item;
        item.model_key  = find_string(obj, "model_key");
        item.instanceid = find_int(obj, "instance_id");
        item.transform  = find_mat4(obj, "transform");

        items.push_back(item);
    }
}

void cmdInstantiateModelParameters::parse(const json& parameters)
{
    model_key = find_string(parameters, "model_key");

    if (parameters.contains("transform")) {
        transform = find_mat4(parameters, "transform");
    }
}

} // namespace felix_daemon