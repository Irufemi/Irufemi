#pragma once

#include "engine/Input/InputManager.h"
#include "math/Transform.h"

#include "3D/SphereClass.h"

class Camera;

// プレイヤーの回転(姿勢)を表すクォータニオン
// どの向きで回転しているかを保持するためのもの
// オイラー角だけだと斜め移動時の転がりで軸が破綻するため、
// 内部ではこちらで姿勢を管理し、最後にオイラー角に変換してTransformに渡す。
struct Quaternion {
  float x;
  float y;
  float z;
  float w;
};

class Player {

public:
  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="camera">カメラ</param>
  /// <param name="model">モデル</param>
  /// <param name="positoin">初期座標</param>
  /// <param name="input">入力マネージャ</param>
  void Initialize(Camera *camera, SphereClass *model, Vector3 positoin,
                  InputManager *input);

  /// <summary>
  /// 更新処理
  /// - WASD入力による移動・転がり処理
  /// - ImGuiでデバッグ表示
  /// -モデルにTransformを反映させる
  /// </summary>
  void Update();

  /// <summary>
  /// 描画処理
  /// -モデルの描画
  /// </summary>
  void Draw();

private:
  /// <summary>
  /// 移動
  /// -WASDによる移動処理
  /// -斜めは正規化して一定速度に調整済み
  /// </summary>
  void Move();

  /// <summary>
  /// 転がりの回転処理
  /// </summary>
  /// <param name="dir">正規化済みの方向ベクトル</param>
  /// <param name="moveDistance">このフレーム中に移動した距離</param>
  void ApplyRolling(const Vector3 &dir, float moveDistance);

public:
  // トランスフォーム

  const Vector3 &GetPosition() const { return transform_.translate; }

  void SetPosition(const Vector3 &position) {
    transform_.translate = position;

    // モデルの座標も更新
    if (model_) {
      model_->SetCenter(transform_.translate);
      model_->Update();
    }
  }

  float GetRadius() const { return radius_; }

private:
  // カメラ
  Camera *camera_ = nullptr;

  // モデル
  SphereClass *model_ = nullptr;

  // 入力
  InputManager *input_ = nullptr;

  // トランスフォーム
  Transform transform_{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  // 球の半径
  float radius_;

  const float baseRadius_ = 0.5f;

  // クォータニオン
  Quaternion rotation_{0.0f, 0.0f, 0.0f, 1.0f};

  float velocityY_ = 0.0f; // 上下の速度（Y）
  float gravity_ = -0.05f; // 重力加速度

public:
  // ノックバック用
  void AddVerticalVelocity(float v) { velocityY_ = v; }

  void StartKnockback(const Vector3 &dir) {
    isKnockback_ = true;
    knockbackVel_ = dir;
  }

private:
  // ノックバック用
  bool isKnockback_ = false;
  Vector3 knockbackVel_{0, 0, 0};

private:
  // 纏っている岩の情報
  int rockCount_ = 0;

  // 攻撃力
  int attackPower_ = 0;

#ifdef _DEBUG

  // デバッグ用
  // float Arock = 0.0f;

#endif // D_EBUG

  // 何個毎に倍率をあげるか
  int rocksPerLevel_ = 3; // 三個毎に x1 -> x2 -> x3...

public:
  /// <summary>
  /// 纏っている岩の数
  /// </summary>
  /// <returns></returns>
  int GetRockCount() const { return rockCount_; }

  /// <summary>
  /// 現在の攻撃力
  /// </summary>
  /// <returns></returns>
  int GetAttackPower() const { return attackPower_; }

  /// <summary>
  /// 岩の数を増やす
  /// </summary>
  void AddRock(int amount) {
    rockCount_ += amount;

    // 0より下にならないようにクランプ
    if (rockCount_ < 0) {
      rockCount_ = 0;
    }
  }

  /// <summary>
  /// 岩の数を0にリセット
  /// </summary>
  void ResetRockCount() { rockCount_ = 0; }

private:
  /// <summary>
  /// 纏っている岩の数からダメージと半径を計算
  /// </summary>
  void ReCalcStatusFromRock();
};
