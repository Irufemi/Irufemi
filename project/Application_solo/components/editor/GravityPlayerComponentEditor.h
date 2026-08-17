#pragma once
#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
#include "Editor/Core/IComponentEditor.h"

class GravityPlayerComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
#endif
