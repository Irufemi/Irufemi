#include "Enemy.h"
#include "Body/Body.h"
#include "EnemyParameters.h"
#include "Engine/Platform/Input/InputManager.h"
#include "IrufemiEngine.h"
#include "Player.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Core/Math/Math.h"
#include <cmath>
#include <manager/debugUI.h>
#include <string>
#include "contents/ui/EnemyHPBar.h"
#include "contents/ui/EnemyPartHPBar.h"
#include "Renderer/Effect/WeaponTrail.h"
#include "Renderer/Object2D/Sprite/Sprite.h"

Enemy::Enemy() = default;
Enemy::~Enemy() {}

void Enemy::Initialize(IrufemiEngine *engine) {
  engine_ = engine;
#ifdef USE_IMGUI
  lineOBB_ = std::make_unique<Line3DRegion>();
  lineOBB_->Initialize();
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
    bodies_[i]->Initialize(bodyLocalTransforms_[i].translate);
    bodies_[i]->SetHP(EnemyParameters::GetInstance()->GetBodyHP());
    bodyOffsets_[i] = {0.0f, 0.0f, 0.0f};
  }

  // 頭部の初期化
  float topY = 6.0f;
  headLeft_ = std::make_unique<HeadLeft>();
  headLeftLocalTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-2.5f, topY, 0.0f}};
  headLeft_->Initialize(headLeftLocalTransform_.translate);
  headLeft_->SetHP(EnemyParameters::GetInstance()->GetHeadLeftHP());

  headMid_ = std::make_unique<HeadMid>();
  headMidLocalTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-0.5f, topY, 0.0f}};
  headMid_->Initialize(headMidLocalTransform_.translate);
  headMid_->SetHP(EnemyParameters::GetInstance()->GetHeadMidHP());

  headRight_ = std::make_unique<HeadRight>();
  headRightLocalTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.5f, topY, 0.0f}};
  headRight_->Initialize(headRightLocalTransform_.translate);
  headRight_->SetHP(EnemyParameters::GetInstance()->GetHeadRightHP());

  ai_ = std::make_unique<EnemyAI>();
  ai_->Initialize(this);
  animation_ = std::make_unique<EnemyAnimation>();
  animation_->Initialize(this);

  // ビームとエフェクトの事前初期化（ヒッチ対策）
  for (int i = 0; i < 3; ++i) {
      beams_[i] = std::make_unique<EnemyBeam>();
      beams_[i]->Initialize(engine_);
      
      bombs_[i] = std::make_unique<EnemyBomb>();
      bombs_[i]->Initialize(engine_);
  }
  stompEffects_ = std::make_unique<EnemyStompEffects>();
  stompEffects_->Initialize();

  tackleEffects_ = std::make_unique<EnemyTackleEffects>();
  tackleEffects_->Initialize();

  neckTrail_ = std::make_unique<WeaponTrail>();
  neckTrail_->Initialize(engine_, "resources/gradationLine.png", {1.0f, 0.2f, 0.2f, 1.0f}); // エネミーの首振り用の赤いトレイル

  // --- UI 初期化 ---
  hpBar_ = std::make_unique<EnemyHPBar>();
  if (engine_) {
      hpBar_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
  } else {
      hpBar_->Initialize(1280, 720); // フォールバック
  }

  for (int i = 0; i < 6; ++i) {
      auto bar = std::make_unique<EnemyPartHPBar>();
      bar->Initialize(engine_);
      partHPBars_.push_back(std::move(bar));
  }

  isActive_ = true;
  isDead_ = false;
  deathPhase_ = DeathPhase::None;
  deathTimer_ = 0.0f;

  // death phase 用の初期位置保存
  for (int i = 0; i < 3; ++i) {
      initialBodyLocalTransforms_[i] = bodyLocalTransforms_[i];
  }
  initialHeadLeftLocalTransform_ = headLeftLocalTransform_;
  initialHeadMidLocalTransform_ = headMidLocalTransform_;
  initialHeadRightLocalTransform_ = headRightLocalTransform_;
}

  // 警告用注意マークの初期化
  warningSprite_ = std::make_unique<Sprite>();
  warningSprite_->Initialize("resources/texture/player/tyui.png");
  warningSprite_->SetSize(180.0f, 180.0f);
  if (engine_) {
      warningSprite_->SetPositionCenter(static_cast<float>(engine_->GetClientWidth()) / 2.0f, 200.0f);
  } else {
      warningSprite_->SetPositionCenter(640.0f, 200.0f);
  }

  // 警告用矢印スプライト（左右2個ずつ）の初期化
  auto initArrow = [&](std::unique_ptr<Sprite>& arrow) {
      arrow = std::make_unique<Sprite>();
      arrow->Initialize("resources/texture/player/yazirusi.png");
      arrow->SetSize(80.0f, 80.0f);
      arrow->SetRotation(-1.570796f); // 上向きに回転 (-π/2ラジアン)
  };

  initArrow(warningArrowLeft1_);
  initArrow(warningArrowLeft2_);
  initArrow(warningArrowRight1_);
  initArrow(warningArrowRight2_);
}

void Enemy::Update(Player *player) {
  if (!isActive_)
    return;

  if (!isDead_) {
    if (ai_ && !isSandbagMode_)
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
  if (neckTrail_) {
      neckTrail_->Update();
  }
  for (int i = 0; i < 3; ++i) {
      if (bombs_[i] && !bombs_[i]->IsExpired()) {
          bombs_[i]->Update();
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
        if (auto* cam = engine_->GetCameraManager()->GetActiveCamera())
          cam->Shake(shakeIntensity_, 15);
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
  if (!isDead_ && !isPhase2_ && !isSandbagMode_) {
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
      // 頭部へフェーズ移行を通知（ロケット噴射開始）
      if (headLeft_) headLeft_->SetPhase2(true);
      if (headMid_) headMid_->SetPhase2(true);
      if (headRight_) headRight_->SetPhase2(true);
    }
  }

  // 2. 死亡判定
  // 論理的な死亡判定（全てのHP全損）を常に評価する
  if (!isDead_ && !isSandbagMode_) {
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

      // 死亡が確定した瞬間に攻撃をキャンセルし、引き戻しフェーズへ移行
      for (int i = 0; i < 3; ++i) {
          if (beams_[i]) {
              beams_[i]->SetAttackActive(false);
              beams_[i]->SetTelegraphActive(false);
              beams_[i]->SetChargeSphereActive(false);
          }
          if (bombs_[i]) {
              bombs_[i]->Cancel();
          }
      }
      // 各部位の吹き飛びをリセット
      Matrix4x4 globalMat = Math::MakeAffineMatrix(globalTransform_.scale, globalTransform_.rotate, globalTransform_.translate);
      Matrix4x4 invGlobalMat = Math::Inverse(globalMat);

      for (int i = 0; i < 3; ++i) {
          if (bodies_[i]) bodies_[i]->ResetBlow();
          startBodyLocalTransforms_[i] = bodyLocalTransforms_[i];
      }
      if (headLeft_) {
          headLeft_->ResetBlow();
          startHeadLeftLocalTransform_ = headLeftLocalTransform_;
          startHeadLeftLocalTransform_.translate = Math::Transform(headLeftLocalTransform_.translate, invGlobalMat);
      }
      if (headMid_) {
          headMid_->ResetBlow();
          startHeadMidLocalTransform_ = headMidLocalTransform_;
          startHeadMidLocalTransform_.translate = Math::Transform(headMidLocalTransform_.translate, invGlobalMat);
      }
      if (headRight_) {
          headRight_->ResetBlow();
          startHeadRightLocalTransform_ = headRightLocalTransform_;
          startHeadRightLocalTransform_.translate = Math::Transform(headRightLocalTransform_.translate, invGlobalMat);
      }

      // フェーズ2を解除して親子関係ベースの描画に復帰
      isPhase2_ = false;

      // 死亡開始時の全体位置を保存
      startGlobalTranslate_ = globalTransform_.translate;

      deathPhase_ = DeathPhase::Reassembling;
      deathTimer_ = 0.0f;
    }
  }

  // 死亡中なら死亡フェーズを更新
  if (isDead_) {
      UpdateDeathPhase(engine_->GetDeltaTime());
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
          partHPBars_[index]->Update(ratio, hpPos);
      } else {
          partHPBars_[index]->Update(0.0f, { 0,0,0 });
      }
  };
  auto* p = EnemyParameters::GetInstance();
  updatePartBar(0, GetBody(0), p->GetBodyHP());
  updatePartBar(1, GetBody(1), p->GetBodyHP());
  updatePartBar(2, GetBody(2), p->GetBodyHP());
  updatePartBar(3, GetHeadLeft(), p->GetHeadLeftHP());
  updatePartBar(4, GetHeadMid(), p->GetHeadMidHP());
  updatePartBar(5, GetHeadRight(), p->GetHeadRightHP());

  // --- 警告（スタンプ攻撃予兆）演出の更新 ---
  {
      if (isWarningActive_) {
          warningTimer_ += engine_ ? engine_->GetDeltaTime() : 1.0f / 60.0f;
      } else {
          warningTimer_ = 0.0f;
      }

      float pulse = std::sin(warningTimer_ * 10.0f); // 激しい脈動 (注意マーク用)
      float slowPulse = std::sin(warningTimer_ * 4.0f); // ゆっくりめの脈動 (矢印点滅用)
      float centerX = engine_ ? (static_cast<float>(engine_->GetClientWidth()) / 2.0f) : 640.0f;

      if (isWarningActive_) {
          // 1. 注意マークの脈動
          if (warningSprite_) {
              float baseScale = 180.0f;
              float currentScale = baseScale * (1.0f + 0.15f * pulse);
              warningSprite_->SetSize(currentScale, currentScale);

              float alpha = 1.0f; 
              warningSprite_->SetColor(Vector4{ 1.0f, 1.0f, 1.0f, alpha });
              warningSprite_->SetPositionCenter(centerX, 200.0f);
              warningSprite_->Update();
          }

          // 2. 矢印スプライト（左右2個ずつ）の更新
          auto updateArrow = [&](std::unique_ptr<Sprite>& arrow, float offsetX, float phaseOffset) {
              if (!arrow) return;

              // ゆっくりめの明滅アルファ (0.6 ~ 1.0)
              float arrowAlpha = 0.6f + 0.4f * (slowPulse * 0.5f + 0.5f);

              // 下から上へのスライド移動アニメーション
              float slideSpeed = 1.5f;
              float slideFactor = std::fmod(warningTimer_ * slideSpeed + phaseOffset, 1.0f);
              
              float startY = 140.0f;
              float endY = 40.0f;
              float arrowY = startY + (endY - startY) * slideFactor;

              // スライドの開始・終了時になめらかにフェードイン・フェードアウト
              float slideFade = 1.0f;
              if (slideFactor < 0.2f) {
                  slideFade = slideFactor / 0.2f;
              } else if (slideFactor > 0.8f) {
                  slideFade = (1.0f - slideFactor) / 0.2f;
              }

              float finalArrowAlpha = arrowAlpha * slideFade;

              arrow->SetColor(Vector4{ 1.0f, 0.2f, 0.2f, finalArrowAlpha });
              arrow->SetPositionCenter(centerX + offsetX, arrowY);
              arrow->Update();
          };

          // 左右に2個ずつの矢印を更新
          // 内側の矢印 (Xオフセット: ±130.0f, 位相差なし)
          updateArrow(warningArrowLeft1_, -130.0f, 0.0f);
          updateArrow(warningArrowRight1_, 130.0f, 0.0f);
          // 外側の矢印 (Xオフセット: ±250.0f, 位相差0.5fで交互に動く)
          updateArrow(warningArrowLeft2_, -250.0f, 0.5f);
          updateArrow(warningArrowRight2_, 250.0f, 0.5f);

          // 赤いビネットの設定
          if (engine_) {
              auto* pp = engine_->GetPostProcessManager();
              if (pp) {
                  if (!pp->HasActiveMode(PostProcessMode::Vignette)) {
                      pp->AddActiveMode(PostProcessMode::Vignette);
                  }
                  auto& vignette = pp->GetVignetteParams();
                  vignette.color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤警告色
                  vignette.scale = vignetteBaseScale_ + vignetteScalePulseWidth_ * pulse;
                  vignette.power = vignetteBasePower_ + vignettePowerPulseWidth_ * pulse;
              }
          }
      } else {
          // 警告非アクティブ時の後処理 (ビネットの解除)
          if (engine_) {
              auto* pp = engine_->GetPostProcessManager();
              if (pp) {
                  if (pp->HasActiveMode(PostProcessMode::Vignette)) {
                      pp->RemoveActiveMode(PostProcessMode::Vignette);
                      // デフォルトパラメータに復元
                      auto& vignette = pp->GetVignetteParams();
                      vignette.color = { 0.0f, 0.0f, 0.0f, 0.0f };
                      vignette.scale = 16.0f;
                      vignette.power = 0.8f;
                  }
              }
          }
      }
  }
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

  // 爆弾を描画
  for (int i = 0; i < 3; ++i) {
      if (bombs_[i] && !bombs_[i]->IsExpired()) {
          bombs_[i]->Draw(engine);
      }
  }

    if (stompEffects_) {
        stompEffects_->Draw(engine);
    }
    if (tackleEffects_) {
        tackleEffects_->Draw(engine);
    }
    if (neckTrail_) {
        neckTrail_->SyncBeforeDraw();
        neckTrail_->Draw();
    }

#ifdef USE_IMGUI
  if (lineOBB_ && isDebugDrawOBB_ && engine_) {
    lineOBB_->Draw();
  }
#endif
}

// ビームの発射命令（トリガー）
void Enemy::FireBeam() {
  // すでに Initialize で生成済みのため、ここでは何もしない（アニメーション状態で制御）
}

// 爆弾の発射命令
void Enemy::FireBomb(int index, const Vector3& targetPos) {
    if (index >= 0 && index < 3 && bombs_[index]) {
        Transform* headT = nullptr;
        if (index == 0) headT = &headLeftLocalTransform_;
        else if (index == 1) headT = &headMidLocalTransform_;
        else if (index == 2) headT = &headRightLocalTransform_;

        if (headT) {
            Vector3 startPos = headT->translate;
            // フェーズ2では translate がワールド座標になっているためそのまま使用
            bombs_[index]->Throw(startPos, targetPos);
        }
    }
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

void Enemy::Draw2DUI(IrufemiEngine* engine, bool isFirstPerson) {
    if (!isActive_ || isDead_) return;
    if (isFirstPerson && hpBar_) {
        hpBar_->Draw();
    }
    // 警告演出がアクティブな場合のみ描画する
    if (isWarningActive_) {
        if (warningSprite_) {
            warningSprite_->Draw();
        }
        if (warningArrowLeft1_) {
            warningArrowLeft1_->Draw();
        }
        if (warningArrowLeft2_) {
            warningArrowLeft2_->Draw();
        }
        if (warningArrowRight1_) {
            warningArrowRight1_->Draw();
        }
        if (warningArrowRight2_) {
            warningArrowRight2_->Draw();
        }
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

Vector3 Enemy::GetTargetPosition() const {
    if (isPhase2_) {
        // 生きている頭を一つ選んで返す（真ん中、左、右の順）
        if (headMid_ && headMid_->GetHP() > 0) {
            return headMid_->GetTransform().translate;
        } else if (headLeft_ && headLeft_->GetHP() > 0) {
            return headLeft_->GetTransform().translate;
        } else if (headRight_ && headRight_->GetHP() > 0) {
            return headRight_->GetTransform().translate;
        }
    }
    return globalTransform_.translate;
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

  ImGui::Separator();
  ImGui::Text("Vignette Settings (Stomp Warning)");
  ImGui::SliderFloat("Vignette Base Scale", &vignetteBaseScale_, 5.0f, 100.0f, "%.1f");
  ImGui::SliderFloat("Vignette Scale Pulse", &vignetteScalePulseWidth_, 0.0f, 10.0f, "%.1f");
  ImGui::SliderFloat("Vignette Base Power", &vignetteBasePower_, 0.05f, 2.0f, "%.2f");
  ImGui::SliderFloat("Vignette Power Pulse", &vignettePowerPulseWidth_, 0.0f, 1.0f, "%.2f");

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
          if (bombs_[i] && !bombs_[i]->IsExpired()) {
              auto obbs = bombs_[i]->GetOBBs();
              for (const auto& obb : obbs) addObbLines(obb);
          }
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

void Enemy::UpdateDeathPhase(float deltaTime) {
    if (deathPhase_ == DeathPhase::Reassembling) {
        deathTimer_ += deltaTime;
        float t = deathTimer_ / kReassembleDuration; // 合体設定時間を基準にする
        if (t > 1.0f) t = 1.0f;

        // Smoothstepでイージング
        float easedT = t * t * (3.0f - 2.0f * t);

        auto lerpTransform = [](const Transform& current, const Transform& target, float factor) -> Transform {
            Transform result = current;
            result.translate = Math::Add(current.translate, Math::Multiply(factor, Math::Subtract(target.translate, current.translate)));
            // 回転は単純Lerp
            result.rotate = Math::Add(current.rotate, Math::Multiply(factor, Math::Subtract(target.rotate, current.rotate)));
            return result;
        };

        for (int i = 0; i < 3; ++i) {
            bodyLocalTransforms_[i] = lerpTransform(startBodyLocalTransforms_[i], initialBodyLocalTransforms_[i], easedT);
        }
        headLeftLocalTransform_ = lerpTransform(startHeadLeftLocalTransform_, initialHeadLeftLocalTransform_, easedT);
        headMidLocalTransform_ = lerpTransform(startHeadMidLocalTransform_, initialHeadMidLocalTransform_, easedT);
        headRightLocalTransform_ = lerpTransform(startHeadRightLocalTransform_, initialHeadRightLocalTransform_, easedT);

        // ボス全体のグローバル座標も、マップ中央 (0.0f, 3.0f, 0.0f) へイージング移動させる
        Vector3 centerPos = { 0.0f, 3.0f, 0.0f };
        globalTransform_.translate = Math::Add(startGlobalTranslate_, Math::Multiply(easedT, Math::Subtract(centerPos, startGlobalTranslate_)));

        if (t >= 1.0f) {
            deathPhase_ = DeathPhase::Gathered;
            deathTimer_ = 0.0f; // タメ用のタイマーリセット
        }
    } else if (deathPhase_ == DeathPhase::Gathered) {
        deathTimer_ += deltaTime;


        // 完全合体した状態（Phase 1の初期ローカル位置）を維持
        for (int i = 0; i < 3; ++i) {
            bodyLocalTransforms_[i] = initialBodyLocalTransforms_[i];
        }
        headLeftLocalTransform_ = initialHeadLeftLocalTransform_;
        headMidLocalTransform_ = initialHeadMidLocalTransform_;
        headRightLocalTransform_ = initialHeadRightLocalTransform_;

        // 苦しんで暴れている（のたうち回る）表現をサイン波で実装
        // 1. 全体のダイナミックな傾き（暴れ）
        globalTransform_.rotate.x = std::sin(deathTimer_ * kAgonyPitchFreq) * kAgonyPitchAmp;
        globalTransform_.rotate.z = std::cos(deathTimer_ * kAgonyRollFreq) * kAgonyRollAmp;

        // 2. 小刻みな高速振動（ブルブル感）と上下ののたうち（暴れ）
        float shakeOffset = std::sin(deathTimer_ * kShakeFreq) * kShakeAmp;
        float verticalOffset = std::sin(deathTimer_ * kVerticalFreq) * kVerticalAmp;

        const Vector3 kCenterPos = { 0.0f, 3.0f, 0.0f };
        globalTransform_.translate = { 
            kCenterPos.x + shakeOffset, 
            kCenterPos.y + verticalOffset, 
            kCenterPos.z + shakeOffset * 0.5f 
        };

        // 3. ３つの頭部がそれぞれ苦しそうに反り返ったり、うねる動き
        float headWiggle = std::sin(deathTimer_ * kHeadWiggleFreq) * kHeadWiggleAmp;
        
        headLeftLocalTransform_.translate.x -= std::abs(headWiggle) * 0.2f;
        headLeftLocalTransform_.translate.y += headWiggle * 0.3f;
        headLeftLocalTransform_.rotate.z = -headWiggle * 0.1f;

        headRightLocalTransform_.translate.x += std::abs(headWiggle) * 0.2f;
        headRightLocalTransform_.translate.y -= headWiggle * 0.3f;
        headRightLocalTransform_.rotate.z = -headWiggle * 0.1f;

        headMidLocalTransform_.translate.y += std::sin(deathTimer_ * kHeadWiggleFreq * 1.2f) * kHeadWiggleAmp * 0.4f;
        headMidLocalTransform_.rotate.x = std::cos(deathTimer_ * kHeadWiggleFreq) * 0.15f;

        if (deathTimer_ >= kHoldDuration) {
            globalTransform_.translate = kCenterPos; // 正確な中央位置に戻す
            globalTransform_.rotate = { 0.0f, 0.0f, 0.0f }; // 回転もリセット
            deathPhase_ = DeathPhase::Exploding;    // 大爆発へ移行
        }
    } else if (deathPhase_ == DeathPhase::Exploding) {
        // マップ中央（ボスの親グローバル位置）
        Vector3 bossCenter = globalTransform_.translate;

        // 各パーツをマップ中央から放射状（斜め上空外側）へ勢いよくはじけ飛ばす
        auto explodePartRadial = [&](auto* part, const Transform& localT) {
            if (part) {
                // パーツのワールド位置を計算
                Matrix4x4 globalMat = Math::MakeAffineMatrix(globalTransform_.scale, globalTransform_.rotate, globalTransform_.translate);
                Vector3 partWorldPos = Math::Transform(localT.translate, globalMat);

                // マップ中央からパーツへの水平方向ベクトルを算出
                Vector3 dir = Math::Subtract(partWorldPos, bossCenter);
                dir.y = 0.0f; // 水平方向
                float len = Math::Length(dir);
                if (len < 0.1f) {
                    // 中心にほぼ位置するパーツ（真ん中の首など）は適宜前方方向などへ散らす
                    dir = {0.0f, 0.0f, 1.0f};
                } else {
                    dir = Math::Normalize(dir);
                }

                // 斜め上空へはじけ飛ぶようにY軸方向（上向き）の吹き飛び成分をブレンド
                dir.y = 0.8f;
                dir = Math::Normalize(dir);

                // パーツ自体の吹き飛び移動を開始（即時ボクセル化フラグをtrueに指定）
                part->OnDestroyed(dir, kExplosionBlowSpeed, true);
                
                // ボクセル粒子たちも吹き飛ぶ方向へ勢いよくScatter（飛散）させる
                OBB impactOBB;
                impactOBB.center = partWorldPos;
                impactOBB.orientations[0] = {1.0f, 0.0f, 0.0f};
                impactOBB.orientations[1] = {0.0f, 1.0f, 0.0f};
                impactOBB.orientations[2] = {0.0f, 0.0f, 1.0f};
                impactOBB.size = {3.0f, 3.0f, 3.0f}; 
                
                // ボクセルの初速ベクトルを合成
                Vector3 scatterVel = Math::Multiply(kExplosionBlowSpeed * 0.8f, dir);
                part->ScatterAt(scatterVel, impactOBB);
            }
        };

        for (int i = 0; i < 3; ++i) explodePartRadial(bodies_[i].get(), bodyLocalTransforms_[i]);
        explodePartRadial(headLeft_.get(), headLeftLocalTransform_);
        explodePartRadial(headMid_.get(), headMidLocalTransform_);
        explodePartRadial(headRight_.get(), headRightLocalTransform_);

        deathPhase_ = DeathPhase::Aftermath; // 余韻フェーズへ移行
        deathTimer_ = 0.0f;                  // 余韻用タイマーリセット
    } else if (deathPhase_ == DeathPhase::Aftermath) {
        deathTimer_ += deltaTime;

        if (deathTimer_ >= kAftermathDuration) {
            deathPhase_ = DeathPhase::None; // もう実行しない
            isActive_ = false;              // ここでボスを非アクティブ化し、クリア画面へ遷移開始！
        }
    }
}
