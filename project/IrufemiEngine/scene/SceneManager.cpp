#include "SceneManager.h"
#include "IScene.h"
#include "engine/IrufemiEngine.h"

SceneManager::SceneManager(IrufemiEngine* engine) : engine_(engine) {}

// 登録順を保持しつつ登録
void SceneManager::Register(const Key& name, Factory f) {
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        order_.push_back(name);            // 新規登録時のみ順序リストに追加
        factories_.emplace(name, std::move(f));
    } else {
        it->second = std::move(f);         // 既存は上書きのみ（順序は変えない）
    }
}

// シーン切替要求（次の Update 冒頭で反映）
void SceneManager::Request(const Key& next) { pending_ = next; }

// 即時切替（初期化時など）
bool SceneManager::ChangeTo(const Key& next) {
    auto it = factories_.find(next);
    if (it == factories_.end()) { return false; }

    current_.reset();
    current_ = it->second();
    currentName_ = next;
    current_->Initialize(engine_);
    return true;
}

void SceneManager::Update() {
    // 入力同期（必要ならここで）
    IScene::SyncInput(engine_);

    if (!pending_.empty()) {
        ChangeTo(pending_);
        pending_.clear();
    }

    if (current_) {
        current_->Update();
    }
}

void SceneManager::Draw() {
    if (current_) { current_->Draw(); }
}

const  SceneManager::Key& SceneManager::GetCurrent() const { return currentName_; }

// 並び順は登録順
std::vector<SceneManager::Key> SceneManager::GetRegisteredKeys() const { return order_; }