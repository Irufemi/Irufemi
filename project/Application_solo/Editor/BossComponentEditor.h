#pragma once
#ifdef EditorMode
#include "IrufemiEditor/Core/IComponentEditor.h"

#include <nlohmann/json.hpp>
#include <string>

class BossComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;

private:
    nlohmann::json cachedJson_;
    std::string cachedPath_;
    bool isJsonLoaded_ = false;
};
#endif
