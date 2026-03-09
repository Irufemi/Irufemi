#include "EnemyBeam.h"
#include <cmath>

void EnemyBeam::Initialize(Camera* camera, const Matrix4x4& muzzleMatrix) {
    obj_ = std::make_unique<ObjClass>();
    // 指定された仮モデルを使用
    obj_->Initialize(camera, "sample/block.obj");

    // muzzleMatrix(HeadMidのワールド行列)から座標と回転を抽出
    // 行列の4列目（m[3]）が座標、1〜3列目が各軸の方向
    transform_.translate = { muzzleMatrix.m[3][0], muzzleMatrix.m[3][1], muzzleMatrix.m[3][2] };

    // ここでは単純化のため、発射時のHeadの回転をコピー
    // ※本来は行列からオイラー角を抽出するか、行列をそのまま保持するのが理想
    transform_.rotate = { 0, 0, 0 };

    // スケール：Z軸方向に長く伸ばすことでビームに見せる
    transform_.scale = { beamThickness_, beamThickness_, 0.1f }; // 最初は短く
}

void EnemyBeam::Update(const Matrix4x4& muzzleMatrix) {
    timer_ += 1.0f / 60.0f;

    // 頭のワールド行列から位置と方向を常に更新（位置の同期）
    transform_.translate = { muzzleMatrix.m[3][0], muzzleMatrix.m[3][1], muzzleMatrix.m[3][2] };

    // 方向の同期（ muzzleMatrixの回転成分を反映させる処理をここに書く ）

    // 演出：ビームを伸ばす
    if (transform_.scale.z < maxLength_) {
        transform_.scale.z += 10.0f;
    }

    if (timer_ > lifeTime_) isExpired_ = true;

    obj_->SetTransform(transform_);
    obj_->Update();
}

void EnemyBeam::Draw() {
    if (obj_) obj_->Draw();
}

OBB EnemyBeam::GetOBB() const {
    OBB obb;
    obb.center = transform_.translate;
    // ビームの中心点は発射口から Z方向に scale.z / 2 進んだ場所
    // ※回転を考慮した計算が必要
    obb.size = transform_.scale;
    return obb;
}