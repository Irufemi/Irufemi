#include "CG4Scene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

// ECSコンポーネントのインクルード
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Effect/ParticleEmitterComponent.h"
#include "Framework/Component/Effect/ParticleFieldComponent.h"

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
