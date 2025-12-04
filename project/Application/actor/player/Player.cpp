#include "Player.h"
#include "3D/SphereClass.h"
#include "manager/DebugUI.h"
#include <cassert>

void Player::Initialize(Camera *camera, SphereClass *model, Vector3 positoin,
                        InputManager *input) {

  // 引数を代入
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

  // 半径設定
  radius_ = baseRadius_;

  transform_.translate.y = radius_;

  // モデルへ反映
  model_->SetRadius(radius_);
  model_->SetCenter(transform_.translate);
}

void Player::Update() {

  // ノックバック処理
  if (isKnockback_) {

    const float knockbackPower = 0.8f;

    knockbackVel_ *= knockbackPower;

    // ノックバック速度を位置に反映
    transform_.translate += knockbackVel_;

    // 減衰（だんだん止まる）
    knockbackVel_.x *= 0.85f;
    knockbackVel_.y *= 0.88f; // 上方向は重力があるので落ちる
    knockbackVel_.z *= 0.85f;

    // 着地判定
    if (transform_.translate.y <= 0.0f) {
      transform_.translate.y = 0.0f;
      knockbackVel_.y = 0.0f;

      float horizontalV2 =
          knockbackVel_.x * knockbackVel_.x + knockbackVel_.z * knockbackVel_.z;

      // 速度が0に近づけば終了
      if (horizontalV2 < 0.0001f) {
        isKnockback_ = false;
      }
    }
  }

  // キー入力に応じた移動
  Move();

  // 毎フレーム重力を加える
  velocityY_ += gravity_;
  transform_.translate.y += velocityY_;

  // 地面着地判定
  if (transform_.translate.y < radius_) {
    transform_.translate.y = radius_;
    velocityY_ = 0.0f; // 着地でY速度リセット
  }

#ifdef _DEBUG

  // ImGui
  ImGui::Begin("Player");
  ImGui::DragFloat3("translate", &transform_.translate.x,
                    0.1f); // translate(平行移動)
  ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.1f); // rotate(回転角)
  ImGui::Text("radius: %f", radius_);
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
  // model_->SetRadius(radius_);
  model_->Update();
}

void Player::Draw() {
  // モデルの描画
  model_->Draw();
}

void Player::Move() {

  // ノックバック中は移動できない
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

  /*ダメージは x2,x3,x4
  * サイズは  x1.2,x1.4,x1.6
  ----------------------------*/

  // 倍率計算
  int damageMul = static_cast<int>(1.0f + (rockCount_ / rocksPerLevel_));

  int sizeMul = static_cast<int>(1.0f + (rockCount_ / rocksPerLevel_ * 0.5f));

  // ダメージ
  //  岩の数　x　倍率
  attackPower_ = rockCount_ * damageMul;

  // サイズ
  // 初期半径 x 倍率
  radius_ = baseRadius_ * sizeMul;
}

Vector3 Player::RotateLocalDir(const Vector3 &dir) const {

  // ローカル方向を現在の回転クォータニオンで回す
  return RotateByQuaternion(dir, rotation_);
}

Vector3 Player::WorldDirToLocal(const Vector3 &worldDir) const {

  // rotateの逆を作る
  Quaternion invRot{-rotation_.x, -rotation_.y, -rotation_.z, rotation_.w};

  // ワールド方向を「プレイヤーローカル」に変換
  return RotateByQuaternion(worldDir, invRot);
}
