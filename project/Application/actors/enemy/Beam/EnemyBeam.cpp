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
    if (lightningParamsOuterResource_) {
        lightningParamsOuterResource_->Unmap(0, nullptr);
        engine_->GetDirectXCommon()->ReleaseAfterFence(lightningParamsOuterResource_);
    }
    if (aoeParamsResourceCylinder_) {
        aoeParamsResourceCylinder_->Unmap(0, nullptr);
        engine_->GetDirectXCommon()->ReleaseAfterFence(aoeParamsResourceCylinder_);
    }
    if (aoeParamsResourceGround_) {
        aoeParamsResourceGround_->Unmap(0, nullptr);
        engine_->GetDirectXCommon()->ReleaseAfterFence(aoeParamsResourceGround_);
    }
}

void EnemyBeam::Initialize(IrufemiEngine* engine) {
    telegraphCylinder_ = std::make_shared<CylinderClass>();
    telegraphCylinder_->Initialize(false, false);
    telegraphCylinder_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
    telegraphCylinder_->SetCastShadows(false);
    telegraphCylinder_->SetCullingEnabled(false);

    telegraphGroundAOE_ = std::make_unique<PrimitiveObjects3DClass>();
    telegraphGroundAOE_->Initialize(PrimitiveType::Plane);
    telegraphGroundAOE_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
    telegraphGroundAOE_->SetCullingEnabled(false);

    if (engine) {
        auto aoePSO = engine->GetPSOManager()->GetPSO("AOEWarning", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None);
        telegraphCylinder_->SetCustomPSO(aoePSO);
        telegraphGroundAOE_->SetCustomPSO(aoePSO);

        aoeParamsResourceCylinder_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(AOEParams));
        aoeParamsResourceCylinder_->Map(0, nullptr, reinterpret_cast<void**>(&aoeParamsDataCylinder_));
        if (aoeParamsDataCylinder_) {
            *aoeParamsDataCylinder_ = AOEParams();
            aoeParamsDataCylinder_->shapeType = 2; // Cylinder専用
        }
        telegraphCylinder_->SetCustomCBVAddress(aoeParamsResourceCylinder_->GetGPUVirtualAddress());

        aoeParamsResourceGround_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(AOEParams));
        aoeParamsResourceGround_->Map(0, nullptr, reinterpret_cast<void**>(&aoeParamsDataGround_));
        if (aoeParamsDataGround_) {
            *aoeParamsDataGround_ = AOEParams();
            aoeParamsDataGround_->shapeType = 0; // Radial
        }
        telegraphGroundAOE_->SetCustomCBVAddress(aoeParamsResourceGround_->GetGPUVirtualAddress());
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
    // CylinderはY軸方向に伸びているため、Z軸方向に倒すために90度(PI/2)足す
    rotate.x += Math::PI / 2.0f;

    Vector3 startPos = Math::Add(headPos, Math::Multiply(originOffset_, direction));

    if (isTelegraphActive_) {
        // 進行度の更新 (固定フレーム想定。発射までの時間に合わせる)
        telegraphProgress_ += 1.0f / 60.0f;
        // 警告が進むにつれて ratio が 0.0 -> 1.0 に近づく (徐々に赤くなる)
        float ratio = (std::min)(telegraphProgress_ * 0.7f, 1.0f);

        // 1. 軌道シリンダーの更新
        telegraphCylinder_->SetCenter(Math::Add(startPos, Math::Multiply(beamLength_ * 0.5f, direction)));
        telegraphCylinder_->SetRotate(rotate);
        // 予兆円柱の太さは画面の占有を防ぐため実際の1/4の太さに縮小
        telegraphCylinder_->SetRadius(telegraphThickness_ * 0.125f);
        telegraphCylinder_->SetHeight(beamLength_);
        telegraphCylinder_->Update();
        
        // 専用のAOEParamsを更新
        if (aoeParamsDataCylinder_) {
            aoeParamsDataCylinder_->warningRatio = ratio;
        }

        // 2. 地面AOEサークルの更新
        if (direction.y < -0.001f) {
            float distToFloor = (0.0f - startPos.y) / direction.y;
            if (distToFloor > 0.0f) {
                // 距離が遠すぎる場合でも消えないように、最大距離でクランプする
                float renderDist = (std::min)(distToFloor, beamLength_);
                Vector3 floorPos = Math::Add(startPos, Math::Multiply(renderDist, direction));
                // クランプした場合、Yが浮く可能性があるためY=0.05fに固定（Zファイティング防止）
                floorPos.y = 0.05f;

                Transform t;
                t.translate = floorPos;
                
                // direction（ビーム方向）のXZ平面での角度（Yaw）を計算
                float yaw = std::atan2(direction.x, direction.z);
                
                // Planeを地面に寝かせるためX軸で90度回転(Rx=90)。
                // その後、ワールドのY軸を中心に回転させることで、地面に這わせたままビームの方向へ向きを変える(Ry=yaw)。
                t.rotate = { Math::PI / 2.0f, yaw, 0.0f };
                
                // ビームが斜めに刺さる場合、断面は楕円になるためスケールを調整
                // PlaneのローカルY軸がビームの進行方向を向くため、Yスケールを伸ばす
                float cosTheta = std::abs(direction.y);
                float scaleZ = telegraphThickness_ / (std::max)(cosTheta, 0.01f);
                t.scale = { telegraphThickness_, scaleZ, 1.0f };
                
                telegraphGroundAOE_->GetTransform().transform = t;
                telegraphGroundAOE_->GetTransform().isDirty = true;
                telegraphGroundAOE_->Update();

                // 専用のAOEParamsを更新
                if (aoeParamsDataGround_) {
                    aoeParamsDataGround_->warningRatio = ratio;
                }
            } else {
                telegraphGroundAOE_->GetTransform().transform.scale = { 0,0,0 };
                telegraphGroundAOE_->GetTransform().isDirty = true;
                telegraphGroundAOE_->Update();
            }
        } else {
            telegraphGroundAOE_->GetTransform().transform.scale = { 0,0,0 };
            telegraphGroundAOE_->GetTransform().isDirty = true;
            telegraphGroundAOE_->Update();
        }
    } else {
        telegraphProgress_ = 0.0f;
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
        attackTransform_.rotate = rotate;
        
        // 内側（コア）の更新：外側より少し細めにする
        attackCylinder_->SetCenter(attackTransform_.translate);
        attackCylinder_->SetRotate(attackTransform_.rotate);
        attackCylinder_->SetRadius(attackThickness_ * 0.3f); // ★コアを細くする
        attackCylinder_->SetHeight(currentLength);
        attackCylinder_->Update();

        // 外側（電撃オーラ）的の更新：当たり判定(OBB = 0.5f)の太さにぴったり合わせる
        if (attackCylinderOuter_) {
            attackCylinderOuter_->SetCenter(attackTransform_.translate);
            attackCylinderOuter_->SetRotate(attackTransform_.rotate);
            attackCylinderOuter_->SetRadius(attackThickness_ * 0.5f); // ★当たり判定に合わせる
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
            // ★発生半径(radius)も、当たり判定と外側オーラに合わせた 0.5f に戻す
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

    if (isTelegraphActive_) {
        if (telegraphCylinder_) telegraphCylinder_->Draw(true); // UIキュー（最前面）で描画
        if (telegraphGroundAOE_ && telegraphGroundAOE_->GetTransform().transform.scale.x > 0.0f) {
            telegraphGroundAOE_->Draw(true);
        }
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