#include "Enemy.h"
#include "Irufemi.h"
#include "camera/Camera.h"
#include "EnemyParameters.h"
#include <cmath>
#include <algorithm>
#include <string>

Enemy::~Enemy() {}

void Enemy::Initialize(Camera* camera) {
    camera_ = camera;

    EnemyParameters::GetInstance()->Load("resources/Json/enemy/parameters.json");

    globalTransform_ = { {4.0f, 4.0f, 4.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f} };

    for (int i = 0; i < 3; ++i) {
        bodies_[i] = std::make_unique<Body>();
        bodyLocalTransforms_[i] = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, (float)i * 2.0f, 0.0f} };
        bodies_[i]->Initialize(camera, bodyLocalTransforms_[i].translate);
        bodies_[i]->SetHP(EnemyParameters::GetInstance()->GetBodyHP());
        bodyOffsets_[i] = { 0.0f, 0.0f, 0.0f };
    }

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

    // だるま落としの物理計算
    float targetY = 0.0f;
    bool triggeredShakeThisFrame = false;

    for (int i = 0; i < 3; ++i) {
        if (bodies_[i]) {
            float diff = targetY - bodyLocalTransforms_[i].translate.y;
            bodyLocalTransforms_[i].translate.y += diff * fallSpeed_;
            bool currentlyFalling = std::abs(diff) > 0.1f;
            if (isFalling_[i] && !currentlyFalling && !triggeredShakeThisFrame) {
                if (camera_) camera_->Shake(shakeIntensity_, 15);
                triggeredShakeThisFrame = true;
            }
            isFalling_[i] = currentlyFalling;
            if (bodies_[i]->GetHP() > 0) targetY += 2.0f;
        }
    }

    if (headLeft_) headLeftLocalTransform_.translate.y += (targetY - headLeftLocalTransform_.translate.y) * fallSpeed_;
    if (headMid_) headMidLocalTransform_.translate.y += (targetY - headMidLocalTransform_.translate.y) * fallSpeed_;
    if (headRight_) headRightLocalTransform_.translate.y += (targetY - headRightLocalTransform_.translate.y) * fallSpeed_;

    // 行列の更新
    Matrix4x4 globalMatrix = Math::MakeAffineMatrix(globalTransform_.scale, globalTransform_.rotate, globalTransform_.translate);

    for (int i = 0; i < 3; ++i) {
        if (bodies_[i]) {
            Vector3 finalLocalPos = Math::Add(bodyLocalTransforms_[i].translate, bodyOffsets_[i]);
            Vector3 worldPos = Math::Transform(finalLocalPos, globalMatrix);
            Vector3 worldRot = Math::Add(globalTransform_.rotate, bodyLocalTransforms_[i].rotate);
            Vector3 worldScale = {
                globalTransform_.scale.x * bodyLocalTransforms_[i].scale.x,
                globalTransform_.scale.y * bodyLocalTransforms_[i].scale.y,
                globalTransform_.scale.z * bodyLocalTransforms_[i].scale.z
            };
            bodies_[i]->SetTransform({ worldScale, worldRot, worldPos });
            bodies_[i]->Update();
        }
    }

    if (headLeft_) {
        Vector3 worldPos = Math::Transform(Math::Add(headLeftLocalTransform_.translate, headLeftOffset_), globalMatrix);
        Vector3 worldRot = Math::Add(globalTransform_.rotate, headLeftLocalTransform_.rotate);
        Vector3 worldScale = {
            globalTransform_.scale.x * headLeftLocalTransform_.scale.x,
            globalTransform_.scale.y * headLeftLocalTransform_.scale.y,
            globalTransform_.scale.z * headLeftLocalTransform_.scale.z
        };
        headLeft_->SetTransform({ worldScale, worldRot, worldPos });
        headLeft_->Update();
    }
    if (headMid_) {
        Vector3 worldPos = Math::Transform(Math::Add(headMidLocalTransform_.translate, headMidOffset_), globalMatrix);
        Vector3 worldRot = Math::Add(globalTransform_.rotate, headMidLocalTransform_.rotate);
        Vector3 worldScale = {
            globalTransform_.scale.x * headMidLocalTransform_.scale.x,
            globalTransform_.scale.y * headMidLocalTransform_.scale.y,
            globalTransform_.scale.z * headMidLocalTransform_.scale.z
        };
        headMid_->SetTransform({ worldScale, worldRot, worldPos });
        headMid_->Update();
    }
    if (headRight_) {
        Vector3 worldPos = Math::Transform(Math::Add(headRightLocalTransform_.translate, headRightOffset_), globalMatrix);
        Vector3 worldRot = Math::Add(globalTransform_.rotate, headRightLocalTransform_.rotate);
        Vector3 worldScale = {
            globalTransform_.scale.x * headRightLocalTransform_.scale.x,
            globalTransform_.scale.y * headRightLocalTransform_.scale.y,
            globalTransform_.scale.z * headRightLocalTransform_.scale.z
        };
        headRight_->SetTransform({ worldScale, worldRot, worldPos });
        headRight_->Update();
    }

#ifdef USE_IMGUI
    ImGui::Begin("Enemy HP Status");
    ImGui::Separator();
    ImGui::Text("Animation Settings");
    ImGui::SliderFloat("Fall Speed", &fallSpeed_, 0.01f, 1.0f);
    ImGui::SliderFloat("Shake Intensity", &shakeIntensity_, 0.0f, 10.0f);
    ImGui::Separator();

    for (int i = 0; i < 3; ++i) {
        if (bodies_[i]) {
            int hp = bodies_[i]->GetHP();
            std::string label = "Body[" + std::to_string(i) + "] HP";
            if (ImGui::SliderInt(label.c_str(), &hp, 0, 10000)) bodies_[i]->SetHP(hp);
        }
    }
    if (headLeft_) {
        int hp = headLeft_->GetHP();
        if (ImGui::SliderInt("Head Left HP", &hp, 0, 10000)) headLeft_->SetHP(hp);
    }
    if (headMid_) {
        int hp = headMid_->GetHP();
        if (ImGui::SliderInt("Head Mid HP", &hp, 0, 10000)) headMid_->SetHP(hp);
    }
    if (headRight_) {
        int hp = headRight_->GetHP();
        if (ImGui::SliderInt("Head Right HP", &hp, 0, 10000)) headRight_->SetHP(hp);
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
}