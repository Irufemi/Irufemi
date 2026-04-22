#include "SceneManager.h"
#include "IScene.h"
#include "LoadingScreen.h"
#include "../Engine/IrufemiEngine.h"
#include <Windows.h>
#include "../Engine/Platform/Input/InputManager.h"
#include "../Engine/Platform/Input/Mouse.h"

SceneManager::SceneManager(IrufemiEngine* engine) : engine_(engine) {
    loadingScreen_ = std::make_unique<LoadingScreen>();
    loadingScreen_->Initialize(engine_);
}

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

    wasLoading_ = false;

    isPaused_ = false; // シーン切り替え時にポーズを解除
    
    /**
     * @brief ポーズ可能なシーン(ゲーム等)はマウスをロック(非表示)し、
     *        そうでないシーン(UI操作メイン等)はマウスを表示させる
     */
    engine_->SetCursorLocked(current_->IsPausable()); 
    return true;
}

void SceneManager::TransitionTo(const Key& next, SceneTransition::Type type, float duration) {
    if (transitionPhase_ != TransitionPhase::None) return; // 二重遷移防止

    pendingTransition_ = next;
    pendingType_ = type;
    pendingDuration_ = duration;
    transitionPhase_ = TransitionPhase::Closing;

    engine_->GetSceneTransition()->Start(type, duration, true);
}

void SceneManager::Update() {
    // モデル・テクスチャのロード状況を確認
    bool modelsLoaded = !engine_->GetObjModelManager() || engine_->GetObjModelManager()->IsAllLoaded();
    bool texturesLoaded = !engine_->GetTextureManager() || engine_->GetTextureManager()->IsAllLoaded();
    bool isLoading = !modelsLoaded || !texturesLoaded;

    if (isLoading != wasLoading_) {
        if (isLoading) {
            // ロード中はマウスカーソルを表示させる
            engine_->SetCursorLocked(false);
        } else {
            // ロード完了時に、現在のシーン本来の設定に戻す
            if (current_) {
                engine_->SetCursorLocked(current_->IsPausable());
            }
        }
        wasLoading_ = isLoading;
    }

    // 入力同期
    IScene::SyncInput(engine_);

    // 現在のシーンがポーズ可能な場合のみ、ESCキーまたはゲームパッドのスタートボタンでポーズ切り替え
    if (current_ && current_->IsPausable()) {
        InputManager* input = engine_->GetInputManager();
        if (input && (IScene::PressedVK(VK_ESCAPE) || input->StartPressed())) {
            TogglePause();
            // ポーズ状態に合わせてマウスのロックを切り替え
            engine_->SetCursorLocked(!isPaused_);
        }
    }

    // シーン切り替え要求（即時）
    if (!pending_.empty()) {
        ChangeTo(pending_);
        pending_.clear();
    }

    // --- 遷移プロセスの更新 ---
    if (transitionPhase_ == TransitionPhase::Closing) {
        // フェードアウト完了待ち
        if (engine_->GetSceneTransition()->IsOutFinished()) {
            ChangeTo(pendingTransition_); // 新しいシーンに切り替え、裏ロード開始
            pendingTransition_.clear();
            transitionPhase_ = TransitionPhase::LoadingWait; // ロード待機フェーズへ
        }
    }
    else if (transitionPhase_ == TransitionPhase::LoadingWait) {
        // 全てのアセットのロード完了を待つ (画面は真っ暗なまま、上にLoadingScreenが描画される)
        if (modelsLoaded && texturesLoaded) {
            transitionPhase_ = TransitionPhase::Opening;
            // ロードが完了した瞬間に、フェードインを開始する
            engine_->GetSceneTransition()->Start(pendingType_, pendingDuration_, false);
        }
    }
    else if (transitionPhase_ == TransitionPhase::Opening) {
        // フェードイン完了待ち
        if (engine_->GetSceneTransition()->IsFinished()) {
            transitionPhase_ = TransitionPhase::None;
        }
    }

    // ロード中であれば、ここで更新を止めてLoadingUIだけアニメーションさせる
    if (!modelsLoaded || !texturesLoaded) {
        if (loadingScreen_) {
            loadingScreen_->Update(engine_->GetDeltaTime());
        }
        return;
    }

    // ロードが完全に終わっている場合のみ、シーン自体のUpdateを回す
    if (current_) {
        if (isPaused_) {
            // ポーズ中
            current_->PauseUpdate();
        }
        else {
            // 通常更新
            // ※フェードイン中 (Opening) は Update を呼ばないよう制限
            if (transitionPhase_ != TransitionPhase::Opening) {
                current_->Update();
            }
        }
    }
}

void SceneManager::Draw() {
    // ロード待ちの場合は背景のみ(UIはDrawLoadingUIでバックバッファに直接描画される)
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

void SceneManager::DrawLoadingUI() {
    bool modelsLoaded = !engine_->GetObjModelManager() || engine_->GetObjModelManager()->IsAllLoaded();
    bool texturesLoaded = !engine_->GetTextureManager() || engine_->GetTextureManager()->IsAllLoaded();
    if (!modelsLoaded || !texturesLoaded) {
        if (loadingScreen_) {
            loadingScreen_->Draw(engine_);
        }
    }
}