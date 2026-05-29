#include "VoxelParticleManager.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"

void VoxelParticleManager::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
}

void VoxelParticleManager::ReservePool(const std::string& modelName, const Vector3Int& resolution, int poolSize) {
    // 現在プール内に存在する同一モデルの数をカウント
    int currentCount = 0;
    for (const auto& entry : pool_) {
        if (entry.modelName == modelName) {
            currentCount++;
        }
    }

    // 不足している分だけを新規に生成して追加
    int addCount = poolSize - currentCount;
    for (int i = 0; i < addCount; ++i) {
        auto voxel = std::make_unique<VoxelParticleSystem>();
        voxel->Initialize(modelName, resolution);
        // バックグラウンドロードを行いつつプールに追加
        pool_.push_back({ modelName, std::move(voxel) });
    }
}

VoxelParticleSystem* VoxelParticleManager::AllocateSystem(const std::string& modelName) {
    // 1. 最優先：ロード完了しており、かつ非アクティブな（暇な）システムを探す
    for (auto& entry : pool_) {
        if (entry.modelName == modelName && 
            entry.system->GetStatus() == VoxelParticleSystem::LoadingStatus::Loaded && 
            !entry.system->IsActive()) {
            return entry.system.get();
        }
    }

    // 2. 準優先（すべて使用中の場合）：ロード完了しているアクティブなシステムの中から、
    // 最も長く再生されている（残り寿命が短い = GetEmitterTime() が最も大きい）ものを上書き再利用する
    VoxelParticleSystem* oldestSystem = nullptr;
    float maxTime = -1.0f;
    for (auto& entry : pool_) {
        if (entry.modelName == modelName && 
            entry.system->GetStatus() == VoxelParticleSystem::LoadingStatus::Loaded) {
            float t = entry.system->GetEmitterTime();
            if (t > maxTime) {
                maxTime = t;
                oldestSystem = entry.system.get();
            }
        }
    }
    if (oldestSystem) {
        return oldestSystem;
    }

    // 3. フォールバック：ロード完了しているものが1つも存在しない場合のみ、安全上限（総数60）を越えない範囲で新規生成を許可
    if (pool_.size() < 60) {
        auto voxel = std::make_unique<VoxelParticleSystem>();
        voxel->Initialize(modelName, {32, 32, 32}); 
        pool_.push_back({ modelName, std::move(voxel) });
        return pool_.back().system.get();
    }
    
    // 安全上限に達しており、かつロード完了したものがない場合は、nullptr を返して発生を諦める（安全対策）
    return nullptr;
}

void VoxelParticleManager::Update(float deltaTime) {
    for (auto& entry : pool_) {
        // ロード未完了のもの（Pending, Loading）、描画準備完了状態、または実行中の場合にUpdateを呼ぶ
        auto status = entry.system->GetStatus();
        if (entry.system->IsActive() || 
            status == VoxelParticleSystem::LoadingStatus::Pending ||
            status == VoxelParticleSystem::LoadingStatus::Loading ||
            status == VoxelParticleSystem::LoadingStatus::ReadyToCreateResources) {
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
