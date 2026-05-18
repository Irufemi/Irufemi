#pragma once

#include "Irufemi.h"
#include <vector>
#include <memory>
#include <string>

#include "Renderer/LineInstanced/LineClass.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Geometry/OBB.h"

class Camera;
class IrufemiEngine;
class ModelRegion;
class VoxelParticleSystem;

/// @brief 個別建物のインスタンスデータ
struct BuildingInstance {
    std::unique_ptr<VoxelParticleSystem> voxelSystem;
    Vector3 position  = {};
    Vector3 scale     = {};
    Vector3 rotate    = {};
    int hp            = 0;

    // 吹き飛び・消滅
    bool isBlownAway       = false;
    bool isDestroyed       = false;  // 完全に消滅済み
    bool hasExploded       = false;  // ボクセル爆散済み
    Vector3 blowVelocity   = {};
    Vector3 angularVelocity = {};
    float disappearTimer   = 0.0f;

    static constexpr float kDisappearTime = 3.0f;
    static constexpr float kFieldBound    = 100.0f;
};

class Building {
public:
    Building();
    ~Building();

    void Initialize(IrufemiEngine* engine);
    void Update();
    void Draw(IrufemiEngine* engine);
    void DrawImGui();

    /// @brief 既存の建物を全て消去し、指定座標に建物を1つだけ配置する（チュートリアル用）
    void ClearAndAddSingleBuilding(const Vector3& position);

    /// @brief 既存の建物を全て消去する
    void ClearAllBuildings();

    // --- 当たり判定用 公開API ---

    /// @brief 建物の数を取得
    int GetBuildingCount() const { return static_cast<int>(instances_.size()); }

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
    void LoadJson();
    void SaveJson();
    void Generate();

private:

    IrufemiEngine* engine_ = nullptr;

    std::vector<BuildingInstance> instances_;
    std::unique_ptr<ModelRegion> buildingRegion_ = nullptr;

    // パラメータ
    struct Parameters {
        int count = 10;
        float minHeight = 10.0f;
        float maxHeight = 50.0f;
        float minScaleXZ = 5.0f;
        float maxScaleXZ = 15.0f;
        float fieldRange = 90.0f;
        float minDistance = 5.0f;
        int buildingHp = 100;
    } params_;

    const std::string kJsonFilePath = "resources/Json/building/parameters.json";

#ifdef USE_IMGUI
    std::unique_ptr<Line3DRegion> debugLines_;
    bool isDebugDraw_ = false;
#endif
};
