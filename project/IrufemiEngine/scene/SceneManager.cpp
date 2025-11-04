#include "SceneManager.h"
#include "IScene.h"
#include "engine/IrufemiEngine.h"

SceneManager::SceneManager(IrufemiEngine* engine) : engine_(engine) {}

void SceneManager::Register(const Key& name, Factory f) { factories_[name] = std::move(f); }

void SceneManager::Request(const Key& next) { pending_ = next; }

bool SceneManager::ChangeTo(const Key& next) {
    auto it = factories_.find(next);
    if (it == factories_.end()) return false;
    current_.reset();
    current_ = it->second();
    currentName_ = next;
    current_->Initialize(engine_);
    return true;
}

void SceneManager::Update() {
    IScene::SyncInput(engine_);
    if (!pending_.empty()) {
        ChangeTo(pending_);
        pending_.clear();
    }
    if (current_) { current_->Update(); }
}

void SceneManager::Draw() { if (current_) current_->Draw(); }

const SceneManager::Key& SceneManager::GetCurrent() const { return currentName_; }

std::vector<SceneManager::Key> SceneManager::GetRegisteredKeys() const {
    std::vector<Key> keys; keys.reserve(factories_.size());
    for (auto& kv : factories_) keys.push_back(kv.first);
    return keys;
}