#pragma once
#include "../Component.h"
#include "../../../Renderer/System/VoxelParticle/VoxelParticleSystem.h"
#include "Engine/Core/Math/Vector3Int.h"
#include <string>

/**
 * @class VoxelParticleComponent
 * @brief 既存のレンダラーモデル等を利用してボクセルパーティクルを発生させるコンポーネント。
 *        シーンロード時に事前計算（ReservePool）を行い、再生遅延を防ぎます。
 */
class VoxelParticleComponent : public Component {
public:
    VoxelParticleComponent();
    ~VoxelParticleComponent() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    
    std::string GetComponentName() const override { return "VoxelParticleComponent"; }
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    const std::string& GetOverrideModelName() const { return overrideModelName_; }
    void SetOverrideModelName(const std::string& name) { overrideModelName_ = name; }

    Vector3Int GetResolution() const { return resolution_; }
    void SetResolution(const Vector3Int& res) { resolution_ = res; }

    int GetPreAllocateCount() const { return preAllocateCount_; }
    void SetPreAllocateCount(int count) { preAllocateCount_ = count; }

    VoxelParticleSystem::VoxelEmitterParams& GetEmitterParams() { return emitterParams_; }

    /**
     * @brief その場にパーティクルを放出します（実装保留・拡張用）
     */
    void Emit();

    /**
     * @brief 爆発的にパーティクルを四散させます。
     * @param velocity 初速
     * @param rotate 回転成分
     * @param scale スケール成分
     */
    void Explode(const Vector3& velocity = {0,0,0}, const Vector3& rotate = {0,0,0}, const Vector3& scale = {1,1,1});

private:
    // 詳細パラメータ
    VoxelParticleSystem::VoxelEmitterParams emitterParams_ = VoxelParticleSystem::VoxelEmitterParams::Default();
    
    // システム設定
    Vector3Int resolution_ = {32, 32, 32}; ///< ボクセルの分割数
    int preAllocateCount_ = 1;             ///< シーン開始時に確保・事前計算しておく数
    std::string overrideModelName_;        ///< 空欄ならMeshRenderer等を利用、指定があればプロキシモデルを利用

    // キャッシュ用
    std::string cachedModelName_;
    bool isInitialized_ = false;

    /**
     * @brief 使用する対象のモデル名を取得します（オーバーライド優先）
     */
    std::string GetTargetModelName();
};
