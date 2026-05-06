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


    // --- スタック管理機能 ---
    // このシーンが下のシーンの更新(Update)をブロックするか（デフォルトはブロックする）
    virtual bool IsUpdateBlocking() const { return true; }
    
    // このシーンが下のシーンの描画(Draw)をブロックするか（デフォルトはブロックしない）
    virtual bool IsDrawBlocking() const { return false; }
    
    // このシーンでマウスカーソルを表示するか（デフォルトは表示する）
    virtual bool IsCursorVisible() const { return true; }
};