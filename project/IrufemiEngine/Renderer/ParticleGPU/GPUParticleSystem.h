#pragma once

#include "../../Engine/Core/Math/Vector3.h"
#include "../../Engine/Core/Math/Vector4.h"
#include "../../Engine/Core/Math/Matrix4x4.h"
#include "../../Engine/Core/Type/PerFrame.h"
#include "../../Engine/Core/Type/PerView.h"
#include "../../Engine/Core/Type/PrimitiveType.h"
#include "../../Engine/Core/Type/BlendMode.h"
#include "../../Engine/Graphics/Pipeline/PSOManager.h"
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <random>
#include "../../Engine/Graphics/DirectX/ConstantBuffer.h"

// 前方宣言
class DrawManager;
class TextureManager;
class Camera;
class IrufemiEngine;
class Line3DRegion;

#include "../../Engine/Graphics/DirectX/DirectXCommon.h"


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

    // 拡張パラメータ
    Vector3 rotation;     ///< 回転角
    Vector3 rotateSpeed;  ///< 回転速度
    Vector3 startScale;   ///< 開始スケール
    Vector3 endScale;     ///< 終了スケール
    Vector4 startColor;   ///< 開始色
    Vector4 endColor;     ///< 終了色
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
    // float4 x 1
    uint32_t type = 0;          ///< 0: Sphere, 1: Beam, 2: Ring, 3: Cylinder
    Vector3 translate = {0,0,0}; ///< 放出中心位置

    // float4 x 2
    int32_t count = 0;          ///< 1回の放出数
    float frequency = 0.1f;     ///< 放出間隔
    float frequencyTime = 0.0f; ///< 放出タイマー
    int32_t emit = 0;           ///< 1: 放出許可, 0: 停止

    // float4 x 3
    float radius = 1.0f;        ///< Sphere/Ring/Cylinder用: 半径
    Vector3 direction = {0,0,1}; ///< Beam用: 方向

    // float4 x 4
    float spread = 0.1f;        ///< Beam用: 広がり
    float velocity = 1.0f;      ///< Beam用: 速度
    float minLife = 1.0f;       ///< 最小寿命
    float maxLife = 1.0f;       ///< 最大寿命

    // float4 x 5
    Vector3 startScaleMin = {1.0f, 1.0f, 1.0f}; ///< 開始スケール最小
    float pad0;
    // float4 x 6
    Vector3 startScaleMax = {1.0f, 1.0f, 1.0f}; ///< 開始スケール最大
    float pad1;
    // float4 x 7
    Vector3 endScaleMin = {1.0f, 1.0f, 1.0f};   ///< 終了スケール最小
    float pad2;
    // float4 x 8
    Vector3 endScaleMax = {1.0f, 1.0f, 1.0f};   ///< 終了スケール最大
    float pad3;

    // float4 x 9
    Vector4 startColorMin = {1.0f, 1.0f, 1.0f, 1.0f}; ///< 開始色最小
    // float4 x 10
    Vector4 startColorMax = {1.0f, 1.0f, 1.0f, 1.0f}; ///< 開始色最大
    // float4 x 11
    Vector4 endColorMin = {1.0f, 1.0f, 1.0f, 1.0f};   ///< 終了色最小
    // float4 x 12
    Vector4 endColorMax = {1.0f, 1.0f, 1.0f, 1.0f};   ///< 終了色最大

    // float4 x 13
    uint32_t colorMode = 0;     ///< カラーモード
    float gravity = 0.0f;       ///< 重力
    float damping = 0.0f;       ///< 空気抵抗
    uint32_t isBillboard = 1;   ///< ビルボードフラグ

    // float4 x 14
    uint32_t burstCount;
    float jitter;
    uint32_t atlasRows;
    uint32_t atlasCols;

    // float4 x 15
    float groundHeight;
    float bounce;
    float attractorStrength;
    uint32_t pad4;

    // float4 x 16
    Vector3 attractorPos;
    uint32_t pad5;

    // float4 x 17
    Vector3 areaSize;
    uint32_t pad6;
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

    /** @name 再生制御 */
    ///@{
    void Play() { isPlaying_ = true; if (emitter_) emitter_->emit = 1; }
    void Stop() { isPlaying_ = false; if (emitter_) emitter_->emit = 0; }
    void Pause() { isPlaying_ = false; }
    void Resume() { isPlaying_ = true; }
    void Clear();

    void SetLoop(bool loop) { isLooping_ = loop; }
    void SetDuration(float duration) { duration_ = duration; }
    void Emit(uint32_t count);
    ///@}

    /** @name 汎用エミッター設定 */
    ///@{
    /** @brief 粒子の発生ON/OFF */
    void SetEmit(bool emit);
    /** @brief パーティクルの基本色を設定 */
    void SetColor(const Vector4& color) { cpuMaterialData_.color = color; }
    /** @brief 3Dメッシュ形状を設定 */
    void SetPrimitive(PrimitiveType type);
    /** @brief ビルボードのON/OFF */
    void SetBillboard(bool isBillboard);
    /** @brief 使用するテクスチャを切り替える */
    void SetTexture(const std::string& textureFilePath);

    /**
     * @brief パーティクルのスケール（開始時と終了時）を動的に設定する
     * @param startMin 開始時の最小スケール
     * @param startMax 開始時の最大スケール
     * @param endMin 終了時の最小スケール
     * @param endMax 終了時の最大スケール
     */
    void SetParticleScale(const Vector3& startMin, const Vector3& startMax, const Vector3& endMin, const Vector3& endMax);

    /**
     * @brief パーティクルの寿命（最小・最大）を設定する
     * @param minLife 最小寿命（秒）
     * @param maxLife 最大寿命（秒）
     */
    void SetParticleLife(float minLife, float maxLife);

    /** @name 描画設定（パイプライン） */
    ///@{
    void SetBlend(BlendMode blend) { selectedBlend_ = blend; }
    void SetDepthWrite(PSOManager::DepthWrite depth) { selectedDepth_ = depth; }
    void SetCull(PSOManager::CullMode cull) { selectedCull_ = cull; }
    ///@}
    ///@}

    /** @name タイプ別エミッター設定 */
    ///@{
    void SetSphereEmitter(const Vector3& pos, float radius, uint32_t count, float frequency);
    void SetBeamEmitter(const Vector3& pos, const Vector3& direction, float radius, float velocity, float spread, uint32_t count, float frequency);

    /** @name アトラス・物理挙動設定 */
    /** @brief テクスチャアトラス（Flipbook）設定 */
    void SetTextureAtlas(uint32_t rows, uint32_t cols);
    /** @brief 床衝突設定 */
    void SetGroundCollision(float height, float bounce);
    /** @brief アトラクター（引力源）設定 */
    void SetAttractor(const Vector3& pos, float strength);

    /**
     * @brief リングエミッターの設定
     * @param pos 中心位置
     * @param radius 半径
     * @param thickness 厚み
     * @param count 一度の放出数
     * @param frequency 放出頻度（秒）
     */
    void SetRingEmitter(const Vector3& pos, float radius, float thickness, uint32_t count, float frequency);

    /**
     * @brief 円柱エミッターの設定
     * @param pos 中心位置
     * @param direction 円柱の方向
     * @param radius 半径
     * @param height 高さ
     * @param count 一度の放出数
     * @param frequency 放出頻度（秒）
     */
    void SetCylinderEmitter(const Vector3& pos, const Vector3& direction, float radius, float height, uint32_t count, float frequency);

    /**
     * @brief ボックス（直方体）エミッターの設定
     * @param pos 中心位置
     * @param size ボックスの各軸のサイズ（幅、高さ、奥行き）
     * @param count 一度の放出数
     * @param frequency 放出頻度（秒）
     */
    void SetBoxEmitter(const Vector3& pos, const Vector3& size, uint32_t count, float frequency);
    ///@}

    /** @name 静的マネージャ設定 */
    ///@{
    static void SetDXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    static void SetDrawManager(DrawManager* drawManager) { drawManager_ = drawManager; }
    static void SetTextureManager(TextureManager* textureManager) { textureManager_ = textureManager; }
    static void SetEngine(IrufemiEngine* engine) { engine_ = engine; }
    ///@}

private:
    void DrawAABB(const Vector3& min, const Vector3& max, const Vector4& color);
    void DrawCircle(const Vector3& center, float radius, const Vector3& axis, const Vector4& color);
    void DrawSphereWireframe(const Vector3& center, float radius, const Vector4& color);
    void DrawCylinderWireframe(const Vector3& center, const Vector3& direction, float radius, float height, const Vector4& color);

    /**
     * @brief DirectX12の各種リソースとディスクリプタ（SRV/UAV）の生成を行う
     */
    void CreateBuffersAndViews();

    /**
     * @brief コンピュートシェーダ（初期化、更新、放出）を実行する
     * @param commandList D3D12コマンドリスト
     */
    void DispatchComputeShaders(ID3D12GraphicsCommandList* commandList);

    /** @name ImGuiデバッグ用描画分割 */
    ///@{
    void DebugGeneralSettings();
    void DebugEmitterSettings();
    void DebugShapeSettings();
    void DebugPhysicsSettings();
    ///@}

    static DirectXCommon* dxCommon_;
    static DrawManager* drawManager_;
    static TextureManager* textureManager_;
    static IrufemiEngine* engine_;

    static const uint32_t kMaxParticles = 32768;

    /** @name 再生状態フラグ */
    bool isPlaying_ = true;
    bool isLooping_ = true;
    float duration_ = -1.0f; // -1: 無限
    float totalTime_ = 0.0f;

    /** @name エミッターリソース */
    ///@{
    GPUParticleEmitter emitterData_{};
    GPUParticleEmitter* emitter_ = &emitterData_; // CPU側のマスターへのポインタ
    ConstantBuffer<GPUParticleEmitter> emitterBuffer_;
    D3D12_CPU_DESCRIPTOR_HANDLE emitterSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE emitterSrvHandleGPU_{};
    ///@}

    /** @name 各種リソース */
    ///@{
    PerFrame perFrameDataStruct_{};
    PerFrame* perFrameData_ = &perFrameDataStruct_; // CPU側のマスターへのポインタ
    ConstantBuffer<PerFrame> perFrameBuffer_;
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

    ConstantBuffer<PerView> perViewBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{}; // NEW: インデックスバッファ
    uint32_t indexCount_ = 0;                  // NEW: インデックス数
    PrimitiveType primitiveType_ = PrimitiveType::Plane; // 現在の形状

    ConstantBuffer<ParticleGPUMaterial> materialBuffer_;
    ParticleGPUMaterial cpuMaterialData_{ { 1.0f, 1.0f, 1.0f, 1.0f }, 0, {0, 0, 0}, {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1} };

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};
    int selectedTextureIndex_ = 0;
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

    BlendMode selectedBlend_ = BlendMode::kBlendModeAdd;
    PSOManager::DepthWrite selectedDepth_ = PSOManager::DepthWrite::Disable;
    PSOManager::CullMode selectedCull_ = PSOManager::CullMode::None;

    std::unique_ptr<Line3DRegion> debugLineRegion_;
    bool showEmitterArea_ = true;
};
