#pragma once

// 前方宣言：ヘッダーのインクルードループを防ぐ
class Enemy;
class Player;

/**
 * @brief 敵のアニメーション状態の基底インターフェース
 */
class IEnemyAnimationState {
public:
    virtual ~IEnemyAnimationState() = default;

    // その状態に入った瞬間（初期化用）
    virtual void Enter(Enemy* enemy) = 0;

    // 毎フレームの更新処理（元のUpdateの中身）
    virtual void Update(Enemy* enemy, Player* player, float deltaTime) = 0;

    // その状態を抜ける瞬間（後片付け用）
    virtual void Exit(Enemy* enemy) = 0;

    // AIが「攻撃が終わったか」を知るための関数
    virtual bool IsFinished() const { return false; }

    // 描画側が「今ビームが出ているか」を知るための関数
    virtual bool IsFiring() const { return false; }
};