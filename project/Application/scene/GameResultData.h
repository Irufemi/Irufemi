#pragma once

// ゲームの結果をシーン間で共有するためのデータ構造
struct GameResultData {
    // ゲームをクリアしたか (true: クリア, false: ゲームオーバー)
    bool isGameClear = false;
    // 倒した敵の数
    int killScore = 0;

    // シングルトンインスタンスを取得
    static GameResultData& GetInstance() {
        static GameResultData instance;
        return instance;
    }

private:
    // プライベートコンストラクタでシングルトンを保証
    GameResultData() = default;
    ~GameResultData() = default;
    GameResultData(const GameResultData&) = delete;
    GameResultData& operator=(const GameResultData&) = delete;
};