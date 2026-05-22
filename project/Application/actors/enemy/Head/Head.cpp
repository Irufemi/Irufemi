#include "Head.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"
#include "Renderer/VoxelParticle/VoxelParticleSystem.h"
#include "Renderer/Object3D/Primitive/CylinderClass.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "actors/enemy/EnemyParameters.h"
#include "Engine/Core/Math/Math.h"
#include "IrufemiEngine.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "Renderer/Particle/ParticleSystem.h"
#include "Renderer/Particle/Data/Particle.h"
#include <algorithm>
#include <cmath>

Head::Head() {}
Head::~Head() {}

void Head::Initialize(const Vector3& initialPos) {
  obj_ = std::make_unique<ObjClass>();
  obj_->Initialize("enemy/head.obj");
  basePosition_ = initialPos;
  obj_->SetPosition(basePosition_);
  obj_->SetColor(baseColor_);

  voxelSystem_ = std::make_unique<VoxelParticleSystem>();
  voxelSystem_->Initialize("enemy/head.obj", {32, 32, 32});

  // ロケット噴射炎の初期化（シリンダーメッシュを使用）
  thrusterFlame_ = std::make_shared<CylinderClass>();
  // 蓋なし（hasTop=false, hasBottom=false）の筒として作成
  thrusterFlame_->Initialize(false, false, "resources/noise0.png");
  thrusterFlame_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
  thrusterFlame_->SetCastShadows(false);

  // パーティクルシステムの初期化
  flameParticle_ = std::make_unique<ParticleSystem>();
  flameParticle_->Initialize("resources/circle.png", ParticleType::kMissileFire);

  smokeParticle_ = std::make_unique<ParticleSystem>();
  smokeParticle_->Initialize("resources/circle.png", ParticleType::kGroundSmoke);

}

void Head::Update() {
  if (isBlownAway_) {
    // 吹き飛び中の移動
    basePosition_ = Math::Add(basePosition_, blowVelocity_);
    
    // 壁との判定（フィールドサイズはX,Z ±100）
    const float kFieldBound = 100.0f;
    const float kRadius = 2.0f; // 部位のだいたいの半径
    
    if (basePosition_.x - kRadius < -kFieldBound) {
        basePosition_.x = -kFieldBound + kRadius;
        blowVelocity_.x *= -1.0f;
    } else if (basePosition_.x + kRadius > kFieldBound) {
        basePosition_.x = kFieldBound - kRadius;
        blowVelocity_.x *= -1.0f;
    }
    
    if (basePosition_.z - kRadius < -kFieldBound) {
        basePosition_.z = -kFieldBound + kRadius;
        blowVelocity_.z *= -1.0f;
    } else if (basePosition_.z + kRadius > kFieldBound) {
        basePosition_.z = kFieldBound - kRadius;
        blowVelocity_.z *= -1.0f;
    }

    transform_.translate = basePosition_;
    if (obj_) {
      obj_->SetTransform(transform_);
    }
    
    // 消滅タイマーを進める
    float prevTimer = disappearTimer_;
    disappearTimer_ += 1.0f / 60.0f;
    blowTimer_ += 1.0f / 60.0f;

    if (prevTimer < EnemyParameters::GetInstance()->GetDisappearTime() &&
        disappearTimer_ >= EnemyParameters::GetInstance()->GetDisappearTime()) {
        // 爆散！
        if (voxelSystem_) {
            // 端から崩れる燃え尽きエフェクトを指定
            voxelSystem_->SetParticleType(VoxelParticleSystem::ParticleType::EnemyBurnout);
            voxelSystem_->Explode(basePosition_, blowVelocity_, transform_.rotate, transform_.scale);
        }
    }
  }

  if (voxelSystem_) {
      voxelSystem_->Update(1.0f / 60.0f);
  }

  if (damageFlashTimer_ > 0.0f) {
    damageFlashTimer_ -= 1.0f / 60.0f;
    if (damageFlashTimer_ < 0.0f) {
      damageFlashTimer_ = 0.0f;
    }
  }

  if (obj_ && !IsCompletelyDead()) {
    Vector4 color = baseColor_;
    float duration = EnemyParameters::GetInstance()->GetDamageFlashDuration();
    if (damageFlashTimer_ > 0.0f && duration > 0.0f) {
      const Vector4 damageColor = EnemyParameters::GetInstance()->GetDamageFlashColor();
      float t = damageFlashTimer_ / duration;
      if (t < 0.0f) {
        t = 0.0f;
      } else if (t > 1.0f) {
        t = 1.0f;
      }
      color.x = baseColor_.x + (damageColor.x - baseColor_.x) * t;
      color.y = baseColor_.y + (damageColor.y - baseColor_.y) * t;
      color.z = baseColor_.z + (damageColor.z - baseColor_.z) * t;
      color.w = baseColor_.w + (damageColor.w - baseColor_.w) * t;
    }
    obj_->SetColor(color);
  }

  // ロケット噴射炎の更新
  if (thrusterFlame_ && isPhase2_ && !isBlownAway_) {
    // 首の下端から噴射するように配置
    // cylinderの高さ方向の中心が center なので、高さを考慮してオフセット
    float flameHeight = 4.0f * transform_.scale.y;
    float flameRadius = 0.8f * transform_.scale.x;
    
    // 向きは首の回転に合わせる
    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(transform_.rotate);
    Vector3 localDown = { -rotMat.m[1][0], -rotMat.m[1][1], -rotMat.m[1][2] }; // Y軸の下方向
    
    // 中心位置を計算（首の原点から少し下にずらした場所がシリンダーの中心）
    Vector3 flameCenter = Math::Add(drawPosition_, Math::Multiply(flameHeight * 0.5f, localDown));
    
    thrusterFlame_->SetCenter(flameCenter);
    thrusterFlame_->SetRotate(transform_.rotate);
    thrusterFlame_->SetRadius(flameRadius);
    thrusterFlame_->SetHeight(flameHeight);
    thrusterFlame_->Update();

    // パーティクルの放出（中心から首の断面付近へオフセット）
    float offsetAmount = 1.0f * transform_.scale.y; 
    Vector3 emissionStartPos = Math::Add(drawPosition_, Math::Multiply(offsetAmount, localDown));

    if (flameParticle_) {
      flameParticle_->SetEmitterPosition(emissionStartPos);
      flameParticle_->SetEmitterArea({1.5f, 1.5f, 1.5f}); // 炎の発生範囲
      // 噴射方向を中心に少しブレさせる
      Vector3 baseFlameVel = Math::Multiply(12.5f, localDown);
      float flameSpread = 1.5f;
      Vector3 minVel = { baseFlameVel.x - flameSpread, baseFlameVel.y - flameSpread, baseFlameVel.z - flameSpread };
      Vector3 maxVel = { baseFlameVel.x + flameSpread, baseFlameVel.y + flameSpread, baseFlameVel.z + flameSpread };
      flameParticle_->SetEmitterVelocity(minVel, maxVel);
      flameParticle_->PlayHitEffect(emissionStartPos, 2);
    }
    if (smokeParticle_) {
      smokeParticle_->SetEmitterPosition(emissionStartPos);
      smokeParticle_->SetEmitterArea({2.5f, 2.5f, 2.5f}); // 煙の発生範囲
      // 煙は全方位へ大きく広がりつつ、下方向への勢いも持たせる
      Vector3 baseSmokeVel = Math::Multiply(7.5f, localDown);
      float smokeSpread = 3.5f; // 横方向への広がりを強くする
      Vector3 minVel = { baseSmokeVel.x - smokeSpread, baseSmokeVel.y - smokeSpread, baseSmokeVel.z - smokeSpread };
      Vector3 maxVel = { baseSmokeVel.x + smokeSpread, baseSmokeVel.y + smokeSpread, baseSmokeVel.z + smokeSpread };
      smokeParticle_->SetEmitterVelocity(minVel, maxVel);
      smokeParticle_->PlayHitEffect(emissionStartPos, 10); // さらに発生数を増やしてモクモクにする
    }
  }

  // パーティクルシステムの更新
  if (flameParticle_) {
    flameParticle_->Update();
  }
  if (smokeParticle_) {
    smokeParticle_->Update();
  }
}

void Head::Draw(IrufemiEngine* engine) {
  bool modelGone = disappearTimer_ >= EnemyParameters::GetInstance()->GetDisappearTime();
  if (obj_ && !modelGone) {
      engine->SetBlend(BlendMode::kBlendModeNormal);
      engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
      engine->SetCull(PSOManager::CullMode::Back);
    obj_->Draw();
  }
  if (voxelSystem_) {
      // ボクセル描画用の状態を明示的にセット
      engine->SetBlend(BlendMode::kBlendModeNormal);
      engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
      engine->SetCull(PSOManager::CullMode::Back);
      voxelSystem_->Draw();
  }

  // ロケット噴射炎の描画
  if (thrusterFlame_ && isPhase2_ && !isBlownAway_) {
    // カスタムPSO(RocketFlame)を適用
    auto pso = engine->GetPSOManager()->GetPSO("RocketFlame", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None);
    thrusterFlame_->SetCustomPSO(pso);

    engine->SetBlend(BlendMode::kBlendModeAdd);
    engine->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine->SetCull(PSOManager::CullMode::None);

    thrusterFlame_->Draw();


    // 状態を元に戻す
    engine->SetBlend(BlendMode::kBlendModeNormal);
    engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine->SetCull(PSOManager::CullMode::Back);
  }

  // パーティクルの描画
  if (isPhase2_ && !isBlownAway_) {
    if (flameParticle_) {
      flameParticle_->Draw();
    }
    if (smokeParticle_) {
      smokeParticle_->Draw();
    }
  }
}

void Head::SetPosition(const Vector3& pos) {
  basePosition_ = pos;
  if (obj_) {
    obj_->SetPosition(pos);
  }
}

void Head::SetTransform(const Transform& transform, const Vector3* drawWorldPos) {
  if (!isBlownAway_) {
    transform_ = transform;
    basePosition_ = transform.translate;
    if (obj_) {
      if (drawWorldPos) {
        drawPosition_ = *drawWorldPos;
        Transform drawTransform = transform;
        drawTransform.translate = *drawWorldPos;
        obj_->SetTransform(drawTransform);
      } else {
        drawPosition_ = transform.translate;
        obj_->SetTransform(transform);
      }
    }
  }
}

void Head::OnDestroyed(const Vector3& attackDir, float blowSpeed, bool immediateVoxel) {
    if (isBlownAway_) return;
    
    isBlownAway_ = true;
    disappearTimer_ = immediateVoxel ? EnemyParameters::GetInstance()->GetDisappearTime() : 0.0f;
    blowTimer_ = 0.0f;
    blowVelocity_ = Math::Multiply(blowSpeed, attackDir);
    if (!immediateVoxel) {
        blowVelocity_.y = 0.0f; // Y軸方向への吹き飛びを完全に無くす
    }
}

void Head::ResetBlow() {
    isBlownAway_ = false;
    disappearTimer_ = 0.0f;
    blowTimer_ = 0.0f;
    blowVelocity_ = {0.0f, 0.0f, 0.0f};
}

bool Head::IsCompletelyDead() const {
    if (!isBlownAway_) return false;
    // モデルが消滅し、かつ VoxelParticle も終了していれば完全に死んだとみなす
    bool modelGone = disappearTimer_ >= EnemyParameters::GetInstance()->GetDisappearTime();
    bool voxelActive = voxelSystem_ && voxelSystem_->IsActive();
    return modelGone && !voxelActive;
}

OBB Head::GetOBB() const {
    if (isBlownAway_ && disappearTimer_ >= EnemyParameters::GetInstance()->GetDisappearTime()) {
        return OBB{}; // モデル消滅後は判定を消す
    }

    OBB obb;
    obb.center = transform_.translate;
    
    Matrix4x4 rotateMat = Math::MakeRotateXYZMatrix(transform_.rotate);
    obb.orientations[0] = { rotateMat.m[0][0], rotateMat.m[0][1], rotateMat.m[0][2] };
    obb.orientations[1] = { rotateMat.m[1][0], rotateMat.m[1][1], rotateMat.m[1][2] };
    obb.orientations[2] = { rotateMat.m[2][0], rotateMat.m[2][1], rotateMat.m[2][2] };
    
    Vector3 baseSize = EnemyParameters::GetInstance()->GetHeadOBBSize();
    obb.size = { baseSize.x * transform_.scale.x, baseSize.y * transform_.scale.y, baseSize.z * transform_.scale.z };
    
    // 当たり判定をモデルの下側（原点）から上方向にシフトさせる
    float offsetY = baseSize.y * transform_.scale.y * 0.7f;
    Vector3 centerOffset = Math::Multiply(offsetY, obb.orientations[1]);
    obb.center = Math::Add(obb.center, centerOffset);

    return obb;
}

bool Head::ApplyDamage(int damage) {
  hp_ -= damage;
  if (hp_ < 0) {
    hp_ = 0;
  }
  damageFlashTimer_ = EnemyParameters::GetInstance()->GetDamageFlashDuration();
  return true;
}

void Head::ScatterAt(const Vector3& velocity, const OBB& collisionArea) {
  if (voxelSystem_) {
    voxelSystem_->SetParameters(VoxelParticleSystem::VoxelEmitterParams::FineScatter());
    voxelSystem_->CollisionScatter(basePosition_, velocity, transform_.rotate,
                                   transform_.scale, collisionArea);
  }
}
