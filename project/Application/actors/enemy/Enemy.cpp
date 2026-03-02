#include "Enemy.h"
#include "camera/Camera.h"
#include "function/Math.h"

Enemy::~Enemy() {}

void Enemy::Initialize(Camera *camera) {
  
  // ボス全体の初期Transform設定
  globalTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  // 各部位の初期化とローカルTransform設定
  for (int i = 0; i < 3; ++i) {
    bodies_[i] = std::make_unique<Body>();
    // ローカル座標で「だるま落とし」のように積む
    bodyLocalTransforms_[i] = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, i * 2.0f, 0.0f}};
    bodies_[i]->Initialize(camera, bodyLocalTransforms_[i].translate); // 初期のグローバル座標は0で初期化されるが、初回のUpdateですぐ上書きされる
  }

  float topY = 2 * 2.0f + 2.0f; // 3段目がY=4.0、その上
  
  headLeft_ = std::make_unique<HeadLeft>();
  headLeftLocalTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-2.5f, topY, 0.0f}};
  headLeft_->Initialize(camera, headLeftLocalTransform_.translate);

  headMid_ = std::make_unique<HeadMid>();
  headMidLocalTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, topY, 0.0f}};
  headMid_->Initialize(camera, headMidLocalTransform_.translate);

  headRight_ = std::make_unique<HeadRight>();
  headRightLocalTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.5f, topY, 0.0f}};
  headRight_->Initialize(camera, headRightLocalTransform_.translate);

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

  // グローバル変換行列の生成（平行移動・回転・拡縮）
  Matrix4x4 globalMatrix = Math::MakeAffineMatrix(globalTransform_.scale, globalTransform_.rotate, globalTransform_.translate);

  // Bodyの更新
  for (int i = 0; i < 3; ++i) {
    if (bodies_[i]) {
      // ローカル行列 * グローバル行列 でワールド行列を算出し、座標などを抽出する簡易実装
      // （IrufemiEngineではアフィン変換成分から行列にする関数はあるが行列から座標等を取り出す関数はExtractEulerFromMatrixのみで、スケール・位置を抽出する共通関数がないため
      //   自前で行列の積による位置変換と回転合成を行う）

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
}

void Enemy::Draw() {
  if (!isActive_) return;

  for (auto& body : bodies_) {
    if (body) body->Draw();
  }
  if (headLeft_) headLeft_->Draw();
  if (headMid_) headMid_->Draw();
  if (headRight_) headRight_->Draw();
}
