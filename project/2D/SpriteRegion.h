#pragma once

#include <vector>
#include <memory>
#include <random>
#include <string>
#include <wrl.h>
#include <d3d12.h>

#include "math/Transform.h"
#include "2D/Sprite.h"
#include "function/Math.h"

class Camera;
class DirectXCommon;
class DrawManager;
class DescriptorAllocator;

// 単一Spriteを使い回して大量描画するためのバッチャ（GPUインスタンシング対応）
class SpriteRegion {
public:
    SpriteRegion() = default;
    ~SpriteRegion() = default;

    // 初期化：共有のSpriteを1つだけ作る（テクスチャ名を固定化）
    void Initialize(Camera* camera, const std::string& textureName = "resources/uvChecker.png");

    // パーティクルライクな管理（CPU 更新）
    void Update(float dt);

    // 単純にインスタンスを追加（Transform.scale は px 単位で幅/高さ）
    void AddInstance(const Transform& t);

    // 大量演出（明滅→シャード）の発生ヘルパ
    void EmitSquareWave(
        const Vector2& center,
        int cols, int rows,
        float cellSizePx,
        float spacingPx,
        float brightDuration = 0.18f,
        int shardsPerCell = 6,
        float shardSizePx = 4.0f,
        float shardLifetime = 0.6f,
        float waveSpeedPxPerSec = 1200.0f
    );

    // 全インスタンス削除
    void ClearInstances();

    // インスタンシングバッファ更新（必要時だけ再構築/更新）
    void BuildInstanceBuffer(bool force = false);

    // 描画（GPUインスタンシング）。CPUフォールバック描画も可能。
    void Draw(bool debugUpdate = false, bool useCpuFallback = false);

    // 共有設定（全インスタンスに適用）
    void SetAnchor(float ax, float ay) { if (sprite_) sprite_->SetAnchor(ax, ay); }
    void SetFlip(bool fx, bool fy) { if (sprite_) sprite_->SetFlip(fx, fy); }
    void SetColor(const Vector4& color) { if (sprite_) sprite_->SetColor(color); }
    bool SetTextureRectPixels(int x, int y, int w, int h, bool autoResize = false) {
        return sprite_ ? sprite_->SetTextureRectPixels(x, y, w, h, autoResize) : false;
    }
    void ClearTextureRect() { if (sprite_) sprite_->ClearTextureRect(); }

    // 統計
    size_t GetInstanceCount() const { return activeCount_; }

    // --- DrawManager から参照する Getter 群 ---
    D3D12ResourceUtil* GetSpriteResource() const { return sprite_ ? sprite_->GetD3D12Resource() : nullptr; }
    D3D12_GPU_DESCRIPTOR_HANDLE   GetInstancingSrvHandleGPU() const { return instancingSrvGPU_; }
    UINT                          GetIndexCount() const; // スプライトのインデックス数
    UINT                          GetInstanceCountU32() const { return static_cast<UINT>(activeCount_); }

    // 依存注入
    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetSrvAllocator(DescriptorAllocator* alloc) { s_srvAllocator_ = alloc; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }

private:
    struct Particle {
        Transform transform;       // translate = 位置, scale = 幅/高さ（px）
        Vector2 velocity{ 0.0f, 0.0f }; // px/sec
        float rotationSpeed = 0.0f;     // rad/sec (Zのみ)
        Vector4 color{ 1.0f,1.0f,1.0f,1.0f };
        Vector2 startScale{ 1.0f, 1.0f };
        Vector2 endScale{ 1.0f, 1.0f };
        Vector4 startColor{ 1.0f,1.0f,1.0f,1.0f };
        Vector4 endColor{ 1.0f,1.0f,1.0f,0.0f };
        float life = 1.0f;
        float age = 0.0f;
        float delay = 0.0f;
        bool  active = false;
        bool  emitShardsOnDeath = false;
        bool  shardsSpawned = false;
        int   shardsCount = 0;
        float shardSize = 0.0f;
        float shardLifetime = 0.0f;
    };

    // VS 側が読む1インスタンス分のデータ（行列は列優先/プロジェクトのHLSL準拠で）
    struct InstanceData {
        Matrix4x4 WVP;   // CPUで計算したWVP（VSでそのまま使う）
        Vector4   color; // 乗算用カラー
        // 将来の拡張用（UV矩形/追加パラメータなど）はここに追加
    };

    void SpawnShardsAt(const Vector3& center, int count, float sizePx, float lifetime);

    // GPUバッファの生成/リサイズ
    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);
    void EnsureInstancingSRV();

private:
    // 静的依存
    static DirectXCommon* dx_;
    static DrawManager* drawManager_;
    static DescriptorAllocator* s_srvAllocator_;

    Camera* camera_ = nullptr;
    std::unique_ptr<Sprite> sprite_ = nullptr;  // 共有Sprite（リソース共有の要）

    std::vector<Particle> particles_;           // 管理するインスタンス群
    size_t activeCount_ = 0;                    // 今回描画する有効インスタンス数

    // プール制限（必要に応じて増やす）
    size_t maxParticles_ = 8192;

    // 乱数
    std::random_device rndDevice_;
    std::mt19937 randomEngine_{ rndDevice_() };

    // インスタンシング用 StructuredBuffer と SRV
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    InstanceData* instanceData_ = nullptr;
    uint32_t                               instanceCapacity_ = 0;

    uint32_t                               instancingSrvIndex_ = UINT32_MAX;
    D3D12_CPU_DESCRIPTOR_HANDLE            instancingSrvCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE            instancingSrvGPU_{};
};