#pragma once

#ifdef EditorMode
#include "Core/IComponentEditor.h"

class Primitive2DRendererComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
#endif // EditorMode
