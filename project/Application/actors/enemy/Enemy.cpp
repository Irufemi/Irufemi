#include "Enemy.h"
#include "camera/Camera.h"
#include "core/math/geometry/Math.h"
#include "EnemyParameters.h"
#include <manager/debugUI.h>
#include "Engine/Core/Math/Geometry/OBB.h"
#include "actors/enemy/Body/Body.h"
#include <cmath>
#include <string>

Enemy::~Enemy() {}

void Enemy::Initialize(Camera* camera) {
    camera_ = camera;
    EnemyParameters::GetInstance()->Load("resources/Json/enemy/parameters.json");

    // 全体の初期トランスフォーム
    globalTransform_ = { {4.0f, 4.0f, 4.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f} };

    // 胴体の初期化
    for (int i = 0; i < 3; ++i) {
        bodies_[i] = std::make_unique<Body>();
        bodyLocalTransforms_[i] = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, (float)i * 2.0f, 0.0f} };
        bodies_[i]->Initialize(camera, bodyLocalTransforms_[i].translate);
        bodies_[i]->SetHP(EnemyParameters::GetInstance()->GetBodyHP());
        bodyOffsets_[i] = { 0.0f, 0.0f, 0.0f };
    }

    // 頭部の初期化
    float topY = 6.0f;
    headLeft_ = std::make_unique<HeadLeft>();
    headLeftLocalTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-2.5f, topY, 0.0f} };
    headLeft_->Initialize(camera, headLeftLocalTransform_.translate);
    headLeft_->SetHP(EnemyParameters::GetInstance()->GetHeadLeftHP());

    headMid_ = std::make_unique<HeadMid>();
    headMidLocalTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, topY, 0.0f} };
    headMid_->Initialize(camera, headMidLocalTransform_.translate);
    headMid_->SetHP(EnemyParameters::GetInstance()->GetHeadMidHP());

    headRight_ = std::make_unique<HeadRight>();
    headRightLocalTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.5f, topY, 0.0f} };
    headRight_->Initialize(camera, headRightLocalTransform_.translate);
    headRight_->SetHP(EnemyParameters::GetInstance()->GetHeadRightHP());

    ai_ = std::make_unique<EnemyAI>();
    ai_->Initialize(this);
    animation_ = std::make_unique<EnemyAnimation>();
    animation_->Initialize(this);

    isActive_ = true;
}

void Enemy::Update() {
    if (!isActive_) return;

    if (ai_) ai_->Update();
    if (animation_) animation_->Update();

    // --- ビームの更新と位置同期 ---
    if (beam_) {
        // 常に最新の「頭のワールド行列」を渡す
        // これにより、頭がシェイクで揺れても、ビームも一緒に揺れる
        beam_->Update(GetHeadMidWorldMatrix());

        if (beam_->IsExpired()) {
            beam_.reset();
        }
    }

    // だるま落とし落下物理
    float targetY = 0.0f;
    bool triggeredShake = false;
    for (int i = 0; i < 3; ++i) {
        if (bodies_[i]) {
            float diff = targetY - bodyLocalTransforms_[i].translate.y;
            bodyLocalTransforms_[i].translate.y += diff * fallSpeed_;
            bool currentlyFalling = std::abs(diff) > 0.1f;
            if (isFalling_[i] && !currentlyFalling && !triggeredShake) {
                if (camera_) camera_->Shake(shakeIntensity_, 15);
                triggeredShake = true;
            }
            isFalling_[i] = currentlyFalling;
            if (bodies_[i]->GetHP() > 0) targetY += 2.0f;
        }
    }
    headLeftLocalTransform_.translate.y += (targetY - headLeftLocalTransform_.translate.y) * fallSpeed_;
    headMidLocalTransform_.translate.y += (targetY - headMidLocalTransform_.translate.y) * fallSpeed_;
    headRightLocalTransform_.translate.y += (targetY - headRightLocalTransform_.translate.y) * fallSpeed_;

    // 行列計算
    Matrix4x4 globalMat = Math::MakeAffineMatrix(globalTransform_.scale, globalTransform_.rotate, globalTransform_.translate);

    // 胴体描画更新
    for (int i = 0; i < 3; ++i) {
        if (bodies_[i]) {
            Vector3 worldPos = Math::Transform(Math::Add(bodyLocalTransforms_[i].translate, bodyOffsets_[i]), globalMat);
            bodies_[i]->SetTransform({ {globalTransform_.scale.x, globalTransform_.scale.y, globalTransform_.scale.z}, globalTransform_.rotate, worldPos });
            bodies_[i]->Update();
        }
    }

    // 頭部描画更新
    auto updateHead = [&](auto& head, Transform& localT, Vector3& offset) {
        if (head) {
            Vector3 worldPos = Math::Transform(Math::Add(localT.translate, offset), globalMat);
            head->SetTransform({ globalTransform_.scale, globalTransform_.rotate, worldPos });
            head->Update();
        }
        };
    updateHead(headLeft_, headLeftLocalTransform_, headLeftOffset_);
    updateHead(headMid_, headMidLocalTransform_, headMidOffset_);
    updateHead(headRight_, headRightLocalTransform_, headRightOffset_);

#ifdef USE_IMGUI
    ImGui::Begin("Enemy HP Status");
    ImGui::SliderFloat("Fall Speed", &fallSpeed_, 0.01f, 1.0f);
    ImGui::SliderFloat("Shake Intensity", &shakeIntensity_, 0.0f, 10.0f);

    float blowSpeed = EnemyParameters::GetInstance()->GetBlowSpeed();
    if (ImGui::SliderFloat("Blow Speed", &blowSpeed, 0.0f, 5.0f)) {
        EnemyParameters::GetInstance()->SetBlowSpeed(blowSpeed);
    }
    float disappearTime = EnemyParameters::GetInstance()->GetDisappearTime();
    if (ImGui::SliderFloat("Disappear Time", &disappearTime, 0.5f, 10.0f)) {
        EnemyParameters::GetInstance()->SetDisappearTime(disappearTime);
    }

    for (int i = 0; i < 3; ++i) {
        if (bodies_[i]) {
            int hp = bodies_[i]->GetHP();
            if (ImGui::SliderInt(("Body[" + std::to_string(i) + "] HP").c_str(), &hp, 0, 10000)) bodies_[i]->SetHP(hp);
        }
    }
    ImGui::End();
#endif
}

void Enemy::Draw() {
    if (!isActive_) return;
    for (auto& body : bodies_) if (body && body->GetHP() > 0) body->Draw();
    if (headLeft_ && headLeft_->GetHP() > 0) headLeft_->Draw();
    if (headMid_ && headMid_->GetHP() > 0) headMid_->Draw();
    if (headRight_ && headRight_->GetHP() > 0) headRight_->Draw();
    // ビームを描画
    if (beam_) beam_->Draw();
}

// ビームの発射命令（トリガー）
void Enemy::FireBeam() {
    if (!beam_) {
        beam_ = std::make_unique<EnemyBeam>();
        beam_->Initialize(camera_, GetHeadMidWorldMatrix());
    }
}

Matrix4x4 Enemy::GetHeadMidWorldMatrix() const {
    Matrix4x4 globalMat = Math::MakeAffineMatrix(globalTransform_.scale, globalTransform_.rotate, globalTransform_.translate);
    Vector3 localPos = Math::Add(headMidLocalTransform_.translate, headMidOffset_);
    return Math::Multiply(Math::MakeAffineMatrix({ 1,1,1 }, headMidLocalTransform_.rotate, localPos), globalMat);
}

OBB Enemy::GetOBB() const {
    OBB obb;
    // globalTransform_ から中心座標、回転、サイズを抽出して設定
    obb.center = globalTransform_.translate;

    // 各軸の方向ベクトル（回転から算出）
    Matrix4x4 rotateMat = Math::MakeRotateXYZMatrix(globalTransform_.rotate);
    obb.orientations[0] = { rotateMat.m[0][0], rotateMat.m[0][1], rotateMat.m[0][2] };
    obb.orientations[1] = { rotateMat.m[1][0], rotateMat.m[1][1], rotateMat.m[1][2] };
    obb.orientations[2] = { rotateMat.m[2][0], rotateMat.m[2][1], rotateMat.m[2][2] };

    // 半径（サイズ）の設定
    obb.size = { 2.0f, 4.0f, 2.0f }; // 敵の見た目に合わせた仮のサイズ

    return obb;
}
