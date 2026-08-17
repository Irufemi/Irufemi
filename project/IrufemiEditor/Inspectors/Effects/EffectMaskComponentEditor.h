#pragma once

#ifdef EditorMode
#include "Core/IComponentEditor.h"

class EffectMaskComponent;

class EffectMaskComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
#endif // EditorMode
