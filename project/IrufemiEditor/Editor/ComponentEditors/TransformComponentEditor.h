#pragma once

#ifdef EditorMode
#include "../Core/IComponentEditor.h"

class TransformComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
#endif // EditorMode
