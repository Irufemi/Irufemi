#pragma once

#include "Irufemi.h"
#include <vector>
#include <memory>
#include <string>

#include "Renderer/LineInstanced/LineClass.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Geometry/OBB.h"
#include "Resource/Audio/Se.h"

class Camera;
class IrufemiEngine;
class ModelRegion;
class VoxelParticleSystem;
class ParticleSystem;

/// @brief 個別建物のインスタンスデータ
struct BuildingInstance {
    Vector3 position  = {};
    Vector3 scale     = {};
    Vector3 rotate    = {};
    int hp            = 0;
    int floorCount    = 1;  // 階層数

    // 吹き飛び・消滅
    bool isBlownAway       = false;
    bool isDestroyed       = false;  // 完全に消滅済み
    bool hasExploded       = false;  // ボクセル爆散済み
    Vector3 blowVelocity   = {};
    Vector3 angularVelocity = {};
    float disappearTimer   = 0.0f;

    // 出現中演出パラメータ
    bool isSpawning        = false;
    float spawnTimer       = 0.0f;
    float spawnDuration    = 2.0f;
    Vector3 targetPosition = {};
    float initialY         = 0.0f;

    static constexpr float kDisappearTime = 3.0f;
    static constexpr float kFieldBound    = 100.0f;
};

class Building {
public:
    // パラメータ
    struct Parameters {
        int count = 10;
        int minFloors = 5;      // ビルの最小階層数
        int maxFloors = 20;     // ビルの最大階層数
        float floorHeightRatio = 0.5f; // 1階層の高さ = scaleXZ * この比率
        float minScaleXZ = 5.0f;
        float maxScaleXZ = 15.0f;
        float fieldRange = 90.0f;
        float minDistance = 5.0f;
        int buildingHp = 100;

        // 自動生成パラメータ
        float spawnInterval = 5.0f;
        int maxCount = 20;
        float avoidPlayerRadius = 15.0f;
        float avoidBossRadius = 30.0f;
        float spawnSpeed = 10.0f;

        // 吹き飛び（散弾）物理パラメータ
        float blowGravity = 0.03f;
        float blowBounceY = -0.4f;
        float blowFrictionXZ = 0.9f;
        float blowAngularFriction = 0.8f;
        float scatterSpread = 0.8f;
        float scatterUpForceBase = 0.1f;
        float scatterUpForceRand = 0.3f;
        float scatterSpeedBase = 1.2f;
        float scatterSpeedRand = 1.5f;
        float scatterAngularVelocity = 0.6f;
    };

    Building();
    ~Building();

    void Initialize(IrufemiEngine* engine);
    void Update();
    void Draw(IrufemiEngine* engine);
    void DrawImGui();

    /// @brief パラメータを取得
    const Parameters& GetParams() const { return params_; }

    /// @brief 既存の建物を全て消去し、指定座標に建物を1つだけ配置する（チュートリアル用）
    void ClearAndAddSingleBuilding(const Vector3& position);

    /// @brief 既存の建物を全て消去する
    void ClearAllBuildings();

    // --- 当たり判定用 公開API ---

    /// @brief 建物の数を取得
    int GetBuildingCount() const { return static_cast<int>(instances_.size()); }

    /// @brief 生存している建物の数を取得
    int GetAliveBuildingCount() const;

    // SEの一時停止・再開
    void PauseSe();
    void ResumeSe();

    /// @brief ランダムな位置に新しい建物を1つ生成する（プレイヤーやボスの位置を避ける）
    void SpawnRandomBuilding(const Vector3& avoidPlayerPos, const Vector3& avoidBossPos);

    /// @brief 建物がアクティブか（HP > 0 で吹き飛んでない＆消滅してない）
    bool IsBuildingAlive(int index) const;

    /// @brief 建物のOBBを取得
    OBB GetBuildingOBB(int index) const;

    /// @brief プレイヤーの押し戻し（Sphereとの判定）
    void ResolvePlayerCollision(Vector3& playerPos, float playerRadius);

    /// @brief 指定建物にダメージを与える。HPが0以下になったら吹き飛ぶ
    void ApplyDamage(int index, int damage, const Vector3& attackDir, float blowSpeed = 0.5f);

    /// @brief 指定建物を即破壊（敵接触や敵攻撃用）
    void DestroyAt(int index, const Vector3& attackDir, float blowSpeed = 0.8f);

    /**
     * @brief 衝突箇所におけるボクセルパーティクルの飛散表現を行う
     * @param index 飛散させる建物のインデックス
     * @param velocity パーティクルが飛散する基準となる速度（方向と強さ）
     * @param collisionArea 飛散させる領域を示すOBB（この範囲内のボクセルが飛散する）
     */
    void ScatterAt(int index, const Vector3& velocity, const OBB& collisionArea);

    /// @brief 建物が吹き飛び中か
    bool IsBuildingBlownAway(int index) const;

    /// @brief 建物が出現中か
    bool IsBuildingSpawning(int index) const;

    /// @brief 建物が完全に消滅済みか
    bool IsBuildingDestroyed(int index) const;

    /// @brief 吹き飛び中の建物の速度を取得
    Vector3 GetBlowVelocity(int index) const;

    /// @brief 吹き飛び中の建物の速度を設定
    void SetBlowVelocity(int index, const Vector3& v);

    /// @brief 吹き飛び中の建物の位置を取得
    Vector3 GetBuildingPosition(int index) const;

    /// @brief 飛んでいる建物を即消滅させる
    void MarkDestroyed(int index);

private:
    void ScatterBuildingFloors(int index, const Vector3& attackDir, float blowSpeed);

    void LoadJson();
    void SaveJson();
    void Generate();

private:

    IrufemiEngine* engine_ = nullptr;

    std::vector<BuildingInstance> instances_;
    std::unique_ptr<ModelRegion> buildingRegion_ = nullptr;

    // 砂煙エフェクト用 (ビル共通で1つだけ保持し、各ビルの位置から発生させる)
    std::unique_ptr<ParticleSystem> spawnDustSystem_;

    std::unique_ptr<Se> seCollapse_;

    // パラメータ
    Parameters params_;

    const std::string kJsonFilePath = "resources/Json/building/parameters.json";

#ifdef USE_IMGUI
    std::unique_ptr<Line3DRegion> debugLines_;
    bool isDebugDraw_ = false;
#endif
};
