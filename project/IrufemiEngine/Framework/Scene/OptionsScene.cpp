#include "Framework/Scene/OptionsScene.h"
#include "Framework/Scene/SceneSerializer.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Component/UI/ButtonComponent.h"
#include "Framework/Component/UI/SliderComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/Utility/CVar.h"
#include "Audio/AudioManager.h"
#include "Audio/Sound.h"
#include "Framework/GameObject/GameObject.h"

void OptionsScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);
    
    // Prefab（実際はシーン形式）からUI要素をロード
    SceneSerializer::Load(this, "OptionsUI.prefab");
    
    uiBound_ = false;
}

void OptionsScene::OnEnter() {
    BaseScene::OnEnter();
}

void OptionsScene::Update() {
    BaseScene::Update();
    
    if (!uiBound_) {
        BindUIComponents();
        uiBound_ = true;
    }
    
    if (closeBtn_ && closeBtn_->IsClicked()) {
        if (auto engine = GetEngine()) {
            if (auto sm = engine->GetSceneManager()) {
                sm->PopScene();
            }
        }
    }
}

void OptionsScene::Finalize() {
    BaseScene::Finalize();
}

void OptionsScene::OnExit() {
    BaseScene::OnExit();
}

void OptionsScene::BindUIComponents() {
    auto engine = GetEngine();
    if (!engine) return;
    
    // UIを閉じるボタン用コールバック
    auto closeScene = [engine]() {
        if (auto sm = engine->GetSceneManager()) {
            sm->PopScene();
        }
    };

    for (auto& obj : gameObjects_) {
        auto name = obj->GetName();
        
        if (name == "Button_Apply") {
            if (auto btn = obj->GetComponent<ButtonComponent>()) {
                // ToDo: Apply logic
            }
        }
        else if (name == "Button_Cancel" || name == "Button_Close") {
            if (auto btn = obj->GetComponent<ButtonComponent>()) {
                closeBtn_ = btn;
            }
        }
        else if (name == "Slider_BGM") {
            if (auto slider = obj->GetComponent<SliderComponent>()) {
                slider->SetValue(Irufemi::CVarSystem::GetFloat("a.MasterVolume")); // 代替としてMasterVolumeを使用
                slider->SetOnValueChangedCallback([](float val) {
                    Irufemi::CVarSystem::SetFloat("a.MasterVolume", val);
                });
            }
        }
        else if (name == "Slider_SE") {
            if (auto slider = obj->GetComponent<SliderComponent>()) {
                slider->SetValue(1.0f); // ダミー
                slider->SetOnValueChangedCallback([](float val) {
                    // SE用音量の保存先があれば設定
                });
            }
        }
    }
}

void OptionsScene::ApplyPendingSettings() {
    // 保留設定を適用
}

void OptionsScene::RevertSettings() {
    // キャンセル時
}
