#pragma once
#include <memory>

class IrufemiEngine;

/**
 * @class ILoadingScreen
 * @brief エンジンからローディング画面の描画を要求するためのインターフェース
 * @details アプリケーション側でこのインターフェースを実装し、IrufemiEngineに登録することで任意のローディングUIを描画できます。
 */
class ILoadingScreen {
public:
    virtual ~ILoadingScreen() = default;

    /**
     * @brief ローディング画面の描画処理
     * @param engine エンジンインスタンス
     * @param deltaTime フレーム経過時間
     */
    virtual void OnDrawLoadingScreen(IrufemiEngine* engine, float deltaTime) = 0;

    /**
     * @brief ローディング画面の明示的な終了・破棄処理
     */
    virtual void Finalize() {}
};
