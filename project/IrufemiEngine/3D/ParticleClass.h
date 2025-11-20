#pragma once

#include "math/ParticleType.h"
#include "source/D3D12ResourceUtil.h"
#include "Application/camera/Camera.h"
#include "manager/TextureManager.h"
#include "manager/DebugUI.h"
#include "math/shape/Particle.h"
#include "math/shape/ParticleForGPU.h"
#include "math/Emitter.h"
#include "math/AccelerationField.h"
#include "function/Math.h"
#include <wrl.h>
#include <memory>
#include <cstdint>
#include <numbers>
#include <list>
#include <random>

class DescriptorPool;

class ParticleClass {
private: // メンバ変数
    static inline const uint32_t kNumMaxInstance_ = 100;

    uint32_t numInstance_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_ = nullptr;
    ParticleForGPU* instancingData_ = nullptr;

    // SRV: アロケータで確保したスロット情報
    uint32_t                   instancingSrvIndex_ = UINT32_MAX; // 追加
    D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_{};

    std::list<Particle> particles_;
    std::unique_ptr<D3D12ResourceUtilParticle> resource_ = nullptr;

    Matrix4x4 backToFrontMatrix_ = Math::MakeRotateYMatrix({ 0 });
    Matrix4x4 billbordMatrix_{};

    int selectedTextureIndex_ = 0;

    static inline const float kDeltatime_ = 1.0f / 60.0f;

    std::random_device seedGenerator_;
    std::mt19937 randomEngine_;

    Emitter emitter_{};
    AccelerationField accelerationField_{};

    // 参照
    Camera*         camera_ = nullptr;

    bool useBillbord_ = true;
    bool isUpdate_ = true;

    // 共有リソース
    static DescriptorPool* s_srvPool_;
    static TextureManager* s_textureManager_;
    static DebugUI*        s_ui_;

    ParticleType particleType_ = ParticleType::kAccelerationField;

public:
    ~ParticleClass(); // 追加

    void Initialize(Camera* camera, const std::string& textureName = "resources/circle.png", ParticleType type = ParticleType::kAccelerationField);
    void Update();
    void Draw();
    void Debug(const char* particleName = "");

    Particle MakeNewParticle(std::mt19937& randomEngine, const Emitter& emitter);
    std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randomEngine);

    /// <summary>
    /// ヒットエフェクトを再生します
    /// </summary>
    /// <param name="position">発生位置</param>
    void PlayHitEffect(const Vector3& position);

    D3D12ResourceUtilParticle* GetD3D12Resource() { return this->resource_.get(); }
    int32_t GetInstanceCount() const { return this->numInstance_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvHandleGPU_; }

    // フィールド設定
    void SetAccelerationField(const Vector3& center, const Vector3& size, const Vector3& acceleration);

    // エミッタ設定
    void SetEmitterPosition(const Vector3& position);
    void SetEmitterArea(const Vector3& area);
    void SetEmitterVelocity(const Vector3& minVel, const Vector3& maxVel);
    void SetEmitterFrequency(float frequency);
    void SetEmitterCount(uint32_t count);

    /// <summary>
    /// エミッタのプロパティをまとめて設定します。
    /// </summary>
    /// <param name="position">パーティクルの発生中心位置</param>
    /// <param name="area">パーティクルの発生範囲のサイズ</param>
    /// <param name="minVel">パーティクルの初速の最小値</param>
    /// <param name="maxVel">パーティクルの初速の最大値</param>
    /// <param name="frequency">パーティクルの生成間隔（秒）</param>
    /// <param name="count">一度に生成するパーティクルの数</param>
    void SetEmitterProperties(
        const Vector3& position,
        const Vector3& area,
        const Vector3& minVel,
        const Vector3& maxVel,
        float frequency,
        uint32_t count);

    /// <summary>
    /// パーティクルのテクスチャを設定します。
    /// </summary>
    /// <param name="textureFilePath">テクスチャのファイルパス</param>
    /// <returns>テクスチャが見つかり、設定に成功した場合はtrue</returns>
    void SetTexture(const std::string& textureFilePath);

    // 共有リソース注入
    static void SetSrvPool(DescriptorPool* pool) { s_srvPool_ = pool; }
    static void SetTextureManager(TextureManager* textureManager) { s_textureManager_ = textureManager; }
    static void SetDebugUI(DebugUI* ui) { s_ui_ = ui; }
};

