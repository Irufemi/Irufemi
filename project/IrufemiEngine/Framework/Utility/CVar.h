#pragma once
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace Irufemi {

// ---------------------------------------------------------
// CVar (Console Variable) Data Structure
// ---------------------------------------------------------
struct CVar {
    enum class Type { Int, Float, Bool, String };

    std::string name;
    std::string description;
    Type type;

    std::variant<int, float, bool, std::string> value;
    std::variant<int, float, bool, std::string> defaultValue;

    // Optional callback when value changes
    std::function<void()> onChangeCallback;

    CVar() = default;
};

// ---------------------------------------------------------
// CVarSystem (Static Registry)
// ---------------------------------------------------------
class CVarSystem {
public:
    // マクロから自動登録される関数
    static void RegisterInt(const std::string& name, int defaultValue, const std::string& description);
    static void RegisterFloat(const std::string& name, float defaultValue, const std::string& description);
    static void RegisterBool(const std::string& name, bool defaultValue, const std::string& description);
    static void RegisterString(const std::string& name, const std::string& defaultValue,
                               const std::string& description);

    // 取得
    static int GetInt(const std::string& name);
    static float GetFloat(const std::string& name);
    static bool GetBool(const std::string& name);
    static std::string GetString(const std::string& name);

    // 更新
    static void SetInt(const std::string& name, int value);
    static void SetFloat(const std::string& name, float value);
    static void SetBool(const std::string& name, bool value);
    static void SetString(const std::string& name, const std::string& value);

    // コールバックの紐付け
    static void SetOnChangeCallback(const std::string& name, std::function<void()> callback);

    // ファイルI/O
    static void Load(const std::string& filepath);
    static void Save(const std::string& filepath);

    // 内部辞書の取得 (DebugUI用など)
    static std::unordered_map<std::string, std::unique_ptr<CVar>>& GetRegistry();
};

} // namespace Irufemi

// ---------------------------------------------------------
// Macros for Global Registration in cpp files
// ---------------------------------------------------------
// (Meyers Singleton initialization pattern equivalent for macros is just direct struct instantiation,
// but we will put these macros ONLY in EngineCVars.cpp, so static order within that file is top-down).
// Wait, to allow any variable name characters, we can sanitize or just use __LINE__ token pasting.

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)

#define DEFINE_CVAR_INT(name, defaultValue, description)                                                               \
    namespace {                                                                                                        \
    struct MACRO_CONCAT(CVarReg_, __LINE__) {                                                                          \
        MACRO_CONCAT(CVarReg_, __LINE__)() {                                                                           \
            Irufemi::CVarSystem::RegisterInt(name, defaultValue, description);                                         \
        }                                                                                                              \
    } MACRO_CONCAT(cvarInst_, __LINE__);                                                                               \
    }

#define DEFINE_CVAR_FLOAT(name, defaultValue, description)                                                             \
    namespace {                                                                                                        \
    struct MACRO_CONCAT(CVarReg_, __LINE__) {                                                                          \
        MACRO_CONCAT(CVarReg_, __LINE__)() {                                                                           \
            Irufemi::CVarSystem::RegisterFloat(name, defaultValue, description);                                       \
        }                                                                                                              \
    } MACRO_CONCAT(cvarInst_, __LINE__);                                                                               \
    }

#define DEFINE_CVAR_BOOL(name, defaultValue, description)                                                              \
    namespace {                                                                                                        \
    struct MACRO_CONCAT(CVarReg_, __LINE__) {                                                                          \
        MACRO_CONCAT(CVarReg_, __LINE__)() {                                                                           \
            Irufemi::CVarSystem::RegisterBool(name, defaultValue, description);                                        \
        }                                                                                                              \
    } MACRO_CONCAT(cvarInst_, __LINE__);                                                                               \
    }

#define DEFINE_CVAR_STRING(name, defaultValue, description)                                                            \
    namespace {                                                                                                        \
    struct MACRO_CONCAT(CVarReg_, __LINE__) {                                                                          \
        MACRO_CONCAT(CVarReg_, __LINE__)() {                                                                           \
            Irufemi::CVarSystem::RegisterString(name, defaultValue, description);                                      \
        }                                                                                                              \
    } MACRO_CONCAT(cvarInst_, __LINE__);                                                                               \
    }
