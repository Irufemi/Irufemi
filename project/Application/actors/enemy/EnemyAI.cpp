#include "EnemyAI.h"
#include "Enemy.h"
#include <cmath>

EnemyAI::~EnemyAI() {}

void EnemyAI::Initialize(Enemy* enemy) {
  enemy_ = enemy;
  timer_ = 0.0f;
}

void EnemyAI::Update() {
  if (!enemy_) return;

  // -------------------------------------------------------------------
  // ここでボス全体と、各部位の動き（Transform）を一緒に制御します
  // -------------------------------------------------------------------

  // 全体のトランスフォームを取得（参照渡しで直接編集する）
  Transform& globalTransform = enemy_->GetGlobalTransform();
  
  // 各部位のローカルトランスフォームを取得
  Transform& bodyTop = enemy_->GetBodyLocalTransform(2); // 一番上の胴体
  Transform& bodyMid = enemy_->GetBodyLocalTransform(1); // 真ん中の胴体
  Transform& bodyBot = enemy_->GetBodyLocalTransform(0); // 一番下の胴体

  Transform& headLeft = enemy_->GetHeadLeftLocalTransform();   // 左の頭
  Transform& headMid = enemy_->GetHeadMidLocalTransform();     // 真ん中の頭
  Transform& headRight = enemy_->GetHeadRightLocalTransform(); // 右の頭

  // タイマー更新
  timer_ += 1.0f / 60.0f;

  // -- 仮アニメーション（AIによる制御テスト） --
  globalTransform.rotate.y += 0.005f;  // 全体がY軸で回転する
  
  bodyTop.translate.y += std::sin(globalTransform.translate.z * 10.0f) * 0.05f;
  bodyMid.translate.y += std::sin(globalTransform.translate.z * 10.0f) * 0.05f; 
  bodyBot.translate.y += std::sin(globalTransform.translate.z * 10.0f) * 0.05f;

  // 左の頭がグルグル回る
  headLeft.rotate.z += 0.05f; 
  // -------------------------------------------------------------------

}
