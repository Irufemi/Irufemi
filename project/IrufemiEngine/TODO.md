# 🎮 IrufemiEngine コア開発＆拡張計画

以下は、`IrufemiEngine`（エンジン側）の次期アップデート計画や、将来的なエディタ拡張機能などのTODOリストです。
※ゲーム固有のロジックやタスクは `Application/TODO.md` で管理します。

## 🚀 次期アップデート計画 (Ongoing & Future Tasks)
より「ツール」として使いやすく、商用水準のパフォーマンスを発揮するための拡張機能群です。

### ⚡ パフォーマンスと最適化 (Performance & Optimization)
- [ ] **GPUParticleSystemのパフォーマンス最適化と共有化 (現在進行中)**
    - [ ] `GPUParticleManager`（または類似の共有機構）の設計・実装。
    - [ ] 各 `ParticleEmitterComponent` から放出命令（Emit）を一つの中央バッファに集約する（Instanced Emission）アーキテクチャへの改修。
    - [ ] パフォーマンス測定と、安定性の確認。
    - [ ] 最適化完了後、旧 `ParticleSystem` (CPU) の完全削除とコードベース統合。
- [ ] **メモリ使用量のプロファイリングと最適化**
    - `TextureManager` および `ModelManager` の未使用リソース自動開放（LRUキャッシュ等）の検討。
- [ ] **製品版（Releaseビルド）におけるエディタ完全削除の検証**
    - `EditorMode` マクロがオフの時に、1バイトも不要なコードが含まれないことの確認。

### ✨ エディタの高度化と表現力の向上 (Editor & Rendering Features)
- [ ] **パーティクルエディタの充実化**
    - インスペクターからの直感的なスライダー操作とリアルタイムプレビューによるエフェクト作成環境の強化。
- [ ] **マテリアルエディタ＆シェーダバリアント**
    - Normal Map、Emission等のPBRパラメータをエディタで設定・即時反映する機能。
- [ ] **タイムライン/Tweenアニメーションシステム**
    - UIやオブジェクトの移動・フェードなどをエディタ上でキーフレーム設定・再生できる仕組みの導入。


---
<br><br><br><br>

## 📦 アーカイブ (完了済みタスク)
以下は過去のフェーズ（フェーズ1〜6）で達成・実装済みの機能一覧です。

<details>
<summary>▶ 完了したアーキテクチャと機能一覧を開く</summary>

### フェーズ1: コンポーネント指向アーキテクチャの完成
- [x] すべてのエンティティのベースとなる `GameObject` クラスの作成
- [x] `GameObject` にアタッチできる `Component` 基底クラスの作成
- [x] 基本コンポーネント群（`Transform`, `MeshRenderer`, `PrimitiveRenderer`, `SpriteRenderer`, `Collider`等）の実装
- [x] 【UI分離】コンポーネントから ImGui 依存を完全に排除（`ComponentEditorRegistry` への移行）
- [x] 【Fat Class解消】`ObjClass` 等の内部トランスフォーム依存を排除し、コンポーネント主導の描画へ移行

### フェーズ2: エディタ基盤機能の構築
- [x] `EditorManager` によるエディタ機能の統括（`#ifdef EditorMode`）
- [x] 「ヒエラルキー（Hierarchy）パネル」による全GameObjectのツリー管理
- [x] 「インスペクター（Inspector）パネル」によるコンポーネントパラメータの動的編集
- [x] `ImGuizmo` を使用したシーンビュー上での直感的なトランスフォーム操作
- [x] 「シーンビュー（SceneView）パネル」による描画結果のプレビューとデバッグ表示切替

### フェーズ3: エディタの高度化と操作性の向上
- [x] **コマンドパターンによる Undo/Redo システムの実装**
- [x] **プロジェクトブラウザ（ProjectBrowser）の最適化** (メモリキャッシュと手動更新)
- [x] **自動ファイル監視システム (DirectoryWatcher) の構築** (Windows APIによる自動検知)
- [x] **ピッキング機能の精緻化** (アウトライン表示の追加)
- [x] **アセットのドラッグ＆ドロップ連携の強化** (シーンへのD&D生成)

### フェーズ4: シーン管理とシリアライズ
- [x] `GameObject` / `Component` の JSON シリアライズ・デシリアライズ基盤の実装
- [x] **エディタ上でのシーン保存・読込UI（File Menu）の実装**
- [x] **起動時の初期シーン自動ロード・自動生成機能**
- [x] プレハブ（Prefab）システムの導入

### フェーズ5: ランタイム連携とデプロイ
- [x] **Play Mode / Edit Mode の完全分離** (シーンの一時保存・復元)
- [x] **完全データ駆動化への移行 (スクリプトのコンポーネント化)**

### フェーズ6: ソロ制作向け・実戦用ゲームエンジンの完成
- [x] **自作スクリプト（Component）作成の効率化・自動化** (マクロ導入)
- [x] **ゲームロジック・イベント連携の強化** (`OnCollisionEnter` コールバック対応等)
- [x] **2D UI（キャンバス）システムの実装** (`ButtonComponent`, `CanvasComponent`)
- [x] **マルチメディアコンポーネントのエディタ対応** (`AudioSourceComponent` 等)
- [x] **シーン遷移のデータ駆動化** (`SceneManager::LoadScene`)

### その他 実装済みの高度な機能
- [x] **ランタイムMSDF動的生成によるテキスト描画 (`TextComponent`)**
- [x] **コライダーの視覚化と直感的なリサイズ操作** (ImGuizmo連携)
- [x] **アプリケーション終了時のリソースリーク（LIVE_DEVICE）の解消**
- [x] **ShaderToyからの高度なエフェクト移植とマテリアル化** (EnergyCore)

</details>