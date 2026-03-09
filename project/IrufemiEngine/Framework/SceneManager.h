#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class IrufemiEngine;
class IScene;

class SceneManager {
public:
    using Key = std::string;
    using Factory = std::function<std::unique_ptr<IScene>()>;

    explicit SceneManager(IrufemiEngine* engine);

    // 登録順を保持しつつ登録
    void Register(const Key& name, Factory f);

    // シーン切替要求(次の Update 冒頭で反映)
    void Request(const Key& next);

    // 即時切替(初期化時など)
    bool ChangeTo(const Key& next);

    void Update();

    void Draw();

    const Key& GetCurrent() const;

    // 並び順は登録順
    std::vector<Key> GetRegisteredKeys() const;

    // ポーズ状態の切り替え
    void TogglePause() { isPaused_ = !isPaused_; }
    bool IsPaused() const { return isPaused_; }

private:
    IrufemiEngine* engine_ = nullptr; // 非所有
    std::unique_ptr<IScene> current_{};
    Key currentName_{};
    Key pending_{};

    std::unordered_map<Key, Factory> factories_;
    std::vector<Key> order_; // ← 登録順を保持

    bool isPaused_ = false; // ポーズ状態フラグ
};