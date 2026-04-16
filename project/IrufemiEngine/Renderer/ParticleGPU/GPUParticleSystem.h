#pragma once

#include "../../Engine/Core/Math/Vector3.h"
#include "../../Engine/Core/Math/Vector4.h"
#include "../../Engine/Core/Math/Matrix4x4.h"
#include "../../Engine/Core/Type/PerFrame.h"
#include "../../Engine/Core/Type/PerView.h"
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <random>

// 前方宣言
class DirectXCommon;
class DrawManager;
class TextureManager;
class Camera;
class IrufemiEngine;

/**
 * @struct ParticleCS
 * @brief コンピュートシェーダ（CS）側で扱う粒子データ構造体
 */
struct ParticleCS {
    Vector3 translate; ///< 位置
    Vector3 scale;     ///< スケール
    float lifeTime;    ///< 寿命（秒）
    Vector3 velocity;  ///< 速度
    float currentTime; ///< 現在の経過時間
    Vector4 color;     ///< 色
};

/**
 * @struct ParticleGPUMaterial
 * @brief GPUパーティクル描画用のマテリアル設定
 */
struct ParticleGPUMaterial {
    Vector4 color;             ///< 乗算色
    int32_t useClampSampler = 0; ///< 0: WRAP, 1: CLAMP
    float pad[3];
    Matrix4x4 uvTransform;     ///< UV変形行列
};

/**
 * @struct GPUParticleEmitter
 * @brief パーティクル放出器の設定（統合版）
 * @details HLSL側の GPUParticleEmitter と構造を一致させています。
 */
struct GPUParticleEmitter {
    uint32_t type = 0;          ///< 0: Sphere, 1: Beam
    Vector3 translate = {0,0,0}; ///< 放出中心位置
    int32_t count = 0;          ///< 1回の放出数
    float frequency = 0.1f;     ///< 放出間隔
    float frequencyTime = 0.0f; ///< 放出タイマー
    int32_t emit = 0;           ///< 1: 放出許可, 0: 停止

    // Type specific
    float radius = 1.0f;        ///< Sphere用: 半径
    Vector3 direction = {0,0,1}; ///< Beam用: 方向
    float spread = 0.1f;        ///< Beam用: 広がり
    float velocity = 1.0f;      ///< Beam用: 速度
    float pad[2] = {0,0};
};

/**
 * @class GPUParticleSystem
 * @brief Compute Shader を使用した、大量の粒子を GPU 上でシミュレーション・描画するクラス
 */
class GPUParticleSystem
{
public:
    GPUParticleSystem();
    ~GPUParticleSystem();

    /** @name 初期化・更新・描画 */
    ///@{
    void Initialize(Camera* camera, const std::string& textureName = "resources/circle.png");
    void Update();
    void Draw();
    void Debug();
    ///@}

    /** @name 汎用エミッター設定 */
    ///@{
    /** @brief 粒子の発生ON/OFF */
    void SetEmit(bool emit) { if (emitter_) emitter_->emit = emit ? 1 : 0; }
    /** @brief パーティクルの基本色を設定 */
    void SetColor(const Vector4& color) { if (materialData_) materialData_->color = color; }
    ///@}

    /** @name タイプ別エミッター設定 */
    ///@{
    /**
     * @brief 球体エミッターの設定
     * @param pos 中心位置
     * @param radius 半径
     * @param count 一度の放出数
     * @param frequency 放出頻度（秒）
     */
    void SetSphereEmitter(const Vector3& pos, float radius, uint32_t count, float frequency);

    /**
     * @brief ビームエミッターの設定
     * @param pos 発射位置
     * @param direction 発射方向
     * @param radius 発生半径（太さ）
     * @param velocity 粒子の速度
     * @param spread 広がり（0: 直線 ～ 1.0: 全方位への影響度）
     * @param count 一度の放出数
     * @param frequency 放出頻度（秒）
     */
    void SetBeamEmitter(const Vector3& pos, const Vector3& direction, float radius, float velocity, float spread, uint32_t count, float frequency);
    ///@}

    /** @name 静的マネージャ設定 */
    ///@{
    static void SetDXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    static void SetDrawManager(DrawManager* drawManager) { drawManager_ = drawManager; }
    static void SetTextureManager(TextureManager* textureManager) { textureManager_ = textureManager; }
    static void SetEngine(IrufemiEngine* engine) { engine_ = engine; }
    ///@}

private:
    static DirectXCommon* dxCommon_;
    static DrawManager* drawManager_;
    static TextureManager* textureManager_;
    static IrufemiEngine* engine_;

    static const uint32_t kMaxParticles = 32768;

    /** @name エミッターリソース */
    ///@{
    GPUParticleEmitter* emitter_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> emitterResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE emitterUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE emitterUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE emitterSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE emitterSrvHandleGPU_{};
    ///@}

    /** @name 各種リソース */
    ///@{
    PerFrame* perFrameData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE perFrameUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE perFrameUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE perFrameSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE perFrameSrvHandleGPU_{};

    ParticleCS* particleData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> particleResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE particleUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE particleUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE particleSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE particleSrvHandleGPU_{};

    int32_t* freeListIndex_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE freeListIndexUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListIndexUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE freeListIndexSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListIndexSrvHandleGPU_{};

    int32_t* freeList_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE freeListUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE freeListSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListSrvHandleGPU_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_;
    PerView* perViewData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    ParticleGPUMaterial* materialData_ = nullptr;

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};
    Camera* camera_ = nullptr;

    uint32_t emitterSrvIndex_ = 0xFFFFFFFF;
    uint32_t perFrameSrvIndex_ = 0xFFFFFFFF;
    uint32_t particleUavIndex_ = 0xFFFFFFFF;
    uint32_t particleSrvIndex_ = 0xFFFFFFFF;
    uint32_t freeListIndexUavIndex_ = 0xFFFFFFFF;
    uint32_t freeListUavIndex_ = 0xFFFFFFFF;
    ///@}

    bool isCullingEnabled_ = true;
    bool isCulled_ = false;
    bool isInitializedCS_ = false;
    bool needsUpdateCS_ = false;
};
