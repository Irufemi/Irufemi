#include "ComponentEditorRegistry.h"

#ifdef EditorMode
#include <imgui/imgui.h>

// Core
#include "IComponentEditor.h"
#include "ComponentUIHelpers.h"

// Component Editors
#include "../ComponentEditors/TransformComponentEditor.h"
#include "../ComponentEditors/MeshRendererComponentEditor.h"
#include "../ComponentEditors/ModelBatchRendererComponentEditor.h"
#include "../ComponentEditors/PrimitiveRendererComponentEditor.h"
#include "../ComponentEditors/SpriteRendererComponentEditor.h"
#include "../ComponentEditors/TextRendererComponentEditor.h"
#include "../ComponentEditors/ColliderComponentEditors.h"
#include "../ComponentEditors/ParticleEmitterComponentEditor.h"
#include "../ComponentEditors/RaycastComponentEditor.h"

// Engine Components
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Framework/Component/Renderer/TextRendererComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Collider/RaycastComponent.h"
#include "Framework/Component/Effect/ParticleEmitterComponent.h"

// =======================================================================
// ComponentEditorRegistry
// =======================================================================

ComponentEditorRegistry::ComponentEditorRegistry() {}
ComponentEditorRegistry::~ComponentEditorRegistry() {}

void ComponentEditorRegistry::RegisterAllEditors() {
    RegisterEditor<TransformComponent, TransformComponentEditor>();
    RegisterEditor<MeshRendererComponent, MeshRendererComponentEditor>();
    RegisterEditor<ModelBatchRendererComponent, ModelBatchRendererComponentEditor>();
    RegisterEditor<PrimitiveRendererComponent, PrimitiveRendererComponentEditor>();
    RegisterEditor<SpriteRendererComponent, SpriteRendererComponentEditor>();
    RegisterEditor<TextRendererComponent, TextRendererComponentEditor>();
    RegisterEditor<AABBColliderComponent, AABBColliderComponentEditor>();
    RegisterEditor<OBBColliderComponent, OBBColliderComponentEditor>();
    RegisterEditor<SphereColliderComponent, SphereColliderComponentEditor>();
    RegisterEditor<RaycastComponent, RaycastComponentEditor>();
    RegisterEditor<ParticleEmitterComponent, ParticleEmitterComponentEditor>();
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
