#pragma once

class GameApplication {
public:
    // コンストラクタ・デストラクタ
    GameApplication();
    ~GameApplication();

    // 実行
    void Run();

private:
    // コピー禁止
    GameApplication(const GameApplication&) = delete;
    GameApplication& operator=(const GameApplication&) = delete;
};