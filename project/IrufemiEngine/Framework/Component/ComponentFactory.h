#pragma once
#include <string>
#include <memory>
#include <map>
#include <functional>
#include "Component.h"

class ComponentFactory {
public:
    using CreatorFunc = std::function<std::shared_ptr<Component>()>;

    struct ComponentRegistration {
        const char* category;
        CreatorFunc creator;
    };

    /**
     * @brief Register を実行する。
     */
    static void Register(const std::string& typeName, const char* category, CreatorFunc func);
    /**
     * @brief Create を実行する。
     */
    static std::shared_ptr<Component> Create(const std::string& typeName);
    static const std::map<std::string, ComponentRegistration>& GetFactoryMap();

    /// @brief エンジン組み込みのコンポーネントを一括登録する
    static void RegisterAllCoreComponents();

private:
    static std::map<std::string, ComponentRegistration>& GetMap();
};
