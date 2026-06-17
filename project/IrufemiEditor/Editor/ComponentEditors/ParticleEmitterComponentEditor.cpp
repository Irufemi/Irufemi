#include "ParticleEmitterComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "Framework/Component/Effect/ParticleEmitterComponent.h"
#include "Renderer/Object/Particle/ParticleObject.h"

void ParticleEmitterComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* compWrapper = static_cast<ParticleEmitterComponent*>(component);
    auto* comp = compWrapper->GetParticleObject();
    
    ImGui::PushID(compWrapper);
    comp->DebugUI("Particle Emitter");
    ImGui::PopID();
}
#endif // EditorMode
