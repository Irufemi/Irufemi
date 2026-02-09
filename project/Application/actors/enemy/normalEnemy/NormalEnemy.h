#pragma once

#include "../IEnemy.h"
#include <cstdint>

// 前方宣言
class Camera;
class GameScene;
class Player;

/**
 * @class NormalEnemy
 * @brief 標準的な敵キャラクタークラス
 * @details IEnemyを継承し、左右に移動するだけのシンプルな振る舞いを実装します。
 *          プレイヤーの攻撃を受けると死亡します。
 */
class NormalEnemy : public IEnemy {
public:
    /**
     * @enum Behavior
     * @brief 敵の振る舞いの種類を定義します
     */
    enum class Behavior {
        kWalk,  // 歩行
        kDeath, // 死亡
        kUnknown, // 不明
    };

public: // メンバ関数
    /**
     * @brief コンストラクタ
     * @param gameScene ゲームシーンのポインタ
     * @param camera カメラのポインタ
     */
    NormalEnemy(GameScene* gameScene, Camera* camera);

    /**
     * @brief 初期化処理
     * @param position 初期座標
     */
    void Initialize(const Vector3& position) override;
    /**
     * @brief 毎フレームの更新処理
     */
    void Update() override;
    /**
     * @brief 描画処理
     */
    void Draw() override;

    /**
     * @brief プレイヤーとの衝突時に呼ばれる関数
     * @param player 衝突したプレイヤーのポインタ
     */
    void OnCollision(Player* player) override;

private: // 内部処理
    /**
     * @brief ワールド行列を更新します
     */
    void UpdateMatrix();

    /**
     * @brief 各振る舞いの初期化・更新処理
     */
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