#include "HeadMid.h"
#include "Renderer/VoxelParticle/VoxelParticleSystem.h"
#include "camera/Camera.h"
#include "actors/enemy/EnemyParameters.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "IrufemiEngine.h"
#include <algorithm>
#include <cmath>

HeadMid::~HeadMid() {}

void HeadMid::Initialize(Camera* camera, const Vector3& initialPos) {
  obj_ = std::make_unique<ObjClass>();
  obj_->Initialize(camera, "enemy/head.obj");
  basePosition_ = initialPos;
  obj_->SetPosition(basePosition_);
  obj_->SetColor(baseColor_);

  voxelSystem_ = std::make_unique<VoxelParticleSystem>();
  voxelSystem_->Initialize("enemy/head.obj", {16, 16, 16}, camera);
}

void HeadMid::Update() {
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

    if (prevTimer < EnemyParameters::GetInstance()->GetDisappearTime() &&
        disappearTimer_ >= EnemyParameters::GetInstance()->GetDisappearTime()) {
        // 爆散！
        if (voxelSystem_) {
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
    obj_->Update();
  }
}

void HeadMid::Draw(IrufemiEngine* engine) {
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
}

void HeadMid::SetPosition(const Vector3& pos) {
  basePosition_ = pos;
  if (obj_) {
    obj_->SetPosition(pos);
  }
}

void HeadMid::SetTransform(const Transform& transform, const Vector3* drawWorldPos) {
  if (!isBlownAway_) {
    transform_ = transform;
    basePosition_ = transform.translate;
    if (obj_) {
      if (drawWorldPos) {
        Transform drawTransform = transform;
        drawTransform.translate = *drawWorldPos;
        obj_->SetTransform(drawTransform);
      } else {
        obj_->SetTransform(transform);
      }
    }
  }
}

void HeadMid::OnDestroyed(const Vector3& attackDir, float blowSpeed) {
    if (isBlownAway_) return;
    
    isBlownAway_ = true;
    disappearTimer_ = 0.0f;
    blowVelocity_ = Math::Multiply(blowSpeed, attackDir);
    blowVelocity_.y = 0.0f; // Y軸方向への吹き飛びを完全に無くす
}

bool HeadMid::IsCompletelyDead() const {
    if (!isBlownAway_) return false;
    // モデルが消滅し、かつ VoxelParticle も終了していれば完全に死んだとみなす
    bool modelGone = disappearTimer_ >= EnemyParameters::GetInstance()->GetDisappearTime();
    bool voxelActive = voxelSystem_ && voxelSystem_->IsActive();
    return modelGone && !voxelActive;
}

OBB HeadMid::GetOBB() const {
    OBB obb;
    obb.center = transform_.translate;
    
    Matrix4x4 rotateMat = Math::MakeRotateXYZMatrix(transform_.rotate);
    obb.orientations[0] = { rotateMat.m[0][0], rotateMat.m[0][1], rotateMat.m[0][2] };
    obb.orientations[1] = { rotateMat.m[1][0], rotateMat.m[1][1], rotateMat.m[1][2] };
    obb.orientations[2] = { rotateMat.m[2][0], rotateMat.m[2][1], rotateMat.m[2][2] };
    
    obb.size = EnemyParameters::GetInstance()->GetHeadOBBSize();
    return obb;
}

void HeadMid::ApplyDamage(int damage) {
  hp_ -= damage;
  if (hp_ < 0) {
    hp_ = 0;
  }
  damageFlashTimer_ = EnemyParameters::GetInstance()->GetDamageFlashDuration();
}
