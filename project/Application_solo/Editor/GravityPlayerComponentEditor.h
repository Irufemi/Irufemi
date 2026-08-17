#pragma once
#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
#include "Core/IComponentEditor.h"

class GravityPlayerComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
#endif
