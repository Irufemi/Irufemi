#pragma once

#include "VoxelParticleSystem.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include "Engine/Core/Math/Vector3Int.h"

class IrufemiEngine;
namespace Irufemi { struct OBB; }

class VoxelParticleManager {
public:
    VoxelParticleManager() = default;
    ~VoxelParticleManager() = default;

    void Initialize(IrufemiEngine* engine);
    void Update(float deltaTime);
    void Draw();
    void Clear();

    struct EmitterHandle {
        VoxelParticleSystem* system = nullptr;
        uint32_t emitterIndex = 0xFFFFFFFF;
        bool IsValid() const { return system != nullptr && emitterIndex != 0xFFFFFFFF; }
    };

    /**
     * @brief 指定したモデルと解像度に対するシステムを取得し、エミッター（インスタンス）を登録する
     */
    EmitterHandle RegisterEmitter(const std::string& modelName, const Irufemi::Vector3Int& resolution);

    /**
     * @brief 登録したエミッターを解放する
     */
    void UnregisterEmitter(const EmitterHandle& handle);

    /**
     * @brief エミッターデータを更新する
     */
    void UpdateEmitterData(const EmitterHandle& handle, const VoxelEmitter& data);
    const VoxelEmitter& GetEmitterData(const EmitterHandle& handle) const;

    /**
     * @brief 事前に指定された数のパーティクルシステムをロード・確保する
     */
    void ReservePool(const std::string& modelName, const Irufemi::Vector3Int& resolution, int preAllocateCount = 1000);

    /**
     * @brief その場での爆発エフェクトを発生させる
     */
    void PlayExplosion(const std::string& modelName, const Irufemi::Vector3& worldPos, const Irufemi::Vector3& velocity, const Irufemi::Vector3& rotate, const Irufemi::Vector3& scale, const VoxelEmitter& params, const Irufemi::Vector3Int& resolution);

private:
    VoxelParticleManager(const VoxelParticleManager&) = delete;
    VoxelParticleManager& operator=(const VoxelParticleManager&) = delete;

    struct SystemKey {
        std::string modelName;
        Irufemi::Vector3Int resolution;

        bool operator==(const SystemKey& other) const {
            return modelName == other.modelName && 
                   resolution.x == other.resolution.x &&
                   resolution.y == other.resolution.y &&
                   resolution.z == other.resolution.z;
        }
    };

    struct OneShotEmitter {
        EmitterHandle handle;
        float emitTimer;
        float lifeTimer;
    };

    struct SystemKeyHasher {
        std::size_t operator()(const SystemKey& k) const {
            std::size_t h1 = std::hash<std::string>()(k.modelName);
            std::size_t h2 = std::hash<int>()(k.resolution.x);
            std::size_t h3 = std::hash<int>()(k.resolution.y);
            std::size_t h4 = std::hash<int>()(k.resolution.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };

    struct SystemContext {
        std::unique_ptr<VoxelParticleSystem> system;
        std::vector<uint32_t> freeIndices;
        uint32_t nextIndex = 0;
    };

    std::unordered_map<SystemKey, SystemContext, SystemKeyHasher> systems_;
    std::vector<OneShotEmitter> oneShots_;
    IrufemiEngine* engine_ = nullptr;
};
