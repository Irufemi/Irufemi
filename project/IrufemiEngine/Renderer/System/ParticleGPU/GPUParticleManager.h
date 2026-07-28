#pragma once
#include "GPUParticleSystem.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "../../../Engine/Core/Type/BlendMode.h"

class GPUParticleManager {
public:
    GPUParticleManager() = default;
    ~GPUParticleManager() = default;

    void Initialize();
    void Update();
    void Draw();
    void Finalize();
    void Debug();
    
    /**
     * @brief 現在GPU上に残っているすべてのパーティクルをクリアする（シーン遷移時などに使用）
     */
    void ClearAllParticles();

    struct EmitterHandle {
        GPUParticleSystem* system = nullptr;
        uint32_t emitterIndex = 0xFFFFFFFF;
        bool IsValid() const { return system != nullptr && emitterIndex != 0xFFFFFFFF; }
    };

    /**
     * @brief 指定したテクスチャ、ブレンドモード、タイムスケール設定に対するシステムを取得し、エミッターを登録する
     */
    EmitterHandle RegisterEmitter(const std::string& texturePath, BlendMode blendMode, bool isUnscaledTime, bool enableLighting);

    /**
     * @brief 登録したエミッターを解放する
     */
    void UnregisterEmitter(const EmitterHandle& handle);

    /**
     * @brief エミッターデータを更新する
     */
    void UpdateEmitterData(const EmitterHandle& handle, const GPUParticleEmitter& data);
    void SetMeshEmitterBuffer(EmitterHandle handle, D3D12_GPU_VIRTUAL_ADDRESS vbAddress);

    /** @name Field Management */
    ///@{
    struct FieldHandle {
        uint32_t index = 0xFFFFFFFF;
        bool IsValid() const { return index != 0xFFFFFFFF; }
    };
    FieldHandle RegisterField();
    void UnregisterField(const FieldHandle& handle);
    void UpdateFieldData(const FieldHandle& handle, const ParticleField& data);
    ///@}

private:
    GPUParticleManager(const GPUParticleManager&) = delete;
    GPUParticleManager& operator=(const GPUParticleManager&) = delete;

    struct SystemContext {
        std::unique_ptr<GPUParticleSystem> system;
        std::vector<uint32_t> freeIndices;
        uint32_t nextIndex = 0;
    };

    struct SystemKey {
        std::string texturePath;
        BlendMode blendMode;
        bool isUnscaledTime;
        bool enableLighting;

        bool operator==(const SystemKey& other) const {
            return texturePath == other.texturePath && 
                   blendMode == other.blendMode && 
                   isUnscaledTime == other.isUnscaledTime &&
                   enableLighting == other.enableLighting;
        }
    };

    struct SystemKeyHasher {
        std::size_t operator()(const SystemKey& k) const {
            std::size_t h1 = std::hash<std::string>()(k.texturePath);
            std::size_t h2 = std::hash<int>()(static_cast<int>(k.blendMode));
            std::size_t h3 = std::hash<bool>()(k.isUnscaledTime);
            std::size_t h4 = std::hash<bool>()(k.enableLighting);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };

    std::unordered_map<SystemKey, SystemContext, SystemKeyHasher> systems_;

    // グローバルなField管理
    std::vector<ParticleField> globalFields_;
    std::vector<uint32_t> freeFieldIndices_;
    uint32_t nextFieldIndex_ = 0;
};
