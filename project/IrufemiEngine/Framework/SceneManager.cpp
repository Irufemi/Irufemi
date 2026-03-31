#include "SceneManager.h"
#include "IScene.h"
#include "../Engine/IrufemiEngine.h"
#include <Windows.h>
#include "../Engine/Platform/Input/InputManager.h"
#include "../Engine/Platform/Input/Mouse.h"

SceneManager::SceneManager(IrufemiEngine* engine) : engine_(engine) {}

// 登録順を保持しつつ登録
void SceneManager::Register(const Key& name, Factory f) {
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        order_.push_back(name);            // 新規登録時のみ順序リストに追加
        factories_.emplace(name, std::move(f));
    } else {
        it->second = std::move(f);         // 既存は上書きのみ(順序は変えない)
    }
}

// シーン切替要求(次の Update 冒頭で反映)
void SceneManager::Request(const Key& next) { pending_ = next; }

// 即時切替(初期化時など)
bool SceneManager::ChangeTo(const Key& next) {
    auto it = factories_.find(next);
    if (it == factories_.end()) { return false; }

    current_.reset();
    current_ = it->second();
    currentName_ = next;
    
    isInitializing_ = true;
    current_->Initialize(engine_);
    isInitializing_ = false;

    isPaused_ = false; // シーン切り替え時にポーズを解除
    engine_->SetCursorLocked(true); // シーン開始時はマウスをロック
    return true;
}

void SceneManager::Update() {
    // モデル・テクスチャの読み込み待ちがある場合は、シーンの更新を止める
    bool modelsLoaded = !engine_->GetObjModelManager() || engine_->GetObjModelManager()->IsAllLoaded();
    bool texturesLoaded = !engine_->GetTextureManager() || engine_->GetTextureManager()->IsAllLoaded();
    if (!modelsLoaded || !texturesLoaded) {
        return;
    }

    // 入力同期
    IScene::SyncInput(engine_);

    // 現在のシーンがポーズ可能な場合のみ、ESCキーまたはゲームパッドのスタートボタンでポーズ切り替え
    if (current_ && current_->IsPausable()) {
        InputManager* input = engine_->GetInputManager();
        if (input && (IScene::PressedVK(VK_ESCAPE) || input->StartPressed())) {
            TogglePause();
            // ポーズ状態に合わせてマウスのロックを切り替え
            // ポーズ中ならロック解除（マウス表示）、通常ならロック（非表示）
            engine_->SetCursorLocked(!isPaused_);
        }
    }

    // シーン切り替え処理
    if (!pending_.empty()) {
        ChangeTo(pending_);
        pending_.clear();
    }

    if (current_) {
        // ロード中（Initialize等で始まった分を含む）なら更新を止める
        if ((engine_->GetObjModelManager() && !engine_->GetObjModelManager()->IsAllLoaded()) ||
            (engine_->GetTextureManager() && !engine_->GetTextureManager()->IsAllLoaded())) {
            return;
        }

        if (isPaused_) {
            // ポーズ中
            current_->PauseUpdate();
        }
        else {
            // 通常更新
            current_->Update();
        }
    }
}

void SceneManager::Draw() {
    // モデル・テクスチャの読み込み待ちがある場合は、シーンの描画を止める（背景のみの状態にする）
    bool modelsLoaded = !engine_->GetObjModelManager() || engine_->GetObjModelManager()->IsAllLoaded();
    bool texturesLoaded = !engine_->GetTextureManager() || engine_->GetTextureManager()->IsAllLoaded();
    if (!modelsLoaded || !texturesLoaded) {
        return;
    }

    if (current_) {
        // 通常の描画
        current_->Draw();
        // ポーズ中なら、その上にポーズ画面を描画
        if (isPaused_) {
            current_->PauseDraw();
        }
    }
}

const  SceneManager::Key& SceneManager::GetCurrent() const { return currentName_; }

// 並び順は登録順
std::vector<SceneManager::Key> SceneManager::GetRegisteredKeys() const { return order_; }