#include "GravityPlayerComponent.h"
#include "DebrisComponent.h"
#include "DebrisManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Engine/Platform/Input/Mouse.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/Core/Math/Random/Random.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Core/Math/MathFunction.h"
#include "RailShooterEnemyComponent.h"
#include "BossComponent.h"
#include "TargetableComponent.h"
#include "LockonMarkerUIComponent.h"
#include <algorithm>

void GravityPlayerComponent::OnRegisterProperties() {
    RegisterProperty("Max Orbit Count", &maxOrbitCount_);
    RegisterProperty("Pull Radius", &pullRadius_);
    RegisterProperty("Lockon Radius 2D", &lockonRadius2D_);
}

void GravityPlayerComponent::Initialize() {
    orbitingDebris_.clear();
    lockedTargets_.clear();
}

void GravityPlayerComponent::Start() {
    if (!gameObject_) return;
    auto scene = gameObject_->GetScene();
    if (scene) {
        auto debrisManagerObj = scene->FindGameObject("DebrisManager");
        if (debrisManagerObj) {
            debrisManager_ = debrisManagerObj->GetComponent<DebrisManagerComponent>();
        }
        
        // UIコンポーネントを検索
        auto uiObj = scene->FindGameObject("LockonMarkerUI");
        if (uiObj) {
            lockonMarkerUI_ = uiObj->GetComponent<LockonMarkerUIComponent>();
        }
    }
}

void GravityPlayerComponent::Update() {
    // 無効になった（すでに敵に当たって非アクティブになった等）ガレキをリストから除外
    orbitingDebris_.erase(
        std::remove_if(orbitingDebris_.begin(), orbitingDebris_.end(),
            [](const std::shared_ptr<GameObject>& obj) {
                if (!obj || !obj->GetIsActive()) return true;
                auto comp = obj->GetComponent<DebrisComponent>();
                // Thrown など、引き寄せ(Pulled)および周回(Orbiting)以外の状態になったらリストから除外
                return !comp || (comp->GetState() != DebrisState::Orbiting && comp->GetState() != DebrisState::Pulled);
            }),
        orbitingDebris_.end()
    );

    UpdateAim();
    HandlePullInput();
    HandleThrowInput();
}

void GravityPlayerComponent::HandlePullInput() {
    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    if (!input) return;

    // 右クリック または Eキー で引き寄せ
    if (input->IsMouseButtonPressed(Mouse::Button::Right) || input->IsKeyPressed('E')) {
        // 現在の保持数が上限に達しているなら引き寄せない
        if (static_cast<int>(orbitingDebris_.size()) >= maxOrbitCount_) return;

        // シーン内のすべての GameObject からガレキを検索
        auto scene = gameObject_->GetScene();
        if (!scene) return;

        auto transform = gameObject_->GetComponent<TransformComponent>();
        if (!transform) return;

        // 1. ロックオン対象がBossのシールド(DebrisState::BossOrbiting)なら、Bossからシールドを奪う
        for (auto& target : lockedTargets_) {
            if (!target) continue;
            if (auto debrisComp = target->GetComponent<DebrisComponent>()) {
                if (debrisComp->GetState() == DebrisState::BossOrbiting) {
                    // Bossのシールドリストから該当のガレキを削除
                    if (auto bossTarget = debrisComp->GetTarget().lock()) {
                        if (auto bossComp = bossTarget->GetComponent<BossComponent>()) {
                            bossComp->RemoveShield(target);
                        }
                    }

                    // プレイヤーの周回ガレキとして設定
                    debrisComp->SetState(DebrisState::Pulled);
                    debrisComp->SetTarget(gameObject_->shared_from_this());
                    debrisComp->SetOrbitParams(
                        Random::GeneratorFloat(0.0f, 6.28f),
                        Random::GeneratorFloat(2.0f, 4.0f)
                    );
                    orbitingDebris_.push_back(target);
                    // 対象をリストから外して探索終了
                    lockedTargets_.erase(std::remove(lockedTargets_.begin(), lockedTargets_.end(), target), lockedTargets_.end());
                    return; // Bossから奪った場合は野良ガレキ探索はスキップ
                }
            }
        }

        if (!debrisManager_) return;

        // 指定半径内で一番近いガレキを実体化して取得
        auto debrisObj = debrisManager_->ExtractNearestIdleDebris(transform->GetPosition(), pullRadius_);
        if (debrisObj) {
            if (auto debrisComp = debrisObj->GetComponent<DebrisComponent>()) {
                debrisComp->SetState(DebrisState::Pulled);
                debrisComp->SetTarget(gameObject_->shared_from_this());
                debrisComp->SetOrbitParams(
                    Random::GeneratorFloat(0.0f, 6.28f),
                    Random::GeneratorFloat(2.0f, 4.0f)
                );
                orbitingDebris_.push_back(debrisObj);
            }
        }
    }
}

void GravityPlayerComponent::HandleThrowInput() {
    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    if (!input) return;

    // 左クリック または Qキー で発射
    if (input->IsMouseButtonPressed(Mouse::Button::Left) || input->IsKeyPressed('Q')) {
        if (orbitingDebris_.empty()) return;

        // 保持しているガレキから1つ取り出す
        auto debris = orbitingDebris_.back();
        orbitingDebris_.pop_back();

        if (debris) {
            auto comp = debris->GetComponent<DebrisComponent>();
            if (comp) {
                std::shared_ptr<GameObject> throwTarget = nullptr;
                if (!lockedTargets_.empty()) {
                    throwTarget = lockedTargets_.front();
                    // 発射したターゲットはリストから外す（1つのターゲットに複数飛ばしたい場合は残すか仕様による）
                    lockedTargets_.erase(lockedTargets_.begin());
                }
                
                comp->SetState(DebrisState::Thrown);
                comp->SetTarget(throwTarget);
                
                // ターゲットがいない場合はカメラの前方へ飛ばす
                if (!throwTarget) {
                    auto cameraManager = BaseModel::GetIrufemiEngine()->GetCameraManager();
                    if (cameraManager && cameraManager->GetActiveCamera()) {
                        auto camera = cameraManager->GetActiveCamera();
                        // カメラのRotationから前方を計算
                        Matrix4x4 viewMat = camera->GetViewMatrix();
                        // View行列のZ軸成分の逆（またはカメラのForward）を使う
                        // Irufemiエンジンの Camera は Transform ではなく rotate_ などを保持しているかもしれないが
                        // 今回は単純に ViewMatrix からカメラの前方を抽出
                        // 3行目がForwardベクトル(D3DのView行列の場合、_31, _32, _33)
                        Vector3 forward = { viewMat.m[0][2], viewMat.m[1][2], viewMat.m[2][2] };
                        
                        // Z軸方向が画面奥だとすると、DirectX(LH)なら View行列の3行目は通常Zの正方向を向いている
                        comp->SetThrowDirection({ forward.x, forward.y, forward.z });
                    }
                }
                
                comp->SetState(DebrisState::Thrown);
            }
        }
    }
}

void GravityPlayerComponent::UpdateAim() {
    auto cameraManager = BaseModel::GetIrufemiEngine()->GetCameraManager();
    if (!cameraManager) {
        lockedTargets_.clear();
        if (lockonMarkerUI_) lockonMarkerUI_->SyncTargets(lockedTargets_);
        return;
    }
    auto camera = cameraManager->GetActiveCamera();
    if (!camera) {
        lockedTargets_.clear();
        if (lockonMarkerUI_) lockonMarkerUI_->SyncTargets(lockedTargets_);
        return;
    }

    Matrix4x4 viewProj = camera->GetViewProjectionMatrix3D();
    float viewWidth = camera->GetViewportWidth();
    float viewHeight = camera->GetViewportHeight();
    
    // マウス座標をエイムの基準とする
    auto inputManager = BaseModel::GetIrufemiEngine()->GetInputManager();
    Vector2 screenCenter = inputManager ? inputManager->GetMousePosition() : Vector2{ viewWidth * 0.5f, viewHeight * 0.5f };
    
    // ヒステリシス: 一度ロックした敵は1.2倍の半径まで許容する
    float lockonRadiusSq = lockonRadius2D_ * lockonRadius2D_;
    float keepRadiusSq = lockonRadiusSq * 1.44f; 

    struct Candidate {
        std::shared_ptr<GameObject> obj;
        float distSq;
        bool wasLocked;
    };
    std::vector<Candidate> candidates;

    auto wasLocked = [&](const std::shared_ptr<GameObject>& obj) {
        return std::find(lockedTargets_.begin(), lockedTargets_.end(), obj) != lockedTargets_.end();
    };

    auto scene = gameObject_->GetScene();
    if (!scene) return;

    for (auto targetComp : TargetableComponent::GetTargets()) {
        auto obj = targetComp->GetGameObject();
        if (!obj || !obj->GetIsActive()) continue;

        bool isTargetable = false;
        if (auto enemyComp = obj->GetComponent<RailShooterEnemyComponent>()) {
            if (enemyComp->IsAlive()) isTargetable = true;
        } else if (auto bossComp = obj->GetComponent<BossComponent>()) {
            if (bossComp->GetState() == BossState::CoreExposed) isTargetable = true;
        } else if (auto debrisComp = obj->GetComponent<DebrisComponent>()) {
            if (debrisComp->GetState() == DebrisState::BossOrbiting) isTargetable = true;
        }

        if (isTargetable) {
            auto transform = obj->GetComponent<TransformComponent>();
            if (transform) {
                // ワールド座標からNDC座標へ
                Vector3 clipPos = Math::Transform(transform->GetWorldPosition(), viewProj);
                
                // Zが0～1の範囲外（カメラ後方など）なら除外
                if (clipPos.z >= 0.0f && clipPos.z <= 1.0f) {
                    float screenX = (clipPos.x + 1.0f) * 0.5f * viewWidth;
                    float screenY = (1.0f - clipPos.y) * 0.5f * viewHeight;

                    float dx = screenX - screenCenter.x;
                    float dy = screenY - screenCenter.y;
                    float distSq = dx * dx + dy * dy;

                    auto sharedObj = obj->shared_from_this();
                    bool isLocked = wasLocked(sharedObj);
                    float allowedRadiusSq = isLocked ? keepRadiusSq : lockonRadiusSq;

                    // レティクル内に入っているか判定
                    if (distSq <= allowedRadiusSq) {
                        candidates.push_back({ sharedObj, distSq, isLocked });
                    }
                }
            }
        }
    }

    // 中心に近い順にソート（ロック継続中かどうかを優先するかは仕様次第だが、今回は距離優先）
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.distSq < b.distSq;
    });

    // 【重要】最大ロックオン数の制限
    // 所持しているガレキの数までロックオン可能。ただし所持0の場合はボスのシールドを奪うために最低1つ許可する。
    size_t maxLockOn = orbitingDebris_.size();
    if (maxLockOn == 0) maxLockOn = 1;

    lockedTargets_.clear();
    for (const auto& candidate : candidates) {
        if (lockedTargets_.size() >= maxLockOn) break;
        lockedTargets_.push_back(candidate.obj);
    }

    // UIへ同期
    if (lockonMarkerUI_) {
        lockonMarkerUI_->SyncTargets(lockedTargets_);
    }
}
