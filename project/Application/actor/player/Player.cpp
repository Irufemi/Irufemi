#include "Player.h"
#include "3D/SphereClass.h"
#include "manager/DebugUI.h"
#include <cassert>
#include <cmath>

// ---------- クォータニオンのユーティリティ ----------

namespace {

/// <summary>
/// クォータニオンの正規化
/// </summary>
/// <param name="q">クォータニオン</param>
/// <returns>長さが1に正規化されたクォータニオン</returns>
Quaternion Normalize(const Quaternion &q) {

  float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;

  // 長さが0なら単位クォータニオンを返す
  if (lenSq <= 0.0f) {
    return {0.0f, 0.0f, 0.0f, 1.0f};
  }

  // 正規化
  float invLen = 1.0f / std::sqrt(lenSq);

  return {q.x * invLen, q.y * invLen, q.z * invLen, q.w * invLen};
}

/// <summary>
/// クォータニオンの乗算関数,
/// q = a * b,
/// b 回転した後に a 回転する
/// </summary>
/// <param name="a"></param>
/// <param name="b"></param>
/// <returns> a * b </returns>
Quaternion Multiply(const Quaternion &a, const Quaternion &b) {
  Quaternion r;
  r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
  r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
  r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
  r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
  return r;
}

/// <summary>
/// 任意の軸の回転からクォータニオンを生成
/// </summary>
/// <param name="axis"></param>
/// <param name="angle"></param>
/// <returns></returns>
Quaternion FromAxisAngle(const Vector3 &axis, float angle) {
  float half = angle * 0.5f;
  float s = std::sin(half);
  return {axis.x * s, axis.y * s, axis.z * s, std::cos(half)};
}

/// <summary>
/// クォータニオンをオイラー角に変換する
/// </summary>
/// <param name="qIn"></param>
/// <returns> roll(X), pitch(Y), yaw(Z)の順番で出る </returns>
Vector3 QuaternionToEuler(const Quaternion &qIn) {
  Quaternion q = Normalize(qIn);

  // roll (X軸まわり)
  float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
  float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
  float roll = std::atan2(sinr_cosp, cosr_cosp);

  // pitch (Y軸まわり)
  float sinp = 2.0f * (q.w * q.y - q.z * q.x);
  float pitch;

  // 範囲外なら±90度にクリップする
  if (std::fabs(sinp) >= 1.0f) {
    pitch = std::copysign(3.14159265f / 2.0f, sinp);
  } else {
    pitch = std::asin(sinp);
  }

  // yaw (Z軸まわり)
  float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
  float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
  float yaw = std::atan2(siny_cosp, cosy_cosp);

  return {roll, pitch, yaw};
}

} // namespace

void Player::Initialize(Camera *camera, SphereClass *model, Vector3 positoin,
                        InputManager *input) {

  // 各項目の代入
  camera_ = camera;
  assert(model);
  model_ = model;
  input_ = input;

  // Transformの初期化
  transform_.translate = positoin;        // 初期座標
  transform_.scale = {1.0f, 1.0f, 1.0f};  // 球のスケール
  transform_.rotate = {0.0f, 0.0f, 0.0f}; // 初期回転

  // 回転クォータニオンの初期化（単位クォータニオンが入っているから回転なし状態）
  rotation_ = {0.0f, 0.0f, 0.0f, 1.0f};

  radius_ = baseRadius_;

  // 描画へ反映
  model_->SetRadius(radius_);
  model_->SetCenter(transform_.translate);
}

void Player::Update() {

// 動作確認のために、数秒ごとに岩の数が増えていく
#ifdef _DEBUG

  // if (Arock < 1.0f) {
  //   Arock += 0.01f;
  // } else {
  //   Arock = 0.0f;
  // }

  // rockCount_ += static_cast<uint32_t>(Arock);

#endif // _DEBUG

  // ノックバック処理
  if (isKnockback_) {

    const float knockbackPower = 0.8f;

    knockbackVel_ *= knockbackPower;

    // ノックバック速度を位置に反映
    transform_.translate.x += knockbackVel_.x;
    transform_.translate.y += knockbackVel_.y;
    transform_.translate.z += knockbackVel_.z;

    // 減衰（だんだん止まる）
    knockbackVel_.x *= 0.85f;
    knockbackVel_.y *= 0.88f; // 上方向は重力があるので落ちる
    knockbackVel_.z *= 0.85f;

    if (transform_.translate.y <= 0.0f) {
      transform_.translate.y = 0.0f;
      knockbackVel_.y = 0.0f;

      float horizontalV2 =
          knockbackVel_.x * knockbackVel_.x + knockbackVel_.z * knockbackVel_.z;

      if (horizontalV2 < 0.0001f) {
        isKnockback_ = false;
      }
    }

    // モデル更新
    model_->SetCenter(transform_.translate);
    model_->SetRotate(transform_.rotate);
    model_->Update();
  }

  // キー入力に応じた移動
  Move();

  // 毎フレーム重力を加える
  velocityY_ += gravity_;
  transform_.translate.y += velocityY_;

  // 地面着地判定
  if (transform_.translate.y < 0.0f) {
    transform_.translate.y = 0.0f;
    velocityY_ = 0.0f; // 着地でY速度リセット
  }

#ifdef _DEBUG

  // ImGui
  ImGui::Begin("Player");
  ImGui::DragFloat3("translate", &transform_.translate.x,
                    0.1f); // translate(平行移動)
  ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.1f); // rotate(回転角)
  ImGui::DragFloat("radius", &radius_, 0.1f, 0.1f, 5.0f);  // radius(半径)
  ImGui::DragFloat("velocityY", &velocityY_, 0.1f);
  ImGui::DragInt("rockCount", &rockCount_, 1, 0, 999);
  ImGui::Text("attackPower: %d", attackPower_);
  ImGui::End();

#endif // _DEBUG

  // 岩の数から再計算
  ReCalcStatusFromRock();

  // モデルにTransformやradiusを反映
  model_->SetCenter(transform_.translate);
  model_->SetRotate(transform_.rotate);
  model_->SetRadius(radius_);
  model_->Update();
}

void Player::Draw() { model_->Draw(); }

void Player::Move() {

  // ノックバック中なら移動しない
  if (isKnockback_) {
    return;
  }

  if (!input_) {
    return;
  }

  // 入力方向ベクトル
  Vector3 dir{0.0f, 0.0f, 0.0f};

  // キー入力での移動
  if (input_->IsKeyDown('W')) {
    dir.z += 1.0f; // 前
  }
  if (input_->IsKeyDown('S')) {
    dir.z -= 1.0f; // 後ろ
  }
  if (input_->IsKeyDown('D')) {
    dir.x += 1.0f; // 右
  }
  if (input_->IsKeyDown('A')) {
    dir.x -= 1.0f; // 左
  }

  // 入力無しなら終了
  if (dir.x == 0.0f && dir.z == 0.0f) {
    return;
  }

  // 斜め入力でも速度が一定になるよう正規化
  float directionLengthSquared = dir.x * dir.x + dir.z * dir.z;
  if (directionLengthSquared > 0.0f) {

    // 長さ
    float directionLength = std::sqrt(directionLengthSquared);

    // 正規化
    float normalizeFactor = 1.0f / directionLength;

    dir.x *= normalizeFactor;
    dir.z *= normalizeFactor;
  }

  // 移動速度(1フレーム当たりの移動量)
  const float speed = 0.1f;

  // 実際の移動
  transform_.translate.x += dir.x * speed;
  transform_.translate.z += dir.z * speed;

  // 転がりによる回転処理
  ApplyRolling(dir, speed);
}

void Player::ApplyRolling(const Vector3 &dir, float moveDistance) {

  // 移動してなければ即リターン
  if (moveDistance <= 0.0f) {
    return;
  }

  // 回転角 = 移動距離 / 半径
  float rollAngle = moveDistance / radius_;

  // 転がる軸を求める
  Vector3 axis; // 軸
  axis.x = dir.z;
  axis.y = 0.0f;
  axis.z = -dir.x;

  // 正規化
  float len = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);

  if (len <= 0.0f) {
    return;
  }

  axis.x /= len;
  axis.y /= len;
  axis.z /= len;

  // 軸と角度から回転クォータニオンを作成
  Quaternion delta = FromAxisAngle(axis, rollAngle);

  // 今までの回転にこのフレームの回転を合成する
  rotation_ = Normalize(Multiply(delta, rotation_));

  // Transform用にオイラー角に変換する
  transform_.rotate = QuaternionToEuler(rotation_);
}

void Player::ReCalcStatusFromRock() {

  // 下限クランプ
  if (rocksPerLevel_ <= 0) {
    rocksPerLevel_ = 3;
  }

  if (rockCount_ < 0) {
    rockCount_ = 0;
  }

  /* 今は仮で　x2,x3,x4...
  ------------------------*/

  // 倍率計算
  int mul = static_cast<int>(1.0f + (rockCount_ / rocksPerLevel_));

  // ダメージ
  //  岩　x　倍率
  attackPower_ = rockCount_ * mul;

  // サイズ
  // 初期半径 x 倍率
  radius_ = baseRadius_ * mul;
}
