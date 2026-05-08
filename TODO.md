# IrufemiEngine 実装・リファクタリング計画

## [進行中] レンダーグラフ（Frame Graph）の導入

現在の固定的な描画パイプライン（`DrawManager::ExecuteRenderQueues`）を疎結合な「RenderPass」の集合体に分割し、保守性と拡張性（新しい描画表現の追加）を向上させる。

### 1. クラス設計と新規ファイル
`IrufemiEngine/Engine/Graphics/Pipeline/RenderGraph/` ディレクトリを新設し、以下のクラスを追加する。

#### 基底インターフェース `IRenderPass`
すべての描画パスの基底となるクラス。
```cpp
class IRenderPass {
public:
    virtual ~IRenderPass() = default;
    
    // パスが必要とする入力リソース・出力リソースを宣言する（将来のバリア自動解決用）
    virtual void Setup() {} 
    
    // 実際の描画コマンド（DrawCall）を発行する
    virtual void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) = 0;
};
```

#### 管理クラス `RenderGraph`
登録された `IRenderPass` を管理し、順次実行する。
```cpp
class RenderGraph {
public:
    void AddPass(std::unique_ptr<IRenderPass> pass);
    void Execute(DrawManager* drawManager, IrufemiEngine* engine);
private:
    std::vector<std::unique_ptr<IRenderPass>> passes_;
};
```

### 2. DrawManager の分割タスク
現在の `DrawManager` にハードコードされている描画手順を以下のパスに切り出す。

- [x] **`ShadowPass`**: シャドウマップへの深度描画を行う。(`BeginShadowPass` ～ `EndShadowPass` の処理)
- [x] **`MainOpaquePass`**: 不透明オブジェクト（Skybox, 3Dモデル, Regionなど）の描画。深度書き込みON。
- [x] **`MainTransparentPass`**: 半透明オブジェクト（パーティクル、ライン、GPUパーティクルなど）の描画。深度書き込みOFFのアルファブレンド。
- [x] **`PostProcessPass`**: `PostProcessManager` と連携し、レンダリング結果にエフェクトを適用。
- [x] **`UIPass`**: 最前面スプライトや2Dオブジェクトの描画。

### 3. DrawManager 側の修正
- [x] 各 RenderPass が描画キュー（`standard3DQueue_` 等）にアクセスできるよう、`DrawManager` に `GetStandard3DQueue()` などのアクセサ（Getter）を追加する。
- [x] `DrawManager::ExecuteRenderQueues()` の中身を削除し、代わりに `renderGraph_->Execute(this, engine);` を呼び出すようにする。

### 4. （将来拡張）リソーストラッキングと自動バリア
- [x] 各 Pass の `Setup()` 経由で「どの RenderTarget を読み書きするか」をグラフに登録する。
- [x] UAVやSRVの `TransitionBarrier` を `RenderGraph` 側で自動発行する仕組みを構築する。