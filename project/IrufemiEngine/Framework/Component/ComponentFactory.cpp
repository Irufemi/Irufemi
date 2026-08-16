#include "ComponentFactory.h"
#include "TransformComponent.h"
#include "Renderer/MeshRendererComponent.h"
#include "Renderer/PrimitiveRendererComponent.h"
#include "Renderer/Primitive2DRendererComponent.h"
#include "Renderer/SpriteRendererComponent.h"
#include "Renderer/TextRendererComponent.h"
#include "Renderer/ModelBatchRendererComponent.h"
#include "Renderer/SkinnedMeshRendererComponent.h"
#include "Renderer/SkeletonDebugRendererComponent.h"
#include "Logic/AnimatorComponent.h"
#include "Logic/BoneAttachmentComponent.h"
#include "Logic/SpawnPointComponent.h"
#include "Effect/VoxelParticleComponent.h"
#include "Effect/EffectMaskComponent.h"
#include "Effect/ScreenEffectComponent.h"
#include "Collider/AABBColliderComponent.h"
#include "Collider/SphereColliderComponent.h"
#include "Collider/OBBColliderComponent.h"
#include "Collider/RaycastComponent.h"
#include "Audio/AudioSourceComponent.h"
#include "Effect/ParticleEmitterComponent.h"
#include "Effect/ParticleFieldComponent.h"
#include "UI/ButtonComponent.h"
#include "UI/CanvasComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeComponent.h"
#include "Camera/TargetFollowComponent.h"
#include "Utility/LifetimeComponent.h"
#include "Utility/SplineComponent.h"
#include "Utility/SplineNodeComponent.h"
#include "VirtualEntity/VirtualEntityManagerComponent.h"
#include "Engine/Core/System/ComponentPool.h"

std::map<std::string, ComponentFactory::ComponentRegistration>& ComponentFactory::GetMap() {
    static std::map<std::string, ComponentRegistration> factoryMap;
    return factoryMap;
}

void ComponentFactory::Register(const std::string& typeName, const char* category, CreatorFunc func) {
    GetMap()[typeName] = { category, func };
}

std::shared_ptr<Component> ComponentFactory::Create(const std::string& typeName) {
    auto& map = GetMap();
    if (map.find(typeName) != map.end()) {
        return map[typeName].creator();
    }
    return nullptr;
}

const std::map<std::string, ComponentFactory::ComponentRegistration>& ComponentFactory::GetFactoryMap() {
    return GetMap();
}

void ComponentFactory::RegisterAllCoreComponents() {
    Register("TransformComponent", "Core", []() { 
        if constexpr (IsPooledComponent<TransformComponent>::value) {
            return std::static_pointer_cast<Component>(ComponentPool<TransformComponent>::GetInstance().Create());
        } else {
            return std::static_pointer_cast<Component>(std::make_shared<TransformComponent>());
        }
    });
    Register("MeshRendererComponent", "Renderer", []() { return std::make_shared<MeshRendererComponent>(); });
    Register("PrimitiveRendererComponent", "Renderer", []() { return std::make_shared<PrimitiveRendererComponent>(); });
    Register("Primitive2DRendererComponent", "Renderer", []() { return std::make_shared<Primitive2DRendererComponent>(); });
    Register("ModelBatchRendererComponent", "Renderer", []() { return std::make_shared<ModelBatchRendererComponent>(); });
    Register("SpriteRendererComponent", "Renderer", []() { return std::make_shared<SpriteRendererComponent>(); });
    Register("TextRendererComponent", "Renderer", []() { return std::make_shared<TextRendererComponent>(); });
    Register("SkinnedMeshRendererComponent", "Renderer", []() { return std::make_shared<SkinnedMeshRendererComponent>(); });
    Register("SkeletonDebugRendererComponent", "Debug", []() { return std::make_shared<SkeletonDebugRendererComponent>(); });
    Register("AnimatorComponent", "Logic", []() { return std::make_shared<AnimatorComponent>(); });
    Register("BoneAttachmentComponent", "Logic", []() { return std::make_shared<BoneAttachmentComponent>(); });
    Register("SpawnPointComponent", "Logic", []() { return std::make_shared<SpawnPointComponent>(); });
    Register("VoxelParticleComponent", "Effect", []() { return std::make_shared<VoxelParticleComponent>(); });
    Register("EffectMaskComponent", "Effect", []() { return std::make_shared<EffectMaskComponent>(); });
    Register("ScreenEffectComponent", "Effect", []() { return std::make_shared<ScreenEffectComponent>(); });
    Register("AABBColliderComponent", "Collider", []() { return std::make_shared<AABBColliderComponent>(); });
    Register("SphereColliderComponent", "Collider", []() { return std::make_shared<SphereColliderComponent>(); });
    Register("OBBColliderComponent", "Collider", []() { return std::make_shared<OBBColliderComponent>(); });
    Register("RaycastComponent", "Collider", []() { return std::make_shared<RaycastComponent>(); });
    Register("AudioSourceComponent", "Audio", []() { return std::make_shared<AudioSourceComponent>(); });
    Register("ParticleEmitterComponent", "Effect", []() { return std::make_shared<ParticleEmitterComponent>(); });
    Register("ParticleFieldComponent", "Effect", []() { return std::make_shared<ParticleFieldComponent>(); });
    Register("ButtonComponent", "UI", []() { return std::make_shared<ButtonComponent>(); });
    Register("CanvasComponent", "UI", []() { return std::make_shared<CanvasComponent>(); });
    Register("CameraComponent", "Camera", []() { return std::make_shared<CameraComponent>(); });
    Register("CameraShakeComponent", "Camera", []() { return std::make_shared<CameraShakeComponent>(); });
    Register("TargetFollowComponent", "Camera", []() { return std::make_shared<TargetFollowComponent>(); });
    Register("LifetimeComponent", "Utility", []() { return std::make_shared<LifetimeComponent>(); });
    Register("SplineComponent", "Utility", []() { return std::make_shared<SplineComponent>(); });
    Register("SplineNodeComponent", "Utility", []() { return std::make_shared<SplineNodeComponent>(); });
    Register("VirtualEntityManagerComponent", "Utility", []() { return std::make_shared<VirtualEntityManagerComponent>(); });
}
