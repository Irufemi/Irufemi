#include "EnemyBeamComponent.h"
#include "Framework/GameObject.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include <cmath>
#include <algorithm>
#include <Windows.h>
#include <string>

#undef min
#undef max

EnemyBeamComponent::~EnemyBeamComponent() {
    auto dxCommon = BaseModel::GetIrufemiEngine()->GetDirectXCommon();
    if (dxCommon) {
        if (beamParamsResource_) {
            beamParamsResource_->Unmap(0, nullptr);
            dxCommon->ReleaseAfterFence(beamParamsResource_);
        }
        if (auraParamsResource_) {
            auraParamsResource_->Unmap(0, nullptr);
            dxCommon->ReleaseAfterFence(auraParamsResource_);
        }
    }
}

void EnemyBeamComponent::OnRegisterProperties() {
    RegisterProperty("Beam Length", &beamLength_);
    RegisterProperty("Beam Max Radius", &beamMaxRadius_);
    RegisterProperty("Charge Duration", &chargeDuration_);
    RegisterProperty("Fire Duration", &fireDuration_);
}

void EnemyBeamComponent::Initialize() {
    auto engine = BaseModel::GetIrufemiEngine();
    if (!engine) return;

    // --- チャージ球の初期化 ---
    chargeSphere_ = std::make_unique<Primitive3DObject>();
    chargeSphere_->Initialize(PrimitiveType::Sphere);
    // 警戒色と赤黒さを混ぜたベース色（アルファ値は中心コアの透明度として利用）
    chargeSphere_->SetColor({ 0.7f, 0.0f, 0.9f, 1.0f }); // ダークなマゼンタパープル
    chargeSphere_->SetCullingEnabled(false);
    chargeSphere_->SetCustomPSO(
        engine->GetPSOManager()->GetPSO("EnergyCore", BlendMode::kBlendModePremultiplied, PSOManager::DepthWrite::Disable, PSOManager::CullMode::Back)
    );
    chargeSphere_->SetIsTransparent(true); // ★半透明パスでZソートして描画させる

    // --- ビーム本体の初期化 ---
    attackCylinder_ = std::make_shared<Primitive3DObject>();
    attackCylinder_->Initialize(PrimitiveType::Cylinder);
    attackCylinder_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    attackCylinder_->SetCastShadows(false);
    attackCylinder_->SetCullingEnabled(false);
    attackCylinder_->SetIsTransparent(true); // ★半透明パスでZソートして描画させる

    attackCylinderOuter_ = std::make_shared<Primitive3DObject>();
    attackCylinderOuter_->Initialize(PrimitiveType::Cylinder);
    attackCylinderOuter_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    attackCylinderOuter_->SetCastShadows(false);
    attackCylinderOuter_->SetCullingEnabled(false);
    attackCylinderOuter_->SetIsTransparent(true); // ★半透明パスでZソートして描画させる

    // --- シェーダーパラメータの初期化 ---
    auto dxCommon = engine->GetDirectXCommon();

    // 内側（極太レーザーコア）用パラメータ
    beamParamsResource_ = dxCommon->CreateBufferResource(sizeof(LightningParams));
    beamParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&beamParamsData_));
    if (beamParamsData_) {
        *beamParamsData_ = LightningParams();
        beamParamsData_->color = { 0.8f, 0.0f, 1.0f, 1.0f }; // ネオンパープルオーラ
        beamParamsData_->coreColor = { 0.0f, 1.0f, 1.0f, 1.0f }; // 高エネルギーのシアンコア
        beamParamsData_->intensity = 6.0f;
        beamParamsData_->noiseThreshold = 0.35f;
        beamParamsData_->coreIntensity = 40.0f;
        beamParamsData_->coreThreshold = 0.85f;
        beamParamsData_->coreScale = 2.5f;
        beamParamsData_->speed = 3.0f;
        beamParamsData_->noiseScale = 1.2f;
        beamParamsData_->spinSpeed = 4.0f;
        beamParamsData_->twistScale = 4.0f;
    }

    // 外側（バチバチ電撃・オーラ）用パラメータ
    auraParamsResource_ = dxCommon->CreateBufferResource(sizeof(LightningParams));
    auraParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&auraParamsData_));
    if (auraParamsData_) {
        *auraParamsData_ = LightningParams();
        auraParamsData_->color = { 0.1f, 0.0f, 0.2f, 1.0f }; // ダークパープル/黒っぽいオーラ
        auraParamsData_->coreColor = { 0.8f, 0.0f, 1.0f, 1.0f }; // コアはネオンパープル
        auraParamsData_->intensity = 12.0f;
        auraParamsData_->noiseThreshold = 0.4f;
        auraParamsData_->coreIntensity = 20.0f;
        auraParamsData_->coreThreshold = 0.55f;
        auraParamsData_->coreScale = 2.5f;
        auraParamsData_->speed = 0.8f;
        auraParamsData_->noiseScale = 1.0f;
        auraParamsData_->spinSpeed = 8.0f;
        auraParamsData_->twistScale = 6.0f;
    }

    state_ = State::IDLE;
}

void EnemyBeamComponent::Fire(const Vector3& startPos, const Vector3& targetPos) {
    state_ = State::CHARGING;
    stateTimer_ = 0.0f;
    startPos_ = startPos;
    
    // 発射方向の計算
    Vector3 diff = Math::Subtract(targetPos, startPos_);
    direction_ = Math::Normalize(diff);
}

void EnemyBeamComponent::Update() {
    if (state_ == State::IDLE) return;

    auto engine = BaseModel::GetIrufemiEngine();
    float deltaTime = engine->GetGameDeltaTime();
    if (deltaTime <= 0.0f) deltaTime = 1.0f / 60.0f;

    stateTimer_ += deltaTime;

    if (state_ == State::CHARGING) {
        // --- 溜め動作のアニメーション ---
        float t = std::min(stateTimer_ / chargeDuration_, 1.0f);
        
        // イーズイン (急激に収縮してエネルギーが凝縮される表現) + 明滅
        float easeT = t * t * t; 
        // 最終的には少し大きめに膨張する
        float baseScale = std::lerp(0.1f, 4.0f, easeT);
        float pulse = 1.0f + 0.3f * std::sin(t * 50.0f); // より激しく不安定な明滅
        float currentScale = baseScale * pulse;

        if (chargeSphere_) {
            Transform tForm;
            tForm.scale = { currentScale, currentScale, currentScale };
            
            // ビルボード処理（カメラに向ける）
            Vector3 cameraPos = startPos_;
            if (engine->GetCameraManager() && engine->GetCameraManager()->GetActiveCamera()) {
                cameraPos = engine->GetCameraManager()->GetActiveCamera()->GetTranslate();
            }
            Vector3 toCamera = Math::Subtract(cameraPos, startPos_);
            Vector3 toCameraDir = Math::Normalize(toCamera);
            
            // 少しカメラ側に引き寄せてモデルに埋まらないようにする
            tForm.translate = Math::Add(startPos_, Math::Multiply(currentScale * 0.5f, toCameraDir));

            toCamera = Math::Subtract(cameraPos, tForm.translate);
            tForm.rotate.y = std::atan2(-toCamera.x, -toCamera.z);
            float distXZ = std::sqrt(toCamera.x * toCamera.x + toCamera.z * toCamera.z);
            tForm.rotate.x = std::atan2(toCamera.y, distXZ);
            tForm.rotate.z = 0.0f;

            chargeSphere_->GetTransform().transform = tForm;
            chargeSphere_->GetTransform().isDirty = true;
            chargeSphere_->Update();
        }

        // 溜め完了で発射ステートへ
        if (stateTimer_ >= chargeDuration_) {
            state_ = State::FIRING;
            stateTimer_ = 0.0f;
            
            if (chargeSphere_) {
                chargeSphere_->GetTransform().transform.scale = { 0, 0, 0 };
                chargeSphere_->GetTransform().isDirty = true;
                chargeSphere_->Update();
            }
        }
    } 
    else if (state_ == State::FIRING) {
        // --- ビーム発射動作のアニメーション ---
        float t = std::min(stateTimer_ / fireDuration_, 1.0f);
        
        // 少し太さを維持し、最後にフェードアウトするように変更
        float easeThickness = 1.0f - (t * t * t);
        float currentThickness = beamMaxRadius_ * easeThickness;
        
        // ビームの長さは一瞬で最大まで到達する想定
        float currentLength = beamLength_;

        // 円柱の回転計算（Y軸方向のCylinderを direction_ に向ける）
        Matrix4x4 rotMat = Math::DirectionToDirection({ 0.0f, 1.0f, 0.0f }, direction_);
        Vector3 rotate = Math::ExtractEulerFromMatrix(rotMat);

        Vector3 center = Math::Add(startPos_, Math::Multiply(currentLength * 0.5f, direction_));

        // 内側コアの更新
        if (attackCylinder_) {
            attackCylinder_->SetPosition(center);
            attackCylinder_->SetRotate(rotate);
            attackCylinder_->SetScale({ currentThickness * 0.5f, currentLength, currentThickness * 0.5f });
            attackCylinder_->Update();
        }

        // 外側オーラの更新
        if (attackCylinderOuter_) {
            attackCylinderOuter_->SetPosition(center);
            attackCylinderOuter_->SetRotate(rotate);
            attackCylinderOuter_->SetScale({ currentThickness, currentLength, currentThickness });
            attackCylinderOuter_->Update();
        }

        // 終了判定
        if (stateTimer_ >= fireDuration_) {
            state_ = State::IDLE;
        }
    }
}

void EnemyBeamComponent::Draw() {
    auto engine = BaseModel::GetIrufemiEngine();
    if (!engine) return;

    if (state_ == State::CHARGING) {
        if (chargeSphere_ && chargeSphere_->GetTransform().transform.scale.x > 0.0f) {
            chargeSphere_->Draw(); // 3D空間描画
        }
    } 
    else if (state_ == State::FIRING) {
        // 外側オーラ (LightningCrawl)
        if (attackCylinderOuter_ && auraParamsResource_) {
            attackCylinderOuter_->SetCustomPSO(engine->GetPSOManager()->GetPSO("LightningCrawl", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
            attackCylinderOuter_->SetCustomCBVAddress(auraParamsResource_->GetGPUVirtualAddress());
            
            attackCylinderOuter_->Draw();
        }

        // 内側コア (EnergyBeam)
        if (attackCylinder_ && beamParamsResource_) {
            attackCylinder_->SetCustomPSO(engine->GetPSOManager()->GetPSO("EnergyBeam", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
            attackCylinder_->SetCustomCBVAddress(beamParamsResource_->GetGPUVirtualAddress());
            
            attackCylinder_->Draw();
        }
    }
}
