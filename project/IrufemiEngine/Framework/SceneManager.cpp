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

    // シーン破棄前にGPU処理の完了を待つ (リソース解放中のアクセス違反を防ぐ)
    engine_->GetDirectXCommon()->WaitForGPU();

    current_.reset();
    current_ = it->second();
    currentName_ = next;
    
    isInitializing_ = true;
    current_->Initialize(engine_);
    isInitializing_ = false;

    wasLoading_ = false;

    isPaused_ = false; // シーン切り替え時にポーズを解除
    engine_->SetTimeScale(1.0f); // 時間の進みもリセット
    
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
    bool isLoading = (transitionPhase_ == TransitionPhase::Initializing) || !modelsLoaded || !texturesLoaded;

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
            // 時間の進み具合を制御
            engine_->SetTimeScale(isPaused_ ? 0.0f : 1.0f);
        }
    }

    // シーン切り替え要求（即時）
    if (!pending_.empty()) {
        StartAsyncInitialize(pending_); // 即時切替の場合も非同期初期化を開始する
        transitionPhase_ = TransitionPhase::Initializing;
        pending_.clear();
    }

    // --- 遷移プロセスの更新 ---
    if (transitionPhase_ == TransitionPhase::Closing) {
        // フェードアウト完了待ち
        if (engine_->GetSceneTransition()->IsOutFinished()) {
            StartAsyncInitialize(pendingTransition_); // 新しいシーンに非同期で切り替え、裏ロード開始
            pendingTransition_.clear();
            transitionPhase_ = TransitionPhase::Initializing; // 初期化フェーズへ
        }
    }
    else if (transitionPhase_ == TransitionPhase::Initializing) {
        // バックグラウンドでのシーン破棄・初期化完了待ち
        if (!isAsyncInitializing_.load()) {
            if (initFuture_.valid()) {
                initFuture_.get(); // 例外があればキャッチ
            }
            
            {
                std::lock_guard<std::mutex> lock(nextSceneMutex_);
                current_ = std::move(nextScene_);
            }
            
            isInitializing_ = false;
            
            // ポーズ可能なシーンかどうかに応じてマウスをロック
            engine_->SetCursorLocked(current_->IsPausable());
            
            transitionPhase_ = TransitionPhase::LoadingWait; // ロード待機フェーズへ
        }
    }
    else if (transitionPhase_ == TransitionPhase::LoadingWait) {
        // 全てのアセットのロード完了を待つ (画面は真っ暗なまま、上にLoadingScreenが描画される)
        if (modelsLoaded && texturesLoaded) {
            transitionPhase_ = TransitionPhase::Opening;
            
            // ---------------------------------------------------------
            // 【重要】フェードイン中 (Opening) は Update() がスキップされるため、
            // そのまま Draw() が呼ばれると、未初期化の定数バッファ（ゼロ行列）や
            // 前シーンのカメラデータが使われてしまい、深刻な点滅・描画崩れが発生する。
            // これを防ぐため、ロード完了直後に強制的に1回だけ Update() を回し、
            // 全オブジェクトの初回更新（UpdateAll等）とカメラ設定を済ませる。
            // ---------------------------------------------------------
            if (current_) {
                current_->Update();
            }

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

    // ロード中かどうかを最新の状態で再評価する（非同期初期化が開始された直後を考慮）
    isLoading = (transitionPhase_ == TransitionPhase::Initializing) || !modelsLoaded || !texturesLoaded;

    // ロード中であれば、ここで更新を止めてLoadingUIだけアニメーションさせる
    if (isLoading) {
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
    if (transitionPhase_ == TransitionPhase::Initializing || !modelsLoaded || !texturesLoaded) {
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
    if (transitionPhase_ == TransitionPhase::Initializing || !modelsLoaded || !texturesLoaded) {
        if (loadingScreen_) {
            loadingScreen_->Draw(engine_);
        }
    }
}

void SceneManager::StartAsyncInitialize(const Key& next) {
    auto it = factories_.find(next);
    if (it == factories_.end()) { return; }

    Factory factory = it->second;
    currentName_ = next;
    isAsyncInitializing_.store(true);
    isInitializing_ = true;
    wasLoading_ = false;
    isPaused_ = false;
    engine_->SetTimeScale(1.0f); // 時間の進みをリセット
    
    // --- 【重要】---
    // シーン破棄前に、現在溜まっている描画・コンピュートタスクを破棄する。
    // これをしないと、裏スレッドでのリソース破棄と並行して、
    // メインスレッドが古いComputeTaskを実行しようとしてクラッシュ（OBJECT_DELETED_WHILE_STILL_IN_USE）する。
    engine_->GetDrawManager()->ClearAllQueues();
    
    initFuture_ = std::async(std::launch::async, [this, factory]() {
        // GPU処理の完了を待つ (リソース解放中のアクセス違反を防ぐ)
        engine_->GetDirectXCommon()->WaitForGPU();
        
        // 現在のシーンを破棄
        current_.reset();
        
        // 新しいシーンを生成して初期化
        auto newScene = factory();
        newScene->Initialize(engine_);
        
        // メインスレッドへ渡すための準備
        {
            std::lock_guard<std::mutex> lock(nextSceneMutex_);
            nextScene_ = std::move(newScene);
        }
        
        isAsyncInitializing_.store(false);
    });
}