#pragma once

#include "VoxelParticleSystem.h"
#include <vector>
#include <memory>
#include <string>

class IrufemiEngine;

class VoxelParticleManager {
public:
    VoxelParticleManager() = default;
    ~VoxelParticleManager() = default;

    void Initialize(IrufemiEngine* engine);
    void Update(float deltaTime);
    void Draw();

    // 事前に特定のモデル用プールを確保しておく
    void ReservePool(const std::string& modelName, const Vector3Int& resolution, int poolSize);

    // 空いているVoxelParticleSystemを取得して爆発を実行
    void PlayExplosion(const std::string& modelName, 
                       const Vector3& position, 
                       const Vector3& velocity, 
                       const Vector3& rotate, 
                       const Vector3& scale,
                       VoxelParticleSystem::ParticleType type = VoxelParticleSystem::ParticleType::Default);

    // 衝突時の部分飛散
    void PlayCollisionScatter(const std::string& modelName, 
                              const Vector3& position, 
                              const Vector3& velocity, 
                              const Vector3& rotate, 
                              const Vector3& scale, 
                              const struct OBB& collisionArea,
                              VoxelParticleSystem::ParticleType type = VoxelParticleSystem::ParticleType::Default);

private:
    struct PoolEntry {
        std::string modelName;
        std::unique_ptr<VoxelParticleSystem> system;
    };
    std::vector<PoolEntry> pool_;
    IrufemiEngine* engine_ = nullptr;

    VoxelParticleSystem* AllocateSystem(const std::string& modelName);
};
