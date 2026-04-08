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
 * @struct EmitterSphere
 * @brief 球体状のパーティクル放出器設定（CSで参照）
 */
struct EmitterSphere {
    Vector3 translate;  ///< 放出中心位置
    float radius;       ///< 放出半径
    uint32_t count;     ///< 1回の放出数
    float frequency;    ///< 放出間隔
    float frequencyTime; ///< 放出タイマー
    uint32_t emit;      ///< 1: 放出許可, 0: 停止
};

/**
 * @class GPUParticleSystem
 * @brief Compute Shader を使用した、大量の粒子を GPU 上でシミュレーション・描画するクラス
 * @details 粒子の生成、更新、描画の全工程を GPU 側で行うことで、
 *          CPU 側の負荷を抑えつつ数万〜数十万規模のパーティクル演出を可能にします。
 *          フリーリストによる効率的な粒子メモリ管理を行っています。
 */
class GPUParticleSystem
{
public:
    GPUParticleSystem();
    ~GPUParticleSystem();

    /** @name 初期化・更新・描画 */
    ///@{
    /**
     * @brief 初期化
     * @param[in] camera 描画用カメラ
     * @param[in] textureName 使用するテクスチャパス
     */
    void Initialize(Camera* camera, const std::string& textureName = "resources/circle.png");
    /** @brief CS による粒子更新・放出の実行 */
    void Update();
    /** @brief インスタンシング描画の実行 */
    void Draw();
    /** @brief DebugUI によるパラメータ調整 */
    void Debug();
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

    static const uint32_t kMaxParticles = 1024; ///< 最大同時生存数

    /** @name エミッターリソース（UAV/SRV） */
    ///@{
    EmitterSphere* emitterSphere_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> emitterSphereResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE emitterSphereUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE emitterSphereUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE emitterSphereSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE emitterSphereSrvHandleGPU_{};
    ///@}

    /** @name フレーム共有データ */
    ///@{
    PerFrame* perFrameData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE perFrameUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE perFrameUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE perFrameSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE perFrameSrvHandleGPU_{};
    ///@}

    /** @name 粒子メインデータ（RWStructuredBuffer） */
    ///@{
    ParticleCS* particleData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> particleResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE particleUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE particleUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE particleSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE particleSrvHandleGPU_{};
    ///@}

    /** @name フリーリスト管理（カウンタとインデックス） */
    ///@{
    int32_t* freeListIndex_ = nullptr; ///< 空きスロットの現在の数
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE freeListIndexUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListIndexUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE freeListIndexSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListIndexSrvHandleGPU_{};

    int32_t* freeList_ = nullptr; ///< 空きスロット番号の配列
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE freeListUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE freeListSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListSrvHandleGPU_{};
    ///@}

    /** @name 描画用リソース */
    ///@{
    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_;
    PerView* perViewData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    ParticleGPUMaterial* materialData_ = nullptr;

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};
    Camera* camera_ = nullptr;

    // デスクリプタインデックスの保持
    uint32_t emitterSrvIndex_ = 0xFFFFFFFF;
    uint32_t perFrameSrvIndex_ = 0xFFFFFFFF;
    uint32_t particleUavIndex_ = 0xFFFFFFFF;
    uint32_t particleSrvIndex_ = 0xFFFFFFFF;
    uint32_t freeListIndexUavIndex_ = 0xFFFFFFFF;
    uint32_t freeListUavIndex_ = 0xFFFFFFFF;
    ///@}

    bool isCullingEnabled_ = true;
    bool isCulled_ = false;
    bool needsUpdateCS_ = false; ///< CSによる更新が必要か（Update呼び出しに同期）
};

