#include "EnemyBeam.h"
#include "Core/Math/Math.h"
#include "IrufemiEngine.h"
#include <cmath>

void EnemyBeam::Initialize(Camera* camera, IrufemiEngine* engine) {
    telegraphObj_ = std::make_unique<ObjClass>();
    telegraphObj_->Initialize(camera, "sample/block.obj");
    telegraphObj_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });

    attackCylinder_ = std::make_unique<CylinderClass>();
    attackCylinder_->Initialize(camera);
    attackCylinder_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });

    // トランスフォームの初期化（Updateで確定するため、ここではゼロクリア）
    telegraphTransform_.translate = { 0, 0, 0 };
    telegraphTransform_.rotate = { 0, 0, 0 };
    telegraphTransform_.scale = { 1.0f, 1.0f, 1.0f };
    telegraphForwardOffset_ = beamLength_ * 0.5f;

    attackTransform_.translate = { 0, 0, 0 };
    attackTransform_.rotate = { 0, 0, 0 };
    attackTransform_.scale = { 1.0f, 1.0f, 1.0f };
    attackForwardOffset_ = beamLength_ * 0.5f;

    if (engine) {
        lightningParamsResource_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(LightningParams));
        lightningParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightningParamsData_));
        if (lightningParamsData_) {
            *lightningParamsData_ = LightningParams();
            // レッドドラゴンのブレス風（赤＋黄色コア）のプラズマ表現
            lightningParamsData_->color = { 1.0f, 0.15f, 0.05f, 1.0f };
            lightningParamsData_->coreColor = { 1.0f, 0.8f, 0.2f, 1.0f };
            lightningParamsData_->intensity = 6.0f;
            lightningParamsData_->noiseThreshold = 0.3f;
            lightningParamsData_->coreIntensity = 5.0f;
            lightningParamsData_->coreScale = 2.0f;
            lightningParamsData_->speed = 4.0f;      // 模様の流れを速く
            lightningParamsData_->noiseScale = 3.0f; // 模様の密度を上げる
        }

        gpuParticle_ = std::make_unique<GPUParticleSystem>();
        gpuParticle_->Initialize(camera, "resources/circle.png");
        gpuParticle_->SetColor({ 1.0f, 0.45f, 0.1f, 1.0f }); // 鮮やかなオレンジプラズマ
        gpuParticle_->SetEmit(false);
    }

    isExpired_ = false;
    isTelegraphActive_ = false;
    isAttackActive_ = false;
    attackTimer_ = 0.0f;
}

void EnemyBeam::Update(const Vector3& headPos, const Vector3& playerPos) {
    Vector3 diff = Math::Subtract(playerPos, headPos);
    float distance = Math::Length(diff);
    Vector3 direction = Math::Normalize(diff);

    Vector3 center = {
        (headPos.x + playerPos.x) * 0.5f,
        (headPos.y + playerPos.y) * 0.5f,
        (headPos.z + playerPos.z) * 0.5f
    };

    Vector3 rotate = { 0.0f, 0.0f, 0.0f };
    rotate.y = std::atan2(direction.x, direction.z);
    float distXZ = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    rotate.x = std::atan2(-direction.y, distXZ);

    if (isTelegraphActive_) {
        telegraphTransform_.scale = { telegraphThickness_, telegraphThickness_, distance };
        telegraphTransform_.translate = center;
        telegraphTransform_.rotate = rotate;
        telegraphObj_->SetTransform(telegraphTransform_);
        telegraphObj_->Update();
    }

    if (isAttackActive_) {
        attackTimer_ += 1.0f / 60.0f; // 固定フレームでの簡易進捗
        float t = (std::min)(attackTimer_ / attackExpandTime_, 1.0f);
        
        // イーズアウト等で伸ばす(t * (2.0f - t))
        float easeT = t * (2.0f - t);
        // プレイヤーまでの距離ではなく、突き抜けるように beamLength_ を使用
        float currentDistance = beamLength_ * easeT;

        // ビームの中心は伸びた距離の半分
        Vector3 currentCenter = Math::Add(headPos, Math::Multiply(currentDistance * 0.5f, direction));

        attackTransform_.translate = currentCenter;
        attackTransform_.rotate = rotate;
        
        attackCylinder_->SetCenter(currentCenter);
        // Cylinder用回転補正
        attackCylinder_->SetRotate({ rotate.x - Math::PI / 2.0f, rotate.y, rotate.z });
        attackCylinder_->SetRadius(attackThickness_ * 0.5f);
        attackCylinder_->SetHeight(currentDistance);
        attackCylinder_->Update();

        if (gpuParticle_) {
            // 放出設定：物量と勢いを大幅に強化してレッドドラゴンのブレス感を演出
            // count: 50 -> 200, velocity: 1.2 -> 2.8, spread: 0.12
            gpuParticle_->SetBeamEmitter(headPos, direction, attackThickness_ * 0.5f, 2.8f, 0.12f, 200, 0.01f);
            gpuParticle_->SetEmit(true);
            gpuParticle_->Update();
        }
    } else {
        attackTimer_ = 0.0f;
        if (gpuParticle_) {
            gpuParticle_->SetEmit(false);
            gpuParticle_->Update();
        }
    }
}

void EnemyBeam::Draw(IrufemiEngine* engine) {
    if (!engine) return;

    if (isTelegraphActive_ && telegraphObj_) {
        engine->ApplyPSO(); // 予兆は通常描画
        telegraphObj_->Draw();
    }

    if (isAttackActive_ && attackCylinder_) {
        // 電撃・プラズマ表現のPSO適用
        engine->SetBlend(BlendMode::kBlendModeAdd);
        engine->SetDepthWrite(PSOManager::DepthWrite::Disable);
        engine->SetCull(PSOManager::CullMode::None);

        engine->ApplyLightningCrawlPSO();
        if (lightningParamsResource_) {
            engine->BindLightningParams(lightningParamsResource_->GetGPUVirtualAddress());
        }

        attackCylinder_->Draw();

        // パーティクルの描画（UpdateでのCSディスパッチ含む）
        if (gpuParticle_) {
            gpuParticle_->Draw();
        }

        // 状態を戻す
        engine->SetBlend(BlendMode::kBlendModeNormal);
        engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
        engine->SetCull(PSOManager::CullMode::Back);
    }
}

OBB EnemyBeam::GetOBB() const {
    OBB obb;
    obb.center = attackCylinder_->GetCenter();
    obb.orientations[0] = attackCylinder_->GetRight();
    obb.orientations[1] = attackCylinder_->GetDirection();
    obb.orientations[2] = attackCylinder_->GetUp();
    // 演出（ビームの伸び）に合わせて、判定の長さと太さを同期させる
    float currentHeight = attackCylinder_->GetInfo().height;
    obb.size = { attackThickness_ * 0.5f, attackThickness_ * 0.5f, currentHeight * 0.5f };
    return obb;
}