#pragma once

// ゲームの結果を管理する静的クラス
class GameResultManager {
public:
    enum class Result {
        Win,
        Lose,
    };

    // ゲームの結果
    static Result result;
};