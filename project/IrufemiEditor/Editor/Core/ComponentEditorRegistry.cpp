#include "ComponentEditorRegistry.h"

#ifdef EditorMode
#include <imgui/imgui.h>

// Core
#include "IComponentEditor.h"
#include "ComponentUIHelpers.h"

// Component Editors
#include "../ComponentEditors/TransformComponentEditor.h"
#include "../ComponentEditors/MeshRendererComponentEditor.h"
#include "../ComponentEditors/SkinnedMeshRendererComponentEditor.h"
#include "../ComponentEditors/ModelBatchRendererComponentEditor.h"
#include "../ComponentEditors/PrimitiveRendererComponentEditor.h"
#include "../ComponentEditors/Primitive2DRendererComponentEditor.h"
#include "../ComponentEditors/SpriteRendererComponentEditor.h"
#include "../ComponentEditors/TextRendererComponentEditor.h"
#include "../ComponentEditors/ColliderComponentEditors.h"
#include "../ComponentEditors/ParticleEmitterComponentEditor.h"
#include "../ComponentEditors/RaycastComponentEditor.h"
#include "../ComponentEditors/VoxelParticleComponentEditor.h"
#include "../ComponentEditors/EffectMaskComponentEditor.h"
#include "../ComponentEditors/CameraComponentEditor.h"

// Engine Components
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Renderer/Primitive2DRendererComponent.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Framework/Component/Renderer/TextRendererComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Collider/RaycastComponent.h"
#include "Framework/Component/Effect/ParticleEmitterComponent.h"
#include "Framework/Component/Effect/VoxelParticleComponent.h"
#include "Framework/Component/Effect/EffectMaskComponent.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "../../Application_solo/components/GravityPlayerComponent.h"
#include "../../Application_solo/components/editor/GravityPlayerComponentEditor.h"
#include "../../Application_solo/components/Boss/BossComponent.h"
#include "../../Application_solo/components/editor/BossComponentEditor.h"

// =======================================================================
// ComponentEditorRegistry
// =======================================================================

ComponentEditorRegistry::ComponentEditorRegistry() {}
ComponentEditorRegistry::~ComponentEditorRegistry() {}

void ComponentEditorRegistry::RegisterAllEditors() {
    RegisterEditor<TransformComponent, TransformComponentEditor>();
    RegisterEditor<MeshRendererComponent, MeshRendererComponentEditor>();
    RegisterEditor<SkinnedMeshRendererComponent, SkinnedMeshRendererComponentEditor>();
    RegisterEditor<ModelBatchRendererComponent, ModelBatchRendererComponentEditor>();
    RegisterEditor<PrimitiveRendererComponent, PrimitiveRendererComponentEditor>();
    RegisterEditor<Primitive2DRendererComponent, Primitive2DRendererComponentEditor>();
    RegisterEditor<SpriteRendererComponent, SpriteRendererComponentEditor>();
    RegisterEditor<TextRendererComponent, TextRendererComponentEditor>();
    RegisterEditor<AABBColliderComponent, AABBColliderComponentEditor>();
    RegisterEditor<OBBColliderComponent, OBBColliderComponentEditor>();
    RegisterEditor<SphereColliderComponent, SphereColliderComponentEditor>();
    RegisterEditor<RaycastComponent, RaycastComponentEditor>();
    RegisterEditor<ParticleEmitterComponent, ParticleEmitterComponentEditor>();
    RegisterEditor<VoxelParticleComponent, VoxelParticleComponentEditor>();
    RegisterEditor<EffectMaskComponent, EffectMaskComponentEditor>();
    RegisterEditor<CameraComponent, CameraComponentEditor>();
    RegisterEditor<GravityPlayerComponent, GravityPlayerComponentEditor>();
    RegisterEditor<BossComponent, BossComponentEditor>();
}

void ComponentEditorRegistry::DrawComponent(Component* component, EditorActionManager* actionManager) {
    if (!component) return;
    ImGui::PushID(component);
    auto it = editors_.find(typeid(*component));
    if (it != editors_.end()) {
        it->second->Draw(component, actionManager);
    } else {
        ComponentUIHelpers::DrawFallbackPropertiesGUI(component, actionManager);
    }
    ImGui::PopID();
}

#endif // EditorMode
