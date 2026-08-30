#include "CVar.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace Irufemi {

// Meyer's Singleton pattern internally to hold the registry safely
std::unordered_map<std::string, std::unique_ptr<CVar>>& CVarSystem::GetRegistry() {
    static std::unordered_map<std::string, std::unique_ptr<CVar>> registry;
    return registry;
}

// ---------------------------------------------------------
// Registration
// ---------------------------------------------------------
void CVarSystem::RegisterInt(const std::string& name, int defaultValue, const std::string& description) {
    auto& reg = GetRegistry();
    if (reg.find(name) == reg.end()) {
        auto cvar = std::make_unique<CVar>();
        cvar->name = name;
        cvar->description = description;
        cvar->type = CVar::Type::Int;
        cvar->defaultValue = defaultValue;
        cvar->value = defaultValue;
        reg[name] = std::move(cvar);
    }
}

void CVarSystem::RegisterFloat(const std::string& name, float defaultValue, const std::string& description) {
    auto& reg = GetRegistry();
    if (reg.find(name) == reg.end()) {
        auto cvar = std::make_unique<CVar>();
        cvar->name = name;
        cvar->description = description;
        cvar->type = CVar::Type::Float;
        cvar->defaultValue = defaultValue;
        cvar->value = defaultValue;
        reg[name] = std::move(cvar);
    }
}

void CVarSystem::RegisterBool(const std::string& name, bool defaultValue, const std::string& description) {
    auto& reg = GetRegistry();
    if (reg.find(name) == reg.end()) {
        auto cvar = std::make_unique<CVar>();
        cvar->name = name;
        cvar->description = description;
        cvar->type = CVar::Type::Bool;
        cvar->defaultValue = defaultValue;
        cvar->value = defaultValue;
        reg[name] = std::move(cvar);
    }
}

void CVarSystem::RegisterString(const std::string& name, const std::string& defaultValue,
                                const std::string& description) {
    auto& reg = GetRegistry();
    if (reg.find(name) == reg.end()) {
        auto cvar = std::make_unique<CVar>();
        cvar->name = name;
        cvar->description = description;
        cvar->type = CVar::Type::String;
        cvar->defaultValue = defaultValue;
        cvar->value = defaultValue;
        reg[name] = std::move(cvar);
    }
}

// ---------------------------------------------------------
// Getters
// ---------------------------------------------------------
int CVarSystem::GetInt(const std::string& name) {
    auto& reg = GetRegistry();
    if (reg.find(name) != reg.end() && reg[name]->type == CVar::Type::Int) {
        return std::get<int>(reg[name]->value);
    }
    return 0;
}

float CVarSystem::GetFloat(const std::string& name) {
    auto& reg = GetRegistry();
    if (reg.find(name) != reg.end() && reg[name]->type == CVar::Type::Float) {
        return std::get<float>(reg[name]->value);
    }
    return 0.0f;
}

bool CVarSystem::GetBool(const std::string& name) {
    auto& reg = GetRegistry();
    if (reg.find(name) != reg.end() && reg[name]->type == CVar::Type::Bool) {
        return std::get<bool>(reg[name]->value);
    }
    return false;
}

std::string CVarSystem::GetString(const std::string& name) {
    auto& reg = GetRegistry();
    if (reg.find(name) != reg.end() && reg[name]->type == CVar::Type::String) {
        return std::get<std::string>(reg[name]->value);
    }
    return "";
}

// ---------------------------------------------------------
// Setters
// ---------------------------------------------------------
void CVarSystem::SetInt(const std::string& name, int value) {
    auto& reg = GetRegistry();
    if (reg.find(name) != reg.end() && reg[name]->type == CVar::Type::Int) {
        reg[name]->value = value;
        if (reg[name]->onChangeCallback) {
            reg[name]->onChangeCallback();
        }
    }
}

void CVarSystem::SetFloat(const std::string& name, float value) {
    auto& reg = GetRegistry();
    if (reg.find(name) != reg.end() && reg[name]->type == CVar::Type::Float) {
        reg[name]->value = value;
        if (reg[name]->onChangeCallback) {
            reg[name]->onChangeCallback();
        }
    }
}

void CVarSystem::SetBool(const std::string& name, bool value) {
    auto& reg = GetRegistry();
    if (reg.find(name) != reg.end() && reg[name]->type == CVar::Type::Bool) {
        reg[name]->value = value;
        if (reg[name]->onChangeCallback) {
            reg[name]->onChangeCallback();
        }
    }
}

void CVarSystem::SetString(const std::string& name, const std::string& value) {
    auto& reg = GetRegistry();
    if (reg.find(name) != reg.end() && reg[name]->type == CVar::Type::String) {
        reg[name]->value = value;
        if (reg[name]->onChangeCallback) {
            reg[name]->onChangeCallback();
        }
    }
}

// ---------------------------------------------------------
// Callback Setup
// ---------------------------------------------------------
void CVarSystem::SetOnChangeCallback(const std::string& name, std::function<void()> callback) {
    auto& reg = GetRegistry();
    if (reg.find(name) != reg.end()) {
        reg[name]->onChangeCallback = std::move(callback);
        // Call it immediately once when registered so current values apply
        reg[name]->onChangeCallback();
    }
}

// ---------------------------------------------------------
// File I/O
// ---------------------------------------------------------
void CVarSystem::Load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return; // File doesn't exist yet, that's fine
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (...) {
        return; // Parse error
    }

    auto& reg = GetRegistry();
    for (auto& [key, val] : j.items()) {
        if (reg.find(key) != reg.end()) {
            auto& cvar = reg[key];
            if (cvar->type == CVar::Type::Int && val.is_number_integer()) {
                cvar->value = val.get<int>();
            } else if (cvar->type == CVar::Type::Float && val.is_number_float()) {
                cvar->value = val.get<float>();
            } else if (cvar->type == CVar::Type::Bool && val.is_boolean()) {
                cvar->value = val.get<bool>();
            } else if (cvar->type == CVar::Type::String && val.is_string()) {
                cvar->value = val.get<std::string>();
            }

            // If callback is already set (unlikely during early load, but just in case)
            if (cvar->onChangeCallback) {
                cvar->onChangeCallback();
            }
        }
    }
}

void CVarSystem::Save(const std::string& filepath) {
    nlohmann::json j;
    auto& reg = GetRegistry();

    for (const auto& pair : reg) {
        const auto& cvar = pair.second;
        if (cvar->type == CVar::Type::Int) {
            j[cvar->name] = std::get<int>(cvar->value);
        } else if (cvar->type == CVar::Type::Float) {
            j[cvar->name] = std::get<float>(cvar->value);
        } else if (cvar->type == CVar::Type::Bool) {
            j[cvar->name] = std::get<bool>(cvar->value);
        } else if (cvar->type == CVar::Type::String) {
            j[cvar->name] = std::get<std::string>(cvar->value);
        }
    }

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

} // namespace Irufemi
