#pragma once

#include <nlohmann/json.hpp>
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

namespace Irufemi {
namespace JsonUtility {

/**
 * @brief Vector2 を JSON に変換する
 */
inline nlohmann::json ToJson(const Irufemi::Vector2& v) {
    return nlohmann::json::array({v.x, v.y});
}

/**
 * @brief JSON から Vector2 に変換する
 */
inline Irufemi::Vector2 ToVector2(const nlohmann::json& j,
                                  const Irufemi::Vector2& defaultValue = Irufemi::Vector2(0.0f, 0.0f)) {
    if (j.is_array() && j.size() >= 2) {
        return Irufemi::Vector2(j[0].get<float>(), j[1].get<float>());
    }
    return defaultValue;
}

/**
 * @brief Vector3 を JSON に変換する
 */
inline nlohmann::json ToJson(const Irufemi::Vector3& v) {
    return nlohmann::json::array({v.x, v.y, v.z});
}

/**
 * @brief JSON から Vector3 に変換する
 */
inline Irufemi::Vector3 ToVector3(const nlohmann::json& j,
                                  const Irufemi::Vector3& defaultValue = Irufemi::Vector3(0.0f, 0.0f, 0.0f)) {
    if (j.is_array() && j.size() >= 3) {
        return Irufemi::Vector3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
    }
    return defaultValue;
}

/**
 * @brief Vector4 を JSON に変換する
 */
inline nlohmann::json ToJson(const Irufemi::Vector4& v) {
    return nlohmann::json::array({v.x, v.y, v.z, v.w});
}

/**
 * @brief JSON から Vector4 に変換する
 */
inline Irufemi::Vector4 ToVector4(const nlohmann::json& j,
                                  const Irufemi::Vector4& defaultValue = Irufemi::Vector4(0.0f, 0.0f, 0.0f, 0.0f)) {
    if (j.is_array() && j.size() >= 4) {
        return Irufemi::Vector4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
    }
    return defaultValue;
}

} // namespace JsonUtility
} // namespace Irufemi
