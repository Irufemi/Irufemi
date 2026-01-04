#pragma once

#include "math/Transform.h"
#include "math/Matrix4x4.h"
#include "math/shape/AABB.h"
#include "math/LRDirection.h"

#include <memory>

// 前方宣言
class Player;
class ObjClass;

class IEnemy
{
public: // メンバ関数
    // デストラクタ
    virtual ~IEnemy() = default;

    // 初期化
    virtual void Initialize(const Vector3& position);
    // 更新
    virtual void Update() = 0;
    // 描画
    virtual void Draw() = 0;

    // 衝突時の処理
    virtual void OnCollision(Player* player);

public: // アクセサ
    // AABBを取得する
    AABB GetAABB() const;
    // 生存フラグを取得する
    bool IsDead() const { return isDead_; }
    // 向きを取得する
    LRDirection GetLRDirection() const { return lrDirection_; }

protected: // メンバ変数
    // トランスフォーム
    Transform transform_;
    // ワールド行列
    Matrix4x4 worldMatrix_;
    // モデル
    std::unique_ptr<ObjClass> model_ = nullptr;
    // 幅
    float width_ = 1.0f;
    // 高さ
    float height_ = 1.0f;
    // 生存フラグ
    bool isDead_ = false;
    // 向き
    LRDirection lrDirection_ = LRDirection::kLeft;
};

