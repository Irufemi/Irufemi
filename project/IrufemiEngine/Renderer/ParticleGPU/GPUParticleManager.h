#pragma once
#include "GPUParticleSystem.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

class GPUParticleManager {
public:
    static GPUParticleManager* GetInstance();

    void Initialize();
    void Update();
    void Draw();
    void Finalize();

    struct EmitterHandle {
        GPUParticleSystem* system = nullptr;
        uint32_t emitterIndex = 0xFFFFFFFF;
        bool IsValid() const { return system != nullptr && emitterIndex != 0xFFFFFFFF; }
    };

    /**
     * @brief 指定したテクスチャに対するパーティクルシステムを取得し、空きエミッターのインデックスを返す
     */
    EmitterHandle RegisterEmitter(const std::string& texturePath);

    /**
     * @brief 登録したエミッターを解放する
     */
    void UnregisterEmitter(const EmitterHandle& handle);

    /**
     * @brief エミッターデータを更新する
     */
    void UpdateEmitterData(const EmitterHandle& handle, const GPUParticleEmitter& data);

private:
    GPUParticleManager() = default;
    ~GPUParticleManager() = default;
    GPUParticleManager(const GPUParticleManager&) = delete;
    GPUParticleManager& operator=(const GPUParticleManager&) = delete;

    struct SystemContext {
        std::unique_ptr<GPUParticleSystem> system;
        std::vector<uint32_t> freeIndices;
        uint32_t nextIndex = 0;
    };

    std::unordered_map<std::string, SystemContext> systems_;
};
