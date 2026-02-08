#pragma once

#include "../IEnemy.h"
#include <cstdint>

// 前方宣言
class Camera;
class GameScene;
class Player;

class NormalEnemy : public IEnemy {
public:
    // 振る舞いの種類
    enum class Behavior {
        kWalk,  // 歩行
        kDeath, // 死亡
        kUnknown, // 不明
    };

public: // メンバ関数
    // コンストラクタ
    NormalEnemy(GameScene* gameScene, Camera* camera);

    // 初期化
    void Initialize(const Vector3& position) override;
    // 更新
    void Update() override;
    // 描画
    void Draw() override;

    // 衝突時に呼ばれる関数
    void OnCollision(Player* player) override;

private: // 内部処理
    // 行列の更新
    void UpdateMatrix();

    // 各振る舞いの処理
    void BehaviorWalkInitialize();
    void BehaviorWalkUpdate();
    void BehaviorDeathInitialize();
    void BehaviorDeathUpdate();

private: // メンバ変数(ゲームシステム)
    // 速度
    Vector3 velocity_{};

    // 振る舞い
    Behavior behavior_ = Behavior::kWalk;
    Behavior behaviorRequest_ = Behavior::kUnknown;

    // 衝突無効化フラグ
    bool isCollisionDisabled_ = false;

    // デス演出用タイマー
    float deathTimer_ = 0.0f;
    static inline const float kDeathDuration = 0.6f;
    Vector3 deathStartRotation_{};
    Vector3 deathEndRotation_{};

    // ゲームシーン(ポインタ参照)
    GameScene* gameScene_ = nullptr;
    // カメラ(ポインタ参照)
    Camera* camera_ = nullptr;
};