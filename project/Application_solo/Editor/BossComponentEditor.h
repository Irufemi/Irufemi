#pragma once
#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
#include "Editor/Core/IComponentEditor.h"

class BossComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
#endif
