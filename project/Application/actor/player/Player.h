#pragma once

#include "3D/SphereClass.h"
#include "contents/Quaternion.h"
#include "engine/Input/InputManager.h"
#include "math/Transform.h"

class Camera;

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
  /// <summary>
  /// 現在位置のゲッター
  /// </summary>
  const Vector3 &GetPosition() const { return transform_.translate; }

  /// <summary>
  /// 現在位置のセッター
  /// </summary>
  void SetPosition(const Vector3 &position) {
    transform_.translate = position;

    // モデルの座標も更新
    if (model_) {
      model_->SetCenter(transform_.translate);
      model_->Update();
    }
  }

  /// <summary>
  /// 半径を返す
  /// </summary>
  float GetRadius() const { return radius_; }

private:
  // カメラ
  Camera *camera_ = nullptr;

  // モデル(球)
  SphereClass *model_ = nullptr;

  // 入力
  InputManager *input_ = nullptr;

  // トランスフォーム
  Transform transform_{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  // 球の現在の半径
  float radius_;

  // 基本の半径
  const float baseRadius_ = 0.5f;

  // クォータニオン
  Quaternion rotation_{0.0f, 0.0f, 0.0f, 1.0f};

public:
  /// <summary>
  /// クォータニオンのゲッター
  /// </summary>
  Quaternion GetRotation() const { return rotation_; }

  /// <summary>
  /// オイラー角の取得
  /// </summary>
  Vector3 GetRotate() const { return transform_.rotate; }

private:
  float velocityY_ = 0.0f; // 縦方向の速度
  float gravity_ = -0.05f; // 重力加速度

public:
  // ノックバック用

  /// <summary>
  /// 上下の速度を加算
  /// </summary>
  /// <param name="v"></param>
  void AddVerticalVelocity(float v) { velocityY_ = v; }

  /// <summary>
  /// ノックバックを開始
  /// </summary>
  /// <param name="dir"></param>
  void StartKnockback(const Vector3 &dir) {
    isKnockback_ = true;
    knockbackVel_ = dir;
  }

private:
  bool isKnockback_ = false;      // ノックバック中かどうか
  Vector3 knockbackVel_{0, 0, 0}; // ノックバック速度

private:
  // 纏っている岩の数
  int rockCount_ = 0;

  // 現在の攻撃力
  int attackPower_ = 0;

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

  /// <summary>
  /// 岩の数を半分にする
  /// </summary>
  void HalveRockCount() {
    rockCount_ /= 2;

    // 0 未満にならないようにする保険
    if (rockCount_ < 0) {
      rockCount_ = 0;
    }
  }

private:
  /// <summary>
  /// 纏っている岩の数からダメージと半径を再計算
  /// </summary>
  void ReCalcStatusFromRock();

public:
  /// <summary>
  /// ローカル方向ベクトルをプレイヤーの現在の回転で回す
  /// </summary>
  /// <param name="dir"></param>
  /// <returns></returns>
  Vector3 RotateLocalDir(const Vector3 &dir) const;

  /// <summary>
  /// ワールド方向をプレイヤーローカルの方向に変換する
  /// </summary>
  /// <param name="worldDir"></param>
  /// <returns></returns>
  Vector3 WorldDirToLocal(const Vector3 &worldDir) const;

  /// <summary>
  /// ノックバック中かどうか
  /// </summary>
  bool IsKnockback() const { return isKnockback_; }
};
