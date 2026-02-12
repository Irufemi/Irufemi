#pragma once

#include "actors/enemy/IEnemy.h"
#include <cstdint>

// 前方宣言
class Camera;
class GameScene;

/**
 * @class ShieldEnemy
 * @brief 盾を持つ敵キャラクタークラス
 * @details 正面からの攻撃を盾で防ぎ、ダメージを無効化する特性を持ちます。
 *          背後からの攻撃に対しては脆弱です。
 */
class ShieldEnemy : public IEnemy {
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
    ShieldEnemy(GameScene* gameScene, Camera* camera);

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
     * @details 攻撃が正面からか背後からかを判定し、振る舞いを変更します。
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

    // ダメージ軽減フラグ
    bool isDamageReduction = false;
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

