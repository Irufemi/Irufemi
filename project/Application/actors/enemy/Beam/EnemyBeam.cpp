#include "EnemyBeam.h"
#include "Core/Math/Math.h"
#include "IrufemiEngine.h"
#include <cmath>
#include <algorithm>
#include "Engine/Graphics/DirectX/DirectXCommon.h"

EnemyBeam::~EnemyBeam() {
    if (lightningParamsResource_ && engine_ && engine_->GetDirectXCommon()) {
        engine_->GetDirectXCommon()->ReleaseAfterFence(lightningParamsResource_);
    }
}

void EnemyBeam::Initialize(IrufemiEngine* engine) {
    telegraphObj_ = std::make_unique<ObjClass>();
    telegraphObj_->Initialize("sample/block.obj");
    telegraphObj_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });
    telegraphObj_->SetCastShadows(false);

    attackCylinder_ = std::make_shared<CylinderClass>();
    attackCylinder_->Initialize();
    attackCylinder_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });
    attackCylinder_->SetCastShadows(false);

    // トランスフォームの初期化（Updateで確定するため、ここではゼロクリア）
    telegraphTransform_.translate = { 0, 0, 0 };
    telegraphTransform_.rotate = { 0, 0, 0 };
    telegraphTransform_.scale = { 1.0f, 1.0f, 1.0f };
    telegraphForwardOffset_ = beamLength_ * 0.5f;

    attackTransform_.translate = { 0, 0, 0 };
    attackTransform_.rotate = { 0, 0, 0 };
    attackTransform_.scale = { 1.0f, 1.0f, 1.0f };
    attackForwardOffset_ = beamLength_ * 0.5f;

    engine_ = engine;

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
        gpuParticle_->Initialize("resources/circle.png");
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
        telegraphTransform_.scale = { telegraphThickness_, telegraphThickness_, beamLength_ };
        Vector3 telegraphCenter = Math::Add(headPos, Math::Multiply(beamLength_ * 0.5f, direction));
        telegraphTransform_.translate = telegraphCenter;
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

        // 地面(y=0.0f)との交差判定（ビームを床で止める）
        if (direction.y < 0.0f) {
            float distanceToFloor = (0.0f - headPos.y) / direction.y;
            if (distanceToFloor > 0.0f) {
                currentDistance = (std::min)(currentDistance, distanceToFloor);
            }
        }

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
            // 基準となる太さ(第2形態相当の 4.0f)に対する今回の太さの倍率を計算
            float thicknessRatio = attackThickness_ / 4.0f;
            
            // スケール乗数に合わせて放出量や拡散を変動
            int emitCount = std::clamp(static_cast<int>(50.0f * thicknessRatio), 50, 400);

            // 平均寿命(例: 0.6秒)で到達点(currentDistance)まで届くように速度を算出
            // ※HLSL側で velocity が「1フレームの移動量(dt不使用)」として加算されているため、60fps換算で割る
            float avgLife = 0.6f;
            float emitVelocity = (currentDistance / avgLife) * (1.0f / 60.0f);
            float emitSpread = 0.12f * std::sqrt(thicknessRatio); // 太いほど拡散させる

            // 目標地点でパーティクルが消えるように寿命を設定
            gpuParticle_->SetParticleLife(avgLife - 0.2f, avgLife + 0.2f);

            // シリンダーのスケールに合わせて、放出されるパーティクル自体の大きさも比例させる
            float sMin = 0.2f * thicknessRatio;
            float sMax = 0.5f * thicknessRatio;
            float eMin = 0.01f * thicknessRatio;
            float eMax = 0.1f * thicknessRatio;
            gpuParticle_->SetParticleScale(
                { sMin, sMin, sMin }, { sMax, sMax, sMax },
                { eMin, eMin, eMin }, { eMax, eMax, eMax }
            );

            // 放出設定：物量と勢いを大幅に強化してレッドドラゴンのブレス感を演出
            gpuParticle_->SetBeamEmitter(headPos, direction, attackThickness_ * 0.5f, emitVelocity, emitSpread, emitCount, 0.01f);
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
        telegraphObj_->Draw();
    }

    if (isAttackActive_ && attackCylinder_) {
        // attackCylinder_->Draw() だと RenderQueue に回されてしまい専用PSOが上書きされるため、
        // 不透明描画の後に専用PSOを適用して描画するよう SubmitPostRender に積む
        attackCylinder_->SyncBeforeDraw();

        engine->GetDrawManager()->SubmitPostRender([
            engine, 
            capturedCylinder = attackCylinder_, 
            capturedLightning = lightningParamsResource_
        ]() {
            engine->SetBlend(BlendMode::kBlendModeAdd);
            engine->SetDepthWrite(PSOManager::DepthWrite::Disable);
            engine->SetCull(PSOManager::CullMode::None);

            engine->ApplyLightningCrawlPSO();
            if (capturedLightning) {
                engine->BindLightningParams(capturedLightning->GetGPUVirtualAddress());
            }

            DrawManager::Standard3DPacket packet{};
            packet.resource = capturedCylinder->GetD3D12Resource();
            packet.blendMode = BlendMode::kBlendModeAdd;
            packet.depthWrite = PSOManager::DepthWrite::Disable;
            packet.cullMode = PSOManager::CullMode::None;
            engine->GetDrawManager()->DrawStandard3D(packet);

            // 状態を戻す
            engine->SetBlend(BlendMode::kBlendModeNormal);
            engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
            engine->SetCull(PSOManager::CullMode::Back);
        });

        // パーティクルの描画（UpdateでのCSディスパッチ含む）
        if (gpuParticle_) {
            OutputDebugStringA("[EnemyBeam] Draw - Calling gpuParticle_->Draw()\n");
            // gpuParticle_ も Queue に入るため、ここでは通常通り呼ぶ
            gpuParticle_->Draw();
        }
    }
}

OBB EnemyBeam::GetOBB() const {
    if (!isAttackActive_) {
        return OBB{}; // 攻撃終了後は判定を消す
    }

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