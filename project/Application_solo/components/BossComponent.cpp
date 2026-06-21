#include "BossComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "DebrisManagerComponent.h"
#include "DebrisComponent.h"
#include "EnemyBeamComponent.h"
#include "GravityPlayerComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include <Windows.h>
#include <string>

BossComponent::BossComponent() {
}

void BossComponent::Initialize() {
    hp_ = maxHp_;
    state_ = BossState::Idle;
    isShieldsInitialized_ = false;

    if (gameObject_) {
        auto collider = gameObject_->GetComponent<SphereColliderComponent>();
        if (!collider) {
            collider = gameObject_->AddComponent<SphereColliderComponent>().get();
        }
        if (collider) {
            collider->isTrigger_ = true;
            collider->SetLocalRadius(10.0f); // モデルに隠れないよう少し大きめに設定
        }
    }

    // ビームコンポーネントの追加・初期化
    if (gameObject_) {
        beamComponent_ = gameObject_->GetComponent<EnemyBeamComponent>();
        if (!beamComponent_) {
            auto comp = gameObject_->AddComponent<EnemyBeamComponent>();
            beamComponent_ = comp.get();
            beamComponent_->Initialize();
        }
    }
    beamTimer_ = 0.0f;
}

void BossComponent::Update() {
    if (!gameObject_) return;
    
    // DebrisManagerから初回だけシールド用のガレキを取得する
    if (!isShieldsInitialized_) {
        auto scene = gameObject_->GetScene();
        if (scene) {
            auto managerObj = scene->FindGameObject("DebrisManager");
            if (managerObj) {
                debrisManager_ = managerObj->GetComponent<DebrisManagerComponent>();
                if (debrisManager_) {
                    auto setupDebris = [this](std::shared_ptr<GameObject> debrisObj) {
                        auto debrisComp = debrisObj->GetComponent<DebrisComponent>();
                        if (debrisComp) {
                            debrisComp->SetTarget(gameObject_->shared_from_this());
                            debrisComp->SetState(DebrisState::BossOrbiting); 
                        }
                        shields_.push_back(debrisObj);
                    };

                    // 残りを取得してセットアップ
                    for (int i = 0; i < maxShieldCount_; ++i) {
                        auto debrisObj = debrisManager_->AcquireDebris();
                        if (debrisObj) {
                            setupDebris(debrisObj);
                        }
                    }
                    isShieldsInitialized_ = true;
                }
            }
        }
    }
    
    // --- ビーム攻撃のタイマー処理 ---
    if (beamComponent_ && state_ != BossState::Destroyed) {
        float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
        if (deltaTime <= 0.0f) deltaTime = 1.0f / 60.0f;
        
        // すでに撃っている最中（CHARGING/FIRING）はタイマーを進めず、撃ち終わってから次に備える等の制御も可能だが
        // 今回はシンプルに10秒ごとにFireを呼ぶ。
        if (!beamComponent_->IsActive()) {
            beamTimer_ += deltaTime;
            if (beamTimer_ >= beamInterval_) {
                beamTimer_ = 0.0f;
                
                if (auto myTrans = gameObject_->GetComponent<TransformComponent>()) {
                    Vector3 startPos = myTrans->GetWorldPosition();
                    
                    // ボスの向いている方向を取得（Z軸が背中を向いているため反転する）
                    Matrix4x4 worldMat = myTrans->GetWorldMatrix();
                    // ボスの顔が手前（-Z方向）を向いていると仮定して、ローカルZ軸を反転させる
                    Vector3 forward = { -worldMat.m[2][0], -worldMat.m[2][1], -worldMat.m[2][2] };
                    forward = Math::Normalize(forward);
                    
                    // ボスのモデルの中にコアが埋まらないように、前方に大きくオフセットする（25.0f）
                    startPos = Math::Add(startPos, Math::Multiply(25.0f, forward)); 
                    startPos.y -= 2.0f; // 高さを-2.0fに調整
                    
                    // ターゲットは距離に関係なくはるか正面
                    Vector3 targetPos = Math::Add(startPos, Math::Multiply(1000.0f, forward));
                    
                    beamComponent_->Fire(startPos, targetPos);
                }
            }
        }
    }
    
    // CoreExposed への遷移チェック
    if (state_ == BossState::Idle && shields_.empty() && isShieldsInitialized_) {
        state_ = BossState::CoreExposed;
    }
}

void BossComponent::OnRegisterProperties() {
    RegisterProperty("Max HP", &maxHp_);
    RegisterProperty("Max Shield Count", &maxShieldCount_);
    RegisterProperty("Shield Radius", &shieldRadius_);
}

std::shared_ptr<GameObject> BossComponent::ExtractDebris() {
    if (shields_.empty()) return nullptr;
    
    // シールドリストから1つ取り出す
    auto debris = shields_.back();
    shields_.pop_back();
    
    // 取り出したガレキの状態は引っ張られる状態に変更する
    if (debris) {
        auto debrisComp = debris->GetComponent<DebrisComponent>();
        if (debrisComp) {
            debrisComp->SetState(DebrisState::Idle);
            debrisComp->SetTarget(std::weak_ptr<GameObject>());
        }
    }
    
    return debris;
}

void BossComponent::TakeDamage(float damage) {
    if (state_ == BossState::Destroyed) return;

    if (state_ == BossState::CoreExposed) {
        hp_ -= damage;
        
        std::string dmgLog = "Boss took damage! HP: " + std::to_string(hp_) + "\n";
        OutputDebugStringA(dmgLog.c_str());

        if (hp_ <= 0) {
            hp_ = 0;
            state_ = BossState::Destroyed;
            OutputDebugStringA("Boss Destroyed!\n");
        }
    } else {
        // シールドがある場合はダメージ無効、またはシールドが身代わりになる
        OutputDebugStringA("Boss blocked damage with shield!\n");
    }
}
