#pragma once

#include <engine/onnx_engine.hpp>

#include <string>

// Application settings loaded from vision_pilot.conf (key = value format).
struct VisionPilotConfig {
    std::string autodrive_model;
    std::string autosteer_model;
    std::string autospeed_model;
    engine::EngineConfig engine;
};

// Load config from a .conf file. Expands leading ~ to $HOME.
// Throws std::runtime_error if the file is missing or required keys are absent.
VisionPilotConfig load_vision_pilot_config(const std::string& config_path);

// Resolve config path: --config <path>, then $VISIONPILOT_CONFIG,
// then ./config/vision_pilot.conf, then ./vision_pilot.conf.
// Returns empty string if nothing was found.
std::string resolve_vision_pilot_config_path(int argc, char** argv);
