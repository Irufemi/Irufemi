#pragma once
#include "IScene.h"
#include "SceneName.h"
#include <functional>
#include <memory>
#include <unordered_map>

class IrufemiEngine;

class SceneManager {
public:
    using Factory = std::function<std::unique_ptr<IScene>()>;

    explicit SceneManager(IrufemiEngine* engine) : engine_(engine) {}

    void Register(SceneName name, Factory f) { factories_[name] = std::move(f); }

    // シーン切替要求（次の Update 冒頭で反映）
    void Request(SceneName next) { pending_ = next; }

    // 即時切替（初期化時など）
    bool ChangeTo(SceneName next) {
        auto it = factories_.find(next);
        if (it == factories_.end())
            return false;

        // 旧シーン破棄
        current_.reset();

        // 新シーン生成
        current_ = it->second();
        currentName_ = next;
        current_->Initialize(engine_);
        return true;
    }

    void Update();

    void Draw() {
        if (current_)
            current_->Draw();
    }

    SceneName GetCurrent() const { return currentName_; }

private:
    IrufemiEngine* engine_ = nullptr; // 非所有
    std::unique_ptr<IScene> current_{};
    SceneName currentName_ = SceneName::CountOfSceneName;
    SceneName pending_ = SceneName::CountOfSceneName;
    std::unordered_map<SceneName, Factory> factories_;
};

// （必要なら）どこからでも Request できるように
extern SceneManager* g_SceneManager;
