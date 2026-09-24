#pragma once

#ifdef EditorMode
#include "Core/IComponentEditor.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"

/**
 * @class SkinnedMeshRendererComponentEditor
 * @brief SkinnedMeshRendererComponent のエディタ（インスペクター）描画クラス
 */
class SkinnedMeshRendererComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};

#endif // EditorMode
