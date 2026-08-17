#pragma once
#ifdef EditorMode
#include "Core/IComponentEditor.h"
#include <memory>

class VoxelParticleComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
#endif // EditorMode
