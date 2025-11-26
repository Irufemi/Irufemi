#pragma once

#include "math/shape/AABB.h"
#include "math/Transform.h"
#include "math/Matrix4x4.h"
#include "3D/ObjClass.h"
#include <memory>
#include <cstdint>

class Camera;
class Player;
class GameScene; // GameSceneへの前方宣言を追加

class ShieldEnemy {
public:
    // 左右の向き
    enum class LRDirection {
        kRight, // 右
        kLeft,  // 左
    };

    // 振る舞いの種類
    enum class Behavior {
        kWalk,  // 歩行
        kDeath, // 死亡
        kUnknown, // 不明
    };

public: // メンバ関数
    // 初期化
    void Initialize(const Vector3& position, GameScene* gameScene);
    // 更新
    void Update();
    // 描画
    void Draw();

    // 衝突時に呼ばれる関数
    void OnCollision(const Player* player);

    // ワールド座標を取得
    Vector3 GetWorldPosition() const;
    // 当たり判定(AABB)の取得
    AABB GetAABB() const;

    // 死亡しているか
    bool IsDead() const { return isDead_; }
    // 衝突を無効にするか
    bool IsCollisionDisabled() const { return isCollisionDisabled_; }

    // カメラを設定
    static void SetCamera(Camera* camera) { camera_ = camera; }
    // Playerを設定
    static void SetPlayer(Player* player) { player_ = player; }

private: // 内部処理
    // 行列の更新
    void UpdateMatrix();

    // 各振る舞いの処理
    void BehaviorWalkInitialize();
    void BehaviorWalkUpdate();
    void BehaviorDeathInitialize();
    void BehaviorDeathUpdate();

private: // メンバ変数(ゲームシステム)
    // Transform(拡縮、回転、位置)
    Transform transform_{};
    // worldMatrix
    Matrix4x4 worldMatrix_{};

    // 速度
    Vector3 velocity_{};

    // 向き
    LRDirection lrDirection_ = LRDirection::kLeft;

    // 振る舞い
    Behavior behavior_ = Behavior::kWalk;
    Behavior behaviorRequest_ = Behavior::kUnknown;

    // 当たり判定サイズ
    static inline const float kWidth = 1.2f;
    static inline const float kHeight = 1.0f;

    // ダメージ軽減フラグ
    bool isDamageReduction = false;
    // デスフラグ
    bool isDead_ = false;
    // 衝突無効化フラグ
    bool isCollisionDisabled_ = false;

    // デス演出用タイマー
    float deathTimer_ = 0.0f;
    static inline const float kDeathDuration = 0.6f;
    Vector3 deathStartRotation_{};
    Vector3 deathEndRotation_{};

    // カメラ(ポインタ参照)
    static Camera* camera_;
    // Player(ポインタ参照)
    static Player* player_;
    // ゲームシーン(ポインタ参照)
    GameScene* gameScene_ = nullptr;

private: // メンバ変数(描画)
    // Obj
    std::unique_ptr<ObjClass> model_ = nullptr;
};

