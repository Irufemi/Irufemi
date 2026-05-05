#pragma once

#include <cstdint>

// 前方宣言
class IrufemiEngine;

/// <summary>
/// Scene系クラスに継承する基底クラス
/// </summary>
class IScene {
public:
    virtual ~IScene() = default;
    virtual void Initialize(IrufemiEngine* engine) = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;

    // --- デバッグ機能 ---
    // エンジン共通のデバッグウィンドウにタブを追加する
    virtual void DrawDebugTab() {}


    // --- ポーズ機能 ---
    // ポーズ中の更新(デフォルトは空実装)
    virtual void PauseUpdate() {}
    // ポーズ中の描画(デフォルトは空実装)
    virtual void PauseDraw() {}
    // このシーンがポーズ可能か(デフォルトは不可)
    virtual bool IsPausable() const { return false; }
};