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

    void Register(const Key& name, Factory f);
    void Request(const Key& next);

    bool ChangeTo(const Key& next);

    void Update();

    void Draw();

    const Key& GetCurrent() const;

    std::vector<Key> GetRegisteredKeys() const;

private:
    IrufemiEngine* engine_ = nullptr;
    std::unique_ptr<IScene> current_{};
    Key currentName_{};
    Key pending_{};
    std::unordered_map<Key, Factory> factories_;
};