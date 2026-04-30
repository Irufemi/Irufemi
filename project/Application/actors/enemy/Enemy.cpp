#include "Enemy.h"
#include "Body/Body.h"
#include "EnemyParameters.h"
#include "Engine/Platform/Input/InputManager.h"
#include "IrufemiEngine.h"
#include "Player.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "camera/Camera.h"
#include "Core/Math/Math.h"
#include <cmath>
#include <manager/debugUI.h>
#include <string>
#include "contents/ui/EnemyHPBar.h"
#include "contents/ui/EnemyPartHPBar.h"

Enemy::~Enemy() {}

void Enemy::Initialize(Camera *camera, IrufemiEngine *engine) {
  camera_ = camera;
  engine_ = engine;
#ifdef USE_IMGUI
  lineOBB_ = std::make_unique<Line3DRegion>();
  lineOBB_->Initialize(camera_);
#endif

  EnemyParameters::GetInstance()->Load("resources/Json/enemy/parameters.json");

  // 全体の初期トランスフォーム
  globalTransform_ = {
      {4.0f, 4.0f, 4.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f}};

  // 胴体の初期化
  for (int i = 0; i < 3; ++i) {
    bodies_[i] = std::make_unique<Body>();
    bodyLocalTransforms_[i] = {
        {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, (float)i * 2.0f, 0.0f}};
    bodies_[i]->Initialize(camera, bodyLocalTransforms_[i].translate);
    bodies_[i]->SetHP(EnemyParameters::GetInstance()->GetBodyHP());
    bodyOffsets_[i] = {0.0f, 0.0f, 0.0f};
  }

  // 頭部の初期化
  float topY = 6.0f;
  headLeft_ = std::make_unique<HeadLeft>();
  headLeftLocalTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-2.5f, topY, 0.0f}};
  headLeft_->Initialize(camera, headLeftLocalTransform_.translate);
  headLeft_->SetHP(EnemyParameters::GetInstance()->GetHeadLeftHP());

  headMid_ = std::make_unique<HeadMid>();
  headMidLocalTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, topY, 0.0f}};
  headMid_->Initialize(camera, headMidLocalTransform_.translate);
  headMid_->SetHP(EnemyParameters::GetInstance()->GetHeadMidHP());

  headRight_ = std::make_unique<HeadRight>();
  headRightLocalTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.5f, topY, 0.0f}};
  headRight_->Initialize(camera, headRightLocalTransform_.translate);
  headRight_->SetHP(EnemyParameters::GetInstance()->GetHeadRightHP());

  ai_ = std::make_unique<EnemyAI>();
  ai_->Initialize(this);
  animation_ = std::make_unique<EnemyAnimation>();
  animation_->Initialize(this);

  // ビームとエフェクトの事前初期化（ヒッチ対策）
  for (int i = 0; i < 3; ++i) {
      beams_[i] = std::make_unique<EnemyBeam>();
      beams_[i]->Initialize(camera_, engine_);
  }
  stompEffects_ = std::make_unique<EnemyStompEffects>();
  stompEffects_->Initialize(camera_);

  tackleEffects_ = std::make_unique<EnemyTackleEffects>();
  tackleEffects_->Initialize(camera_);

  // --- UI 初期化 ---
  hpBar_ = std::make_unique<EnemyHPBar>();
  if (engine_) {
      hpBar_->Initialize(camera_, engine_->GetClientWidth(), engine_->GetClientHeight());
  } else {
      hpBar_->Initialize(camera_, 1280, 720); // フォールバック
  }

  for (int i = 0; i < 6; ++i) {
      auto bar = std::make_unique<EnemyPartHPBar>();
      bar->Initialize(camera_);
      partHPBars_.push_back(std::move(bar));
  }

  isActive_ = true;
  isDead_ = false;
}

void Enemy::Update(Player *player) {
  if (!isActive_)
    return;

  if (!isDead_) {
    if (ai_)
      ai_->Update(player, engine_->GetDeltaTime());
    if (animation_)
      animation_->Update(player, 1.0f / 60.0f);
  }

#ifdef USE_IMGUI
  UpdateDebugUI();
#endif

  if (stompEffects_) {
      stompEffects_->Update(1.0f / 60.0f);
  }
  if (tackleEffects_) {
      tackleEffects_->Update(1.0f / 60.0f);
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
        if (camera_)
          camera_->Shake(shakeIntensity_, 15);
        triggeredShake = true;
      }
      isFalling_[i] = currentlyFalling;
      if (bodies_[i]->GetHP() > 0)
        targetY += 2.0f;
    }
  }
  headLeftLocalTransform_.translate.y +=
      (targetY - headLeftLocalTransform_.translate.y) * fallSpeed_;
  headMidLocalTransform_.translate.y +=
      (targetY - headMidLocalTransform_.translate.y) * fallSpeed_;
  headRightLocalTransform_.translate.y +=
      (targetY - headRightLocalTransform_.translate.y) * fallSpeed_;

  // 行列計算
  Matrix4x4 globalMat =
      Math::MakeAffineMatrix(globalTransform_.scale, globalTransform_.rotate,
                             globalTransform_.translate);

  // 胴体描画更新
  for (int i = 0; i < 3; ++i) {
    if (bodies_[i]) {
      Vector3 worldPosWithOffset = Math::Transform(
          Math::Add(bodyLocalTransforms_[i].translate, bodyOffsets_[i]),
          globalMat);
      Vector3 worldPosWithoutOffset =
          Math::Transform(bodyLocalTransforms_[i].translate, globalMat);
      bodies_[i]->SetTransform(
          {{globalTransform_.scale.x, globalTransform_.scale.y,
            globalTransform_.scale.z},
           globalTransform_.rotate,
           worldPosWithoutOffset},
          &worldPosWithOffset);
      bodies_[i]->Update();
    }
  }

  // 頭部描画更新
  auto updateHead = [&](auto &head, Transform &localT, Vector3 &offset) {
    if (head) {
      if (!isPhase2_) {
          // 通常時: 親子関係（globalMat）を使ってワールド座標を計算
          Vector3 worldPosWithOffset =
              Math::Transform(Math::Add(localT.translate, offset), globalMat);
          Vector3 worldPosWithoutOffset =
              Math::Transform(localT.translate, globalMat);
          Vector3 combinedRotate = Math::Add(globalTransform_.rotate, localT.rotate);
          Vector3 combinedScale = { globalTransform_.scale.x * localT.scale.x, 
                                    globalTransform_.scale.y * localT.scale.y, 
                                    globalTransform_.scale.z * localT.scale.z };

          // モデルを「根元から」回転・伸縮しているように見せるため、
          // スケールで伸びたローカルY軸方向に合わせてシフトさせる
          float shiftAmount = 0.0f;
          if (localT.scale.y > 1.0f) {
              Vector3 headHalfSize = EnemyParameters::GetInstance()->GetHeadOBBSize();
              // obb.size はハーフサイズなので、(scale - 1) 倍シフトさせれば下端が固定される
              shiftAmount = headHalfSize.y * (localT.scale.y - 1.0f);
          }
          
          Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(combinedRotate);
          Vector3 localUp = { rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2] }; 
          Vector3 shiftedWorldPos = Math::Add(worldPosWithOffset, Math::Multiply(shiftAmount, localUp));

          head->SetTransform({combinedScale, combinedRotate, shiftedWorldPos}, nullptr);
      } else {
          // フェーズ2: localT をそのままワールド座標として扱う（親子関係からの独立）
          // localT.translate には AnimationState が直接ワールド座標を書き込む想定
          head->SetTransform({globalTransform_.scale, localT.rotate, localT.translate}, nullptr);
      }
      head->Update();
    }
  };
  updateHead(headLeft_, headLeftLocalTransform_, headLeftOffset_);
  updateHead(headMid_, headMidLocalTransform_, headMidOffset_);
  updateHead(headRight_, headRightLocalTransform_, headRightOffset_);



  // 1. フェーズ2移行判定：ボディが全損した瞬間に移行する
  if (!isDead_ && !isPhase2_) {
    bool allBodiesZero = true;
    for (int i = 0; i < 3; ++i) {
      if (bodies_[i] && bodies_[i]->GetHP() > 0) {
        allBodiesZero = false;
        break;
      }
    }
    if (allBodiesZero) {
      isPhase2_ = true;
      SetState(EnemyState::Phase2);
    }
  }

  // 2. 死亡判定
  // 論理的な死亡判定（全てのHP全損）を常に評価する
  if (!isDead_) {
    bool allHpZero = true;
    for (int i = 0; i < 3; ++i) {
      if (bodies_[i] && bodies_[i]->GetHP() > 0) {
        allHpZero = false;
        break;
      }
    }
    if (allHpZero && headMid_->GetHP() <= 0 && headLeft_->GetHP() <= 0 &&
        headRight_->GetHP() <= 0) {
      isDead_ = true;
    }
  }

  // 3. 演出完了判定（全ての部位がボクセル含めて消滅したか）
  if (isDead_ && isActive_) {
    if (headMid_->IsCompletelyDead() && headLeft_->IsCompletelyDead() &&
        headRight_->IsCompletelyDead()) {
      bool allPartsGone = true;
      for (int i = 0; i < 3; ++i) {
        if (bodies_[i] && !bodies_[i]->IsCompletelyDead()) {
          allPartsGone = false;
          break;
        }
      }
      if (allPartsGone) {
        isActive_ = false; // 全ての部位（ボクセル粒子含む）が消滅したら非アクティブにする
      }
    }
  }

  // --- UI 更新 ---
  if (hpBar_) {
      hpBar_->Update(this);
  }

  auto updatePartBar = [&](int index, auto* part, int maxHp) {
      if (part && part->GetHP() > 0) {
          float ratio = (maxHp > 0) ? static_cast<float>(part->GetHP()) / maxHp : 0.0f;
          Vector3 hpPos = part->GetDrawPosition();
          float scaleY = part->GetTransform().scale.y;
          float offsetY = (index >= 3) ? (5.5f * scaleY) : (1.5f * scaleY);
          hpPos.y += offsetY;
          partHPBars_[index]->Update(ratio, hpPos, camera_);
      } else {
          partHPBars_[index]->Update(0.0f, { 0,0,0 }, nullptr);
      }
  };
  auto* p = EnemyParameters::GetInstance();
  updatePartBar(0, GetBody(0), p->GetBodyHP());
  updatePartBar(1, GetBody(1), p->GetBodyHP());
  updatePartBar(2, GetBody(2), p->GetBodyHP());
  updatePartBar(3, GetHeadLeft(), p->GetHeadLeftHP());
  updatePartBar(4, GetHeadMid(), p->GetHeadMidHP());
  updatePartBar(5, GetHeadRight(), p->GetHeadRightHP());
}

void Enemy::Draw(IrufemiEngine* engine) {
  if (!isActive_) return;
  for (auto &body : bodies_) {
    if (body && !body->IsCompletelyDead()) {
      body->Draw(engine);
    }
  }
  if (headLeft_ && !headLeft_->IsCompletelyDead())
    headLeft_->Draw(engine);
  if (headMid_ && !headMid_->IsCompletelyDead())
    headMid_->Draw(engine);
  if (headRight_ && !headRight_->IsCompletelyDead())
    headRight_->Draw(engine);

  // ビームを描画
  for (int i = 0; i < 3; ++i) {
      if (beams_[i]) {
          beams_[i]->Draw(engine);
      }
  }

    if (stompEffects_) {
        stompEffects_->Draw(engine);
    }
    if (tackleEffects_) {
        tackleEffects_->Draw(engine);
    }

#ifdef USE_IMGUI
  if (lineOBB_ && isDebugDrawOBB_ && engine_) {
    engine_->ApplyLineInstancedPSO();
    lineOBB_->Draw();
    engine_->ApplyPSO(); // restore
  }
#endif
}

// ビームの発射命令（トリガー）
void Enemy::FireBeam() {
  // すでに Initialize で生成済みのため、ここでは何もしない（アニメーション状態で制御）
}

bool Enemy::IsHeadDead(int index) const {
    if (index == 0) return headLeft_ && headLeft_->GetHP() <= 0;
    if (index == 1) return headMid_ && headMid_->GetHP() <= 0;
    if (index == 2) return headRight_ && headRight_->GetHP() <= 0;
    return true;
}

void Enemy::FireStomp(const Vector3& position) {
    if (stompEffects_) {
        stompEffects_->Fire(position);
    }
}

void Enemy::FireTackleRushWave(const Vector3& position) {
    if (tackleEffects_) {
        tackleEffects_->FireRushWave(position);
    }
}

void Enemy::FireTackleCrashWave(const Vector3& position) {
    if (tackleEffects_) {
        tackleEffects_->FireCrashWave(position);
    }
}

void Enemy::Draw3DUI(IrufemiEngine* engine, bool isUI) {
    if (!isActive_) return;
    
    auto drawIfAlive = [&](int index, auto* part) {
        if (part && part->GetHP() > 0) partHPBars_[index]->Draw(isUI);
    };
    drawIfAlive(0, GetBody(0));
    drawIfAlive(1, GetBody(1));
    drawIfAlive(2, GetBody(2));
    drawIfAlive(3, GetHeadLeft());
    drawIfAlive(4, GetHeadMid());
    drawIfAlive(5, GetHeadRight());
}

void Enemy::Draw2DUI(IrufemiEngine* engine) {
    if (!isActive_ || isDead_) return;
    if (hpBar_) {
        hpBar_->Draw();
    }
}

Matrix4x4 Enemy::GetHeadMidWorldMatrix() const {
  // globalTransform_.rotate には EnemyAnimation で計算した
  // 「プレイヤーを向くための X回転とY回転」が入っている必要があります
  Matrix4x4 globalMat = Math::MakeAffineMatrix(
      globalTransform_.scale,
      globalTransform_.rotate, // ここに X(上下) と Y(左右) が入っていればOK
      globalTransform_.translate);

  Vector3 localPos =
      Math::Add(headMidLocalTransform_.translate, headMidOffset_);

  return Math::Multiply(Math::MakeAffineMatrix(
                            {1, 1, 1}, headMidLocalTransform_.rotate, localPos),
                        globalMat);
}

OBB Enemy::GetOBB() const {
  OBB obb;
  // globalTransform_ から中心座標、回転、サイズを抽出して設定
  obb.center = globalTransform_.translate;

  // 各軸の方向ベクトル（回転から算出）
  Matrix4x4 rotateMat = Math::MakeRotateXYZMatrix(globalTransform_.rotate);
  obb.orientations[0] = {rotateMat.m[0][0], rotateMat.m[0][1],
                         rotateMat.m[0][2]};
  obb.orientations[1] = {rotateMat.m[1][0], rotateMat.m[1][1],
                         rotateMat.m[1][2]};
  obb.orientations[2] = {rotateMat.m[2][0], rotateMat.m[2][1],
                         rotateMat.m[2][2]};

  // 半径（サイズ）の設定
  obb.size = {2.0f, 4.0f, 2.0f}; // 敵の見た目に合わせた仮のサイズ

  return obb;
}

void Enemy::SetState(EnemyState newState) {
  state_ = newState;

  // ★重要：アニメーションクラスにも「状態が変わったよ！」と教えてあげる
  if (animation_) {
    animation_->ChangeState(newState);
  }
}

#ifdef USE_IMGUI
void Enemy::UpdateDebugUI() {
  if (engine_ && engine_->GetInputManager()->IsKeyPressedDIK(0x3B /*DIK_F1*/)) {
    isDebugDrawOBB_ = !isDebugDrawOBB_;
  }

  ImGui::Begin("Enemy HP Status");

  ImGui::Text("Enemy Status");

  auto clampHp = [](int hp) { return hp < 0 ? 0 : hp; };

  auto drawHpBar = [&](const char *label, int currentHp, int maxHp) {
    currentHp = clampHp(currentHp);

    float hpFraction = 0.0f;
    if (maxHp > 0) {
      hpFraction = static_cast<float>(currentHp) / static_cast<float>(maxHp);
    }

    if (hpFraction < 0.0f)
      hpFraction = 0.0f;
    if (hpFraction > 1.0f)
      hpFraction = 1.0f;

    char hpText[64];
    snprintf(hpText, sizeof(hpText), "%s : %d / %d", label, currentHp, maxHp);

    ImVec4 hpColor;
    if (hpFraction > 0.5f) {
      hpColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); // 緑
    } else if (hpFraction > 0.2f) {
      hpColor = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // 黄
    } else {
      hpColor = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); // 赤
    }

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpColor);
    ImGui::ProgressBar(hpFraction, ImVec2(-1.0f, 0.0f), hpText);
    ImGui::PopStyleColor();
  };

  // =========================
  // 累計HP
  // =========================
  int maxEnemyHp = EnemyParameters::GetInstance()->GetBodyHP() * 3 +
                   EnemyParameters::GetInstance()->GetHeadLeftHP() +
                   EnemyParameters::GetInstance()->GetHeadMidHP() +
                   EnemyParameters::GetInstance()->GetHeadRightHP();

  int currentEnemyHp = 0;
  for (int i = 0; i < 3; ++i) {
    if (bodies_[i]) {
      currentEnemyHp += clampHp(bodies_[i]->GetHP());
    }
  }
  if (headLeft_) {
    currentEnemyHp += clampHp(headLeft_->GetHP());
  }
  if (headMid_) {
    currentEnemyHp += clampHp(headMid_->GetHP());
  }
  if (headRight_) {
    currentEnemyHp += clampHp(headRight_->GetHP());
  }

  drawHpBar("Total HP", currentEnemyHp, maxEnemyHp);

  ImGui::Separator();

  // =========================
  // BodyごとのHP
  // =========================
  int bodyMaxHp = EnemyParameters::GetInstance()->GetBodyHP();

  for (int i = 0; i < 3; ++i) {
    char label[32];
    snprintf(label, sizeof(label), "Body %d", i + 1);

    int currentHp = 0;
    if (bodies_[i]) {
      currentHp = bodies_[i]->GetHP();
    }

    drawHpBar(label, currentHp, bodyMaxHp);
  }

  ImGui::Separator();

  // =========================
  // HeadごとのHP
  // =========================
  drawHpBar("Head Left", headLeft_ ? headLeft_->GetHP() : 0,
            EnemyParameters::GetInstance()->GetHeadLeftHP());

  drawHpBar("Head Mid", headMid_ ? headMid_->GetHP() : 0,
            EnemyParameters::GetInstance()->GetHeadMidHP());

  drawHpBar("Head Right", headRight_ ? headRight_->GetHP() : 0,
            EnemyParameters::GetInstance()->GetHeadRightHP());

  ImGui::Separator();

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

  float flashDuration =
      EnemyParameters::GetInstance()->GetDamageFlashDuration();
  if (ImGui::SliderFloat("Damage Flash Duration", &flashDuration, 0.0f, 1.0f)) {
    EnemyParameters::GetInstance()->SetDamageFlashDuration(flashDuration);
  }

  Vector4 flashColor = EnemyParameters::GetInstance()->GetDamageFlashColor();
  if (ImGui::ColorEdit4("Damage Flash Color", &flashColor.x)) {
    EnemyParameters::GetInstance()->SetDamageFlashColor(flashColor);
  }

  Vector3 bodyObb = EnemyParameters::GetInstance()->GetBodyOBBSize();
  if (ImGui::SliderFloat3("Body OBB Size", &bodyObb.x, 0.1f, 30.0f)) {
    EnemyParameters::GetInstance()->SetBodyOBBSize(bodyObb);
  }

  Vector3 headObb = EnemyParameters::GetInstance()->GetHeadOBBSize();
  if (ImGui::SliderFloat3("Head OBB Size", &headObb.x, 0.1f, 30.0f)) {
    EnemyParameters::GetInstance()->SetHeadOBBSize(headObb);
  }

  ImGui::End();

  if (lineOBB_) {
    lineOBB_->ClearInstances();
    if (isDebugDrawOBB_) {
      auto addObbLines = [&](const OBB &obb) {
        if (obb.size.x == 0.0f && obb.size.y == 0.0f && obb.size.z == 0.0f) return;
        Vector3 corners[8];
        for (int i = 0; i < 8; ++i) {
          Vector3 offset = {0, 0, 0};
          offset = Math::Add(offset,
                             Math::Multiply((i & 1) ? obb.size.x : -obb.size.x,
                                            obb.orientations[0]));
          offset = Math::Add(offset,
                             Math::Multiply((i & 2) ? obb.size.y : -obb.size.y,
                                            obb.orientations[1]));
          offset = Math::Add(offset,
                             Math::Multiply((i & 4) ? obb.size.z : -obb.size.z,
                                            obb.orientations[2]));
          corners[i] = Math::Add(obb.center, offset);
        }
        Vector4 color = {0.0f, 1.0f, 0.0f, 1.0f}; // Green
        lineOBB_->AddInstance(corners[0], corners[1], color);
        lineOBB_->AddInstance(corners[1], corners[3], color);
        lineOBB_->AddInstance(corners[3], corners[2], color);
        lineOBB_->AddInstance(corners[2], corners[0], color);
        lineOBB_->AddInstance(corners[4], corners[5], color);
        lineOBB_->AddInstance(corners[5], corners[7], color);
        lineOBB_->AddInstance(corners[7], corners[6], color);
        lineOBB_->AddInstance(corners[6], corners[4], color);
        lineOBB_->AddInstance(corners[0], corners[4], color);
        lineOBB_->AddInstance(corners[1], corners[5], color);
        lineOBB_->AddInstance(corners[2], corners[6], color);
        lineOBB_->AddInstance(corners[3], corners[7], color);
      };
      for (int i = 0; i < 3; ++i) {
        if (bodies_[i] && !bodies_[i]->IsCompletelyDead())
          addObbLines(bodies_[i]->GetOBB());
      }
      if (headLeft_ && !headLeft_->IsCompletelyDead())
        addObbLines(headLeft_->GetOBB());
      if (headMid_ && !headMid_->IsCompletelyDead())
        addObbLines(headMid_->GetOBB());
      if (headRight_ && !headRight_->IsCompletelyDead())
        addObbLines(headRight_->GetOBB());
      for (int i = 0; i < 3; ++i) {
          if (beams_[i]) addObbLines(beams_[i]->GetOBB());
      }
      if (stompEffects_)
        stompEffects_->DrawDebug(lineOBB_.get());
      if (tackleEffects_)
        tackleEffects_->DrawDebug(lineOBB_.get());
    }
    lineOBB_->Update();
  }
}
#endif
