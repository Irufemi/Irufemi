#include "SceneManager.h"

SceneManager* g_SceneManager = nullptr;

void SceneManager::Update() {
    if (pending_ != SceneName::CountOfSceneName) {
        ChangeTo(pending_);
        pending_ = SceneName::CountOfSceneName;
    }
    if (current_) {
        IScene::SyncInput(engine_);
        current_->Update();
    }
}