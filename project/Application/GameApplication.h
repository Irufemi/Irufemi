#pragma once

/**
 * @class GameApplication
 * @brief ゲームアプリケーション全体のエントリーポイントと実行を管理するクラス
 *
 * エンジンの初期化、シーンの登録、ゲームループの開始を担当します。
 */
class GameApplication {
public:
    /**
     * @brief コンストラクタ
     */
    GameApplication();
    /**
     * @brief デストラクタ
     */
    ~GameApplication();

    /**
     * @brief ゲームアプリケーションを実行します
     */
    void Run();

private:
    // コピー禁止
    GameApplication(const GameApplication&) = delete;
    GameApplication& operator=(const GameApplication&) = delete;
};