#pragma once

#include "../source/D3D12ResourceUtil.h"
#include "../camera/Camera.h"
#include "../manager/TextureManager.h"
#include "../manager/DebugUI.h"
#include "../math/shape/Particle.h"
#include "../math/shape/ParticleForGPU.h"
#include "../math/Emitter.h"
#include "../math/AccelerationField.h"
#include "../function/Math.h"
#include <wrl.h>
#include <memory>
#include <cstdint>
#include <numbers>
#include <list>
#include <random>

class DescriptorAllocator; // 追加

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
    TextureManager* textureManager_ = nullptr;
    DebugUI*        ui_ = nullptr;

    bool useBillbord_ = true;
    bool isUpdate_ = true;

    // 共有アロケータ
    static DescriptorAllocator* s_srvAllocator_; // 追加

public:
    ~ParticleClass(); // 追加

    void Initialize(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& srvDescriptorHeap, Camera* camera, TextureManager* textureManager, DebugUI* ui, const std::string& textureName = "resources/circle.png");
    void Update(const char* particleName = "");
    void Draw();

    Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);
    std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randomEngine);

    D3D12ResourceUtilParticle* GetD3D12Resource() { return this->resource_.get(); }
    int32_t GetInstanceCount() const { return this->numInstance_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvHandleGPU_; }

    // アロケータ注入
    static void SetSrvAllocator(DescriptorAllocator* alloc) { s_srvAllocator_ = alloc; } // 追加
};

