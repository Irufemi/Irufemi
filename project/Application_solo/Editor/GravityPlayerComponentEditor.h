#pragma once
#ifdef EditorMode
#include "IrufemiEditor/Core/IComponentEditor.h"

class GravityPlayerComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
#endif
