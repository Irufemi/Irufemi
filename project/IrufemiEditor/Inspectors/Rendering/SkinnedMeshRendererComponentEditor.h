#pragma once
#include "Core/IComponentEditor.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"

class SkinnedMeshRendererComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};
