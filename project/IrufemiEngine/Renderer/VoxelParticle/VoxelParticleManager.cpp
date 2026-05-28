#include "VoxelParticleManager.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"

void VoxelParticleManager::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
}

void VoxelParticleManager::ReservePool(const std::string& modelName, const Vector3Int& resolution, int poolSize) {
    for (int i = 0; i < poolSize; ++i) {
        auto voxel = std::make_unique<VoxelParticleSystem>();
        voxel->Initialize(modelName, resolution);
        // バックグラウンドロードを行いつつプールに追加
        pool_.push_back({ modelName, std::move(voxel) });
    }
}

VoxelParticleSystem* VoxelParticleManager::AllocateSystem(const std::string& modelName) {
    for (auto& entry : pool_) {
        // 同じモデル名で、かつ現在アクティブでないものを使用
        if (entry.modelName == modelName && !entry.system->IsActive()) {
            return entry.system.get();
        }
    }
    
    // プールが空の場合はフォールバックとして一つ生成（カクつく原因になるので基本は避ける）
    // セーフガード: 無限にプールが肥大化しDescriptorPoolを枯渇させるのを防ぐ
    constexpr size_t kMaxPoolLimit = 256;
    if (pool_.size() >= kMaxPoolLimit) {
        return nullptr;
    }

    auto voxel = std::make_unique<VoxelParticleSystem>();
    voxel->Initialize(modelName, {32, 32, 32}); 
    pool_.push_back({ modelName, std::move(voxel) });
    return pool_.back().system.get();
}

void VoxelParticleManager::Update(float deltaTime) {
    for (auto& entry : pool_) {
        // 描画準備完了状態か、実行中の場合のみUpdateを呼んで状態を更新する
        if (entry.system->IsActive() || entry.system->GetStatus() == VoxelParticleSystem::LoadingStatus::ReadyToCreateResources) {
            entry.system->Update(deltaTime);
        }
    }
}

void VoxelParticleManager::Draw() {
    for (auto& entry : pool_) {
        if (entry.system->IsActive()) {
            engine_->SetBlend(BlendMode::kBlendModeNormal);
            engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
            engine_->SetCull(PSOManager::CullMode::Back);
            entry.system->Draw();
        }
    }
}

void VoxelParticleManager::PlayExplosion(const std::string& modelName, 
                                         const Vector3& position, 
                                         const Vector3& velocity, 
                                         const Vector3& rotate, 
                                         const Vector3& scale,
                                         VoxelParticleSystem::ParticleType type) {
    auto system = AllocateSystem(modelName);
    if (system) {
        system->SetParticleType(type);
        if (type == VoxelParticleSystem::ParticleType::Building) {
            system->SetGravity(40.0f);
        }
        system->Explode(position, velocity, rotate, scale);
    }
}

void VoxelParticleManager::PlayCollisionScatter(const std::string& modelName, 
                                                const Vector3& position, 
                                                const Vector3& velocity, 
                                                const Vector3& rotate, 
                                                const Vector3& scale, 
                                                const struct OBB& collisionArea,
                                                VoxelParticleSystem::ParticleType type) {
    auto system = AllocateSystem(modelName);
    if (system) {
        system->SetParticleType(type);
        if (type == VoxelParticleSystem::ParticleType::Building) {
            system->SetGravity(40.0f);
        }
        system->CollisionScatter(position, velocity, rotate, scale, collisionArea);
    }
}
