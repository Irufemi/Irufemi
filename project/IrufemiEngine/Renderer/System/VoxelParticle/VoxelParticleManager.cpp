#include "Renderer/System/VoxelParticle/VoxelParticleManager.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/Pipeline/PSOManager.h"

void VoxelParticleManager::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
}

VoxelParticleManager::EmitterHandle VoxelParticleManager::RegisterEmitter(const std::string& modelName, const Irufemi::Vector3Int& resolution) {
    SystemKey key{ modelName, resolution };
    auto& context = systems_[key];

    if (!context.system) {
        context.system = std::make_unique<VoxelParticleSystem>();
        VoxelParticleSystem::SetEngine(engine_);
        context.system->Initialize(modelName, resolution);
        context.nextIndex = 0;
    }

    uint32_t index = 0;
    if (!context.freeIndices.empty()) {
        index = context.freeIndices.back();
        context.freeIndices.pop_back();
    } else {
        if (context.nextIndex >= context.system->GetMaxInstances()) {
            return EmitterHandle{}; // 上限に達した場合は無効なハンドルを返す
        }
        index = context.nextIndex++;
    }

    EmitterHandle handle;
    handle.system = context.system.get();
    handle.emitterIndex = index;
    return handle;
}

void VoxelParticleManager::UnregisterEmitter(const EmitterHandle& handle) {
    if (!handle.IsValid()) return;
    
    // システムを検索してfreeIndicesに戻す
    for (auto& pair : systems_) {
        if (pair.second.system.get() == handle.system) {
            pair.second.freeIndices.push_back(handle.emitterIndex);
            
            // 無効化用のダミーデータを送る
            VoxelEmitter emptyData;
            emptyData.emit = 0;
            emptyData.lifeTime = 0.0f; // 即座に非表示判定にする
            handle.system->UpdateEmitterData(handle.emitterIndex, emptyData);
            break;
        }
    }
}

void VoxelParticleManager::Clear() {
    // 実行中のワンショットエミッターをすべて即座に解放
    for (auto& shot : oneShots_) {
        UnregisterEmitter(shot.handle);
    }
    oneShots_.clear();

    // 登録済みの全システムの状態をリセット
    for (auto& pair : systems_) {
        if (pair.second.system) {
            uint32_t maxInstances = pair.second.system->GetMaxInstances();
            for (uint32_t i = 0; i < maxInstances; ++i) {
                VoxelEmitter emptyData;
                emptyData.emit = 0;
                emptyData.lifeTime = 0.0f;
                pair.second.system->UpdateEmitterData(i, emptyData);
            }
            
            // インデックスの割り当て状態を完全に初期化する
            pair.second.freeIndices.clear();
            pair.second.nextIndex = 0;
        }
    }
}

void VoxelParticleManager::UpdateEmitterData(const EmitterHandle& handle, const VoxelEmitter& data) {
    if (!handle.IsValid()) return;
    handle.system->UpdateEmitterData(handle.emitterIndex, data);
}

const VoxelEmitter& VoxelParticleManager::GetEmitterData(const EmitterHandle& handle) const {
    if (!handle.IsValid()) {
        static VoxelEmitter dummy;
        return dummy;
    }
    return handle.system->GetEmitterData(handle.emitterIndex);
}

void VoxelParticleManager::Update(float deltaTime) {
    for (auto& pair : systems_) {
        if (pair.second.system) {
            pair.second.system->Update(deltaTime);
        }
    }

    for (auto it = oneShots_.begin(); it != oneShots_.end(); ) {
        if (it->emitTimer > 0.0f) {
            it->emitTimer -= deltaTime;
            if (it->emitTimer <= 0.0f) {
                VoxelEmitter data = GetEmitterData(it->handle);
                data.emit = 0;
                UpdateEmitterData(it->handle, data);
            }
        }
        
        it->lifeTimer -= deltaTime;
        if (it->lifeTimer <= 0.0f) {
            UnregisterEmitter(it->handle);
            it = oneShots_.erase(it);
        } else {
            ++it;
        }
    }
}

void VoxelParticleManager::Draw() {
    for (auto& pair : systems_) {
        if (pair.second.system) {
            engine_->SetBlend(Irufemi::BlendMode::kBlendModeNormal);
            engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
            engine_->SetCull(PSOManager::CullMode::Back);
            pair.second.system->Draw();
        }
    }
}

void VoxelParticleManager::ReservePool(const std::string& modelName, const Irufemi::Vector3Int& resolution, int preAllocateCount) {
    auto handle = RegisterEmitter(modelName, resolution);
    UnregisterEmitter(handle);
}

void VoxelParticleManager::PlayExplosion(const std::string& modelName, const Irufemi::Vector3& worldPos, const Irufemi::Vector3& velocity, const Irufemi::Vector3& rotate, const Irufemi::Vector3& scale, const VoxelEmitter& params, const Irufemi::Vector3Int& resolution) {
    auto handle = RegisterEmitter(modelName, resolution);
    if (!handle.IsValid()) return; // 制限オーバーで取得できなかった場合は処理しない

    VoxelEmitter explosion = params;
    explosion.emit = 1;
    explosion.emitPosition = worldPos;
    explosion.baseVelocity = velocity;
    explosion.rotate = rotate;
    explosion.scale = scale;
    UpdateEmitterData(handle, explosion);

    OneShotEmitter shot{};
    shot.handle = handle;
    shot.emitTimer = 0.1f;
    shot.lifeTimer = params.lifeTime + 0.5f;
    oneShots_.push_back(shot);
}
