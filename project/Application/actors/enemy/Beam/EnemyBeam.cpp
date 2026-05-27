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
    telegraphObj_ = std::make_unique<PrimitiveObjects3DClass>();
    telegraphObj_->Initialize(PrimitiveType::Cube, "resources/whiteTexture.png");
    telegraphObj_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });
    telegraphObj_->SetCastShadows(false);
    if (engine) {
        // 予兆線を不透明オブジェクト（ビルなど）の後にUIキューとして描画し、
        // 深度テストを有効にする（DepthWrite::Disable）ことで、ビルの裏側は隠れ、表側は表示されるようになります。
        telegraphObj_->SetCustomPSO(engine->GetPSOManager()->GetPSO("Object3D", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
    }

    attackCylinder_ = std::make_shared<CylinderClass>();
    attackCylinder_->Initialize(false, false);
    attackCylinder_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    // 攻撃用 (外側の電撃オーラ)
    attackCylinderOuter_ = std::make_shared<CylinderClass>();
    attackCylinderOuter_->Initialize(false, false);
    attackCylinderOuter_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    attackCylinder_->SetCastShadows(false);

    chargeSphere_ = std::make_unique<PrimitiveObjects3DClass>();
    chargeSphere_->Initialize(PrimitiveType::Plane);
    chargeSphere_->SetColor({ 1.0f, 0.3f, 0.0f, 1.0f }); // レッドドラゴン風の赤オレンジコア
    chargeSphere_->SetCullingEnabled(false);
    if (engine) {
        chargeSphere_->SetCustomPSO(
            engine->GetPSOManager()->GetPSO("EnergyCore", BlendMode::kBlendModePremultiplied, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None)
        );
    }

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
            // --- 内側（コア）: EnergyBeam用の超高輝度レーザーパラメータ ---
            lightningParamsData_->color = { 1.0f, 0.4f, 0.1f, 1.0f }; // 外周オーラ（微か）
            lightningParamsData_->coreColor = { 1.0f, 1.0f, 0.8f, 1.0f }; // コア（白〜微黄色）
            lightningParamsData_->intensity = 5.0f; // オーラの輝度（外側があるので控えめ）
            lightningParamsData_->noiseThreshold = 0.3f;
            lightningParamsData_->coreIntensity = 40.0f; // コアの輝度（HDR白飛び）
            lightningParamsData_->coreThreshold = 0.8f; // コアの太さ（極太）
            lightningParamsData_->coreScale = 2.0f;
            lightningParamsData_->speed = 3.0f;      // 奔流の速度（かなり速く）
            lightningParamsData_->noiseScale = 1.0f; // ノイズの密度
            lightningParamsData_->spinSpeed = 5.0f;  // コアも少しだけ回転させる
            lightningParamsData_->twistScale = 3.0f; // 僅かにねじれるように
        }

        // 電撃用のパラメータリソースを作成 (外側オーラ・LightningCrawl用)
        lightningParamsOuterResource_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(LightningParams));
        lightningParamsOuterResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightningParamsOuterData_));
        if (lightningParamsOuterData_) {
            *lightningParamsOuterData_ = LightningParams();
            // --- 外側（オーラ）: LightningCrawl用の激しいバチバチ電撃パラメータ ---
            lightningParamsOuterData_->color = { 1.0f, 0.1f, 0.05f, 1.0f }; // 赤オレンジ
            lightningParamsOuterData_->coreColor = { 1.0f, 0.6f, 0.2f, 1.0f }; // 黄色コア
            lightningParamsOuterData_->intensity = 15.0f; // オーラの輝度（高め）
            lightningParamsOuterData_->noiseThreshold = 0.45f;
            lightningParamsOuterData_->coreIntensity = 10.0f;
            lightningParamsOuterData_->coreScale = 2.0f;
            lightningParamsOuterData_->speed = 0.5f;      // 前進は遅めにして横回転を強調
            lightningParamsOuterData_->noiseScale = 0.8f; // ★ノイズ密度を下げる（細かいとちぎれやすいため）
            lightningParamsOuterData_->spinSpeed = 10.0f; // 横回転（少し緩やかに）
            lightningParamsOuterData_->twistScale = 5.0f; // ★ねじれを大幅に下げる（強すぎるとエイリアスで線が切れる）
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

    Vector3 startPos = Math::Add(headPos, Math::Multiply(originOffset_, direction));

    if (isTelegraphActive_) {
        telegraphTransform_.scale = { telegraphThickness_, telegraphThickness_, beamLength_ };
        Vector3 telegraphCenter = Math::Add(startPos, Math::Multiply(beamLength_ * 0.5f, direction));
        telegraphTransform_.translate = telegraphCenter;
        telegraphTransform_.rotate = rotate;
        telegraphObj_->GetTransform().transform = telegraphTransform_;
        telegraphObj_->GetTransform().isDirty = true;
        telegraphObj_->Update();
    }

    if (isChargeSphereActive_ && chargeSphere_) {
        Transform t;
        t.scale = { chargeSphereScale_, chargeSphereScale_, chargeSphereScale_ };
        
        // startPosは頭の表面（中心から前方へ originOffset_ ずらした位置）
        float sphereForwardOffset = chargeSphereScale_ * 0.4f;
        t.translate = Math::Add(startPos, Math::Multiply(sphereForwardOffset, direction));

        // ビルボード回転（常にカメラを向く）
        Vector3 cameraPos = playerPos;
        if (engine_ && engine_->GetCameraManager() && engine_->GetCameraManager()->GetActiveCamera()) {
            cameraPos = engine_->GetCameraManager()->GetActiveCamera()->GetTranslate();
        }
        
        Vector3 toCamera = Math::Subtract(cameraPos, t.translate);
        Vector3 toCameraDir = Math::Normalize(toCamera);
        
        // 球（立体）から平面になったため、中心座標のままだと頭のモデルに埋まってしまいます。
        // 球の表面が手前に張り出していた分（スケールの約半分）、カメラ方向に少し引き寄せて埋まりを防ぎます。
        t.translate = Math::Add(t.translate, Math::Multiply(chargeSphereScale_ * 0.5f, toCameraDir));
        
        // 引き寄せた後の位置で再計算
        toCamera = Math::Subtract(cameraPos, t.translate);
        t.rotate.y = std::atan2(-toCamera.x, -toCamera.z);
        float distXZ = std::sqrt(toCamera.x * toCamera.x + toCamera.z * toCamera.z);
        t.rotate.x = std::atan2(toCamera.y, distXZ);
        t.rotate.z = 0.0f;

        chargeSphere_->GetTransform().transform = t;
        chargeSphere_->GetTransform().isDirty = true;
        chargeSphere_->Update();
    }

    if (isAttackActive_) {
        attackTimer_ += 1.0f / 60.0f; // 固定フレームでの簡易進捗
        float t = (std::min)(attackTimer_ / attackExpandTime_, 1.0f);
        
        // イーズアウト等で伸ばす(t * (2.0f - t))
        float easeT = t * (2.0f - t);
        // プレイヤーまでの距離ではなく、突き抜けるように beamLength_ を使用
        float currentLength = beamLength_ * easeT;

        // 地面(y=0.0f)との交差判定（ビームを床で止める）
        // 断面が浮いて見えないように、床よりもさらに長め（+20.0f）に突き抜けさせる
        if (direction.y < 0.0f) {
            float distanceToFloor = (0.0f - startPos.y) / direction.y;
            if (distanceToFloor > 0.0f) {
                currentLength = (std::min)(currentLength, distanceToFloor + 20.0f);
            }
        }

        // ビームの中心は伸びた距離の半分
        Vector3 currentCenter = Math::Add(startPos, Math::Multiply(currentLength * 0.5f, direction));

        attackTransform_.translate = currentCenter;
        attackTransform_.rotate = { rotate.x - Math::PI / 2.0f, rotate.y, rotate.z };
        
        // 内側（コア）の更新：外側より少し細めにする
        attackCylinder_->SetCenter(attackTransform_.translate);
        attackCylinder_->SetRotate(attackTransform_.rotate);
        attackCylinder_->SetRadius(attackThickness_ * 0.45f); // 内側コアは細く
        attackCylinder_->SetHeight(currentLength);
        attackCylinder_->Update();

        // 外側（電撃オーラ）的の更新：内側より太く覆う
        if (attackCylinderOuter_) {
            attackCylinderOuter_->SetCenter(attackTransform_.translate);
            attackCylinderOuter_->SetRotate(attackTransform_.rotate);
            attackCylinderOuter_->SetRadius(attackThickness_ * 0.85f); // 外側の電撃は太く
            attackCylinderOuter_->SetHeight(currentLength);
            attackCylinderOuter_->Update();
        }

        if (gpuParticle_) {
            // 基準となる太さ(第2形態相当の 4.0f)に対する今回の太さの倍率を計算
            float thicknessRatio = attackThickness_ / 4.0f;
            
            // スケール乗数に合わせて放出量や拡散を変動
            int emitCount = std::clamp(static_cast<int>(50.0f * thicknessRatio), 50, 400);

            // 平均寿命(例: 0.6秒)で到達点(currentLength)まで届くように速度を算出
            // ※HLSL側で velocity が「1フレームの移動量(dt不使用)」として加算されているため、60fps換算で割る
            float avgLife = 0.6f;
            float emitVelocity = (currentLength / avgLife) * (1.0f / 60.0f);
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
            gpuParticle_->SetBeamEmitter(startPos, direction, attackThickness_ * 0.5f, emitVelocity, emitSpread, emitCount, 0.01f);
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

    if (isChargeSphereActive_ && chargeSphere_) {
        chargeSphere_->Draw();
    }

    if (isTelegraphActive_ && telegraphObj_) {
        telegraphObj_->Draw(true); // ビルなどの遮蔽物に上書きされないようUIキュー（最前面）で描画
    }

    if (isAttackActive_) {
        // 1. 外側のバチバチ電撃（LightningCrawl）を先に描画
        if (attackCylinderOuter_ && lightningParamsOuterResource_) {
            attackCylinderOuter_->SetCustomPSO(engine->GetPSOManager()->GetPSO("LightningCrawl", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
            attackCylinderOuter_->SetCustomCBVAddress(lightningParamsOuterResource_->GetGPUVirtualAddress());
            
            // パケット（キュー）に積まれる描画ステートを設定
            engine->SetBlend(BlendMode::kBlendModeAdd);
            engine->SetDepthWrite(PSOManager::DepthWrite::Disable);
            engine->SetCull(PSOManager::CullMode::None);
            
            attackCylinderOuter_->Draw(true); // UIキューで描画
        }

        // 2. 内側の極太レーザーコア（EnergyBeam）を描画
        if (attackCylinder_ && lightningParamsResource_) {
            attackCylinder_->SetCustomPSO(engine->GetPSOManager()->GetPSO("EnergyBeam", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
            attackCylinder_->SetCustomCBVAddress(lightningParamsResource_->GetGPUVirtualAddress());
            
            attackCylinder_->Draw(true); // UIキューで描画
        }

        // 状態を元に戻す
        engine->SetBlend(BlendMode::kBlendModeNormal);
        engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
        engine->SetCull(PSOManager::CullMode::Back);

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