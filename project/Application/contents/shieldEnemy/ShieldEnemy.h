#pragma once

#include "math/shape/AABB.h"
#include "math/Transform.h"
#include "math/Matrix4x4.h"
#include "3D/ObjClass.h"
#include <memory>

class Camera;
class Player;

class ShieldEnemy {

public: // メンバ関数

    // 当たり判定(AABB)の取得
    AABB GetAABB();

    // カメラを設定
    static void SetCamera(Camera* camera) { camera_ = camera; }


    // Playerを設定
    static void SetPlayer(Player* player) { player_ = player; }

private: // メンバ変数(ゲームシステム)

    // Transform(拡縮、回転、位置)
    Transform transform_{};

    // worldMatrix
    Matrix4x4 worldMatrix_{};

    // 当たり判定サイズ (右か左を向いているので横から見たとき準拠)
    float kWidth_ = 1.2f;
    float kHeight = 1.0f;

    bool isDamageReduction = false;

    // デスフラグ
    bool isDead_ = false;

    // カメラ(ポインタ参照)
    static Camera* camera_;

    // Player(ポインタ参照)
    static Player* player_;


private: // メンバ変数(描画)

    // Obj
    std::unique_ptr<ObjClass> model_ = nullptr;

};

