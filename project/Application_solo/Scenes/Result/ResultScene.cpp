#include "Scenes/Result/ResultScene.h"
#include "Core/ResultManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/Irufemi.h"

// 静的メンバ変数の実体定義
bool ResultScene::s_isClear = false;

void ResultScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // 本来は "Result.json" などから DataDriven にロードすべきですが、
    // 今回は空のシーンに対して、UI・進行を管理する ResultManagerComponent を持った
    // GameObject を一つだけ生成して配置します。
    auto managerObj = std::make_shared<GameObject>("ResultManager");
    AddGameObject(managerObj);

    auto t = managerObj->GetTransform();
    if (t) {
        t->SetPosition({640.0f, 340.0f, 0.0f});
    }

    // ResultManagerComponent にすべてのロジックを委譲する
    managerObj->AddComponent<ResultManagerComponent>();

    managerObj->Initialize();
}
