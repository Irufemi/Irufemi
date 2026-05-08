#pragma once

// 前方宣言
class DrawManager;
class IrufemiEngine;

/**
 * @class IRenderPass
 * @brief すべての描画パスの基底となるインターフェース
 * @details レンダーグラフに登録され、特定の描画工程（シャドウ、不透明、半透明など）をカプセル化します。
 */
class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    /**
     * @brief パスのセットアップ処理
     * @details 将来的なリソースバリアの自動解決のため、入力・出力リソースの宣言を行う際に使用します。
     *          （現在はプレースホルダーとして用意）
     */
    virtual void Setup() {}

    /**
     * @brief パスの実行処理
     * @param[in] drawManager 描画コマンドを発行するための DrawManager
     * @param[in] engine エンジン本体（各種マネージャーへのアクセス用）
     * @details 実際の DrawCall（描画コマンドの積み込み）をこの中で行います。
     */
    virtual void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) = 0;
};
