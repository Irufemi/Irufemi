#pragma once

#include <memory>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"

class PlaneClass;
class Camera;
class IrufemiEngine;

/**
 * @class EnemyPartHPBar
 * @brief 敵の各部位の少し上に表示する3DビルボードHPバー（PlaneClass使用）
 */
class EnemyPartHPBar {
public:
    ~EnemyPartHPBar() = default;

    /**
     * @brief 初期化
     * @param engine エンジン
     */
    void Initialize(IrufemiEngine* engine);

    /**
     * @brief 毎フレーム更新（HP割合と色を更新し、指定された位置に追従する）
     * @param hpRatio 現在のHP割合 (0.0〜1.0)
     * @param targetWorldPos 追従する対象のワールド座標（この上にオフセットをかけて表示する）
     * @param pullRadius モデルへの埋まりを回避するためにカメラ側に引き寄せる半径
     */
    void Update(float hpRatio, const Vector3& targetWorldPos, float pullRadius = 0.0f);

    /**
     * @brief 3Dポリゴン描画
     */
    void Draw(bool isUI = false);

private:
    /// HP割合からバーの色を計算する（緑→黄→赤のグラデーション）
    void UpdateBarColor(float hpRatio);

    // バー表示用3Dポリゴン
    std::unique_ptr<PlaneClass> barBg_;       ///< 背景（暗いバー）
    std::unique_ptr<PlaneClass> barFill_;     ///< HP残量（色付きバー）
    std::unique_ptr<PlaneClass> barFrame_;    ///< 枠線

    // 3D空間でのサイズ・オフセット定数
    float barMaxWidth_ = 0.0f;
    float barHeight_ = 0.0f;

    // アニメーション用（ダメージ時のスムーズな減少）
    float displayRatio_ = 1.0f;
    IrufemiEngine* engine_ = nullptr;
};
