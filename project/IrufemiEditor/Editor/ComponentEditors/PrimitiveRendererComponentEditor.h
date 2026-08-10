#pragma once

#ifdef EditorMode
#include "../Core/IComponentEditor.h"

class PrimitiveRendererComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
#endif // EditorMode
