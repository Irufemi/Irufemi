#pragma once

/**
 * @class GameResultManager
 * @brief ゲームの結果を管理する静的クラス
 * @details GameSceneでの結果（勝利/敗北）をResultSceneに伝えるために使用されます。
 */
class GameResultManager {
public:
    /**
     * @enum Result
     * @brief ゲームの結果（勝利または敗北）
     */
    enum class Result {
        Win,
        Lose,
    };

    /**
     * @brief ゲームの結果
     * @details GameSceneで設定され、ResultSceneで読み込まれます。
     */
    static Result result;
};