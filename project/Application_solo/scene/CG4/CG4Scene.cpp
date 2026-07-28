#include "CG4Scene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

// ECSコンポーネントのインクルード
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Effect/ParticleEmitterComponent.h"
#include "Framework/Component/Effect/ParticleFieldComponent.h"
#include "Framework/Component/Logic/BoneAttachmentComponent.h"
// デストラクタ
CG4Scene::~CG4Scene() = default;

// 初期化
void CG4Scene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // シーン初期化時に必要なオブジェクト等があればここに追加する
    
    // Mesh Particle Emitter の作成
    auto particleObj = std::make_shared<GameObject>("MeshParticleEmitter");
    AddGameObject(particleObj);
    auto emitter = particleObj->AddComponent<ParticleEmitterComponent>();
    if (auto po = emitter->GetParticleObject()) {
        po->SetEmitType(6); // 6: Mesh
        po->SetEmitterModelPath("CG4/BrainStem.glb");
        po->SetEmissionRate(1000.0f); // 10000だとMaxParticles(32768)を即座に使い切るため減らします
        po->SetLifeTimeMin(1.0f);
        po->SetLifeTimeMax(2.0f);
        po->SetEnableTrail(false); // トレイル(軌跡)は大量のパーティクルを消費するためOFFにします
        po->SetEnableDeathEmit(false);
        po->SetColor({0.2f, 1.0f, 0.5f, 1.0f}); // 緑系
        po->SetStartScale({0.2f, 0.2f, 0.2f});
        po->SetEndScale({0.0f, 0.0f, 0.0f});
    }
    if (auto t = particleObj->GetComponent<TransformComponent>()) {
        t->SetPosition({0.0f, 0.0f, 0.0f});
        t->SetScale({3.0f, 3.0f, 3.0f}); // 少し大きめに
    }
    emitter->Play();

    // パーティクルに影響を与える Field の作成
    auto fieldObj = std::make_shared<GameObject>("ParticleVortexField");
    AddGameObject(fieldObj);
    auto fieldComp = fieldObj->AddComponent<ParticleFieldComponent>();
    fieldComp->GetFieldData().type = 2; // 2: Vortex (渦)
    fieldComp->GetFieldData().strength = 100.0f;
    fieldComp->GetFieldData().range = 20.0f;
    fieldComp->GetFieldData().axis = {0.0f, 1.0f, 0.0f};
    if (auto t = fieldObj->GetComponent<TransformComponent>()) {
        t->SetPosition({0.0f, 5.0f, 0.0f});
    }

    // プレイヤーの右手から発生するパーティクル
    auto handParticleObj = std::make_shared<GameObject>("HandParticleEmitter");
    AddGameObject(handParticleObj);
    handParticleObj->AddComponent<TransformComponent>();
    auto handEmitter = handParticleObj->AddComponent<ParticleEmitterComponent>();
    if (auto po = handEmitter->GetParticleObject()) {
        po->SetEmitType(1); // 1: Beam (円錐状の拡散)
        po->SetDirection({0.0f, -1.0f, 0.0f}); // 下方向(または手首の方向)
        po->SetVelocity(3.0f); // 飛び出す勢い
        po->SetSpread(0.3f); // 狭めの拡散(Spread)
        po->SetRadius(0.0f); // 発生源の大きさ
        po->SetEmissionRate(200.0f); // 少し発生量を増やす
        po->SetLifeTimeMin(0.5f);
        po->SetLifeTimeMax(1.0f);
        po->SetEnableTrail(false);
        po->SetColor({1.0f, 0.5f, 0.2f, 1.0f}); // オレンジ系の火花風
        po->SetStartScale({0.1f, 0.1f, 0.1f});
        po->SetEndScale({0.0f, 0.0f, 0.0f});
    }
    auto boneAttachment = handParticleObj->AddComponent<BoneAttachmentComponent>();
    boneAttachment->SetTargetName("Player");
    boneAttachment->SetTargetBoneName("mixamorig:RightHand");
    handEmitter->Play();

    // 左手の武器（適当な棒）
    auto weaponObj = std::make_shared<GameObject>("Weapon_Stick");
    AddGameObject(weaponObj);
    auto weaponTransform = weaponObj->AddComponent<TransformComponent>();
    // スケールを棒状に(細長く)
    weaponTransform->SetScale({0.05f, 0.4f, 0.05f});
    auto weaponRenderer = weaponObj->AddComponent<PrimitiveRendererComponent>();
    weaponRenderer->SetShape(PrimitiveType::Cylinder);
    weaponRenderer->SetColor({0.6f, 0.3f, 0.1f, 1.0f}); // 木の棒っぽい色

    auto weaponAttachment = weaponObj->AddComponent<BoneAttachmentComponent>();
    weaponAttachment->SetTargetName("Player");
    weaponAttachment->SetTargetBoneName("mixamorig:LeftHand");
}

// 更新
void CG4Scene::Update() {
    BaseScene::Update();

    // BackSpaceキーで遷移テスト (例)
    if (IsKeyPressed(VK_BACK)) {
        engine_->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Slide, 1.0f);
    }
}

void CG4Scene::Draw() {
    BaseScene::Draw();
}

void CG4Scene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}
