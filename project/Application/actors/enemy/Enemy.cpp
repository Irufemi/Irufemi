#include "Enemy.h"
#include "camera/Camera.h"
#include "function/Math.h"
#include "EnemyParameters.h"
#include <imgui.h>

Enemy::~Enemy() {}

void Enemy::Initialize(Camera *camera) {
  
  // JSONからのパラメータ読み込み
  EnemyParameters::GetInstance()->Load("resources/Json/enemy/parameters.json");

  // ボス全体の初期Transform設定
  globalTransform_ = {{4.0f, 4.0f, 4.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f}};

  // 各部位の初期化とローカルTransform設定
  for (int i = 0; i < 3; ++i) {
    bodies_[i] = std::make_unique<Body>();
    // ローカル座標で「だるま落とし」のように積む
    bodyLocalTransforms_[i] = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, i * 2.0f, 0.0f}};
    bodies_[i]->Initialize(camera, bodyLocalTransforms_[i].translate); // 初期のグローバル座標は0で初期化されるが、初回のUpdateですぐ上書きされる
    bodies_[i]->SetHP(EnemyParameters::GetInstance()->GetBodyHP());
  }

  float topY = 2 * 2.0f + 2.0f; // 3段目がY=4.0、その上
  
  headLeft_ = std::make_unique<HeadLeft>();
  headLeftLocalTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-2.5f, topY, 0.0f}};
  headLeft_->Initialize(camera, headLeftLocalTransform_.translate);
  headLeft_->SetHP(EnemyParameters::GetInstance()->GetHeadLeftHP());

  headMid_ = std::make_unique<HeadMid>();
  headMidLocalTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, topY, 0.0f}};
  headMid_->Initialize(camera, headMidLocalTransform_.translate);
  headMid_->SetHP(EnemyParameters::GetInstance()->GetHeadMidHP());

  headRight_ = std::make_unique<HeadRight>();
  headRightLocalTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.5f, topY, 0.0f}};
  headRight_->Initialize(camera, headRightLocalTransform_.translate);
  headRight_->SetHP(EnemyParameters::GetInstance()->GetHeadRightHP());

  // AIの初期化
  ai_ = std::make_unique<EnemyAI>();
  ai_->Initialize(this);

  isActive_ = true;
}

void Enemy::Update() {
  if (!isActive_) return;

  // AIによる全体のTransform・各部位のTransform更新
  if (ai_) {
    ai_->Update();
  }

  // --- だるま落としの高さ計算と落下アニメーション ---
  float targetY = 0.0f;
  for (int i = 0; i < 3; ++i) {
    if (bodies_[i]) {
      // 現在の目標Y座標に向けて滑らかに移動させる (毎フレーム20%近づく)
      bodyLocalTransforms_[i].translate.y += (targetY - bodyLocalTransforms_[i].translate.y) * 0.2f;

      // この部位が破壊されていなければ上に高さを積む
      if (bodies_[i]->GetHP() > 0) {
        targetY += 2.0f;
      }
    }
  }

  // 頭の高さはボディの積算高さを目標にする
  if (headLeft_) headLeftLocalTransform_.translate.y += (targetY - headLeftLocalTransform_.translate.y) * 0.2f;
  if (headMid_) headMidLocalTransform_.translate.y += (targetY - headMidLocalTransform_.translate.y) * 0.2f;
  if (headRight_) headRightLocalTransform_.translate.y += (targetY - headRightLocalTransform_.translate.y) * 0.2f;
  // -----------------------------------------------------

  // グローバル変換行列の生成（平行移動・回転・拡縮）
  Matrix4x4 globalMatrix = Math::MakeAffineMatrix(globalTransform_.scale, globalTransform_.rotate, globalTransform_.translate);

  // Bodyの更新
  for (int i = 0; i < 3; ++i) {
    if (bodies_[i]) {

      // 【1】位置の合成： ローカル位置をグローバル行列で変換
      Vector3 worldPos = Math::Transform(bodyLocalTransforms_[i].translate, globalMatrix);
      // 【2】回転の合成： ローカル回転とグローバル回転を加算（簡易的なオイラー角の合成）
      Vector3 worldRot = Math::Add(globalTransform_.rotate, bodyLocalTransforms_[i].rotate);
      // 【3】スケールの合成：
      Vector3 worldScale = {globalTransform_.scale.x * bodyLocalTransforms_[i].scale.x, globalTransform_.scale.y * bodyLocalTransforms_[i].scale.y, globalTransform_.scale.z * bodyLocalTransforms_[i].scale.z};

      bodies_[i]->SetTransform({worldScale, worldRot, worldPos});
      bodies_[i]->Update();
    }
  }

  // HeadLeftの更新
  if (headLeft_) {
    Vector3 worldPos = Math::Transform(headLeftLocalTransform_.translate, globalMatrix);
    Vector3 worldRot = Math::Add(globalTransform_.rotate, headLeftLocalTransform_.rotate);
    Vector3 worldScale = {globalTransform_.scale.x * headLeftLocalTransform_.scale.x, globalTransform_.scale.y * headLeftLocalTransform_.scale.y, globalTransform_.scale.z * headLeftLocalTransform_.scale.z};
    headLeft_->SetTransform({worldScale, worldRot, worldPos});
    headLeft_->Update();
  }

  // HeadMidの更新
  if (headMid_) {
    Vector3 worldPos = Math::Transform(headMidLocalTransform_.translate, globalMatrix);
    Vector3 worldRot = Math::Add(globalTransform_.rotate, headMidLocalTransform_.rotate);
    Vector3 worldScale = {globalTransform_.scale.x * headMidLocalTransform_.scale.x, globalTransform_.scale.y * headMidLocalTransform_.scale.y, globalTransform_.scale.z * headMidLocalTransform_.scale.z};
    headMid_->SetTransform({worldScale, worldRot, worldPos});
    headMid_->Update();
  }

  // HeadRightの更新
  if (headRight_) {
    Vector3 worldPos = Math::Transform(headRightLocalTransform_.translate, globalMatrix);
    Vector3 worldRot = Math::Add(globalTransform_.rotate, headRightLocalTransform_.rotate);
    Vector3 worldScale = {globalTransform_.scale.x * headRightLocalTransform_.scale.x, globalTransform_.scale.y * headRightLocalTransform_.scale.y, globalTransform_.scale.z * headRightLocalTransform_.scale.z};
    headRight_->SetTransform({worldScale, worldRot, worldPos});
    headRight_->Update();
  }

#ifdef USE_IMGUI
  ImGui::Begin("Enemy HP Status");
  for (int i = 0; i < 3; ++i) {
    if (bodies_[i]) {
      int hp = bodies_[i]->GetHP();
      std::string label = "Body[" + std::to_string(i) + "] HP";
      if (ImGui::SliderInt(label.c_str(), &hp, 0, 10000)) {
        bodies_[i]->SetHP(hp);
      }
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

  for (auto& body : bodies_) {
    if (body && body->GetHP() > 0) body->Draw();
  }
  if (headLeft_ && headLeft_->GetHP() > 0) headLeft_->Draw();
  if (headMid_ && headMid_->GetHP() > 0) headMid_->Draw();
  if (headRight_ && headRight_->GetHP() > 0) headRight_->Draw();
}
