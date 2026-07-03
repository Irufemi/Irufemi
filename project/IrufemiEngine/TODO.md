# 🎮 IrufemiEngine コア開発＆拡張計画

以下は、`IrufemiEngine`（エンジン側）の次期アップデート計画や、将来的なエディタ拡張機能などのTODOリストです。
※ゲーム固有のロジックやタスクは `Application/TODO.md` で管理します。

## 🚀 次期アップデート計画 (Ongoing & Future Tasks)
より「ツール」として使いやすく、商用水準のパフォーマンスを発揮するための拡張機能群です。

### ⚡ パフォーマンスと最適化 (Performance & Optimization)
### ⚡ 次世代パフォーマンス・アーキテクチャ最適化 (Next-Gen AAA Performance)
- [x] **CollisionManager のスレッドセーフ化と Async Raycast の実装**
    - `shared_mutex` を用いたRead-Write Lockの導入と、ThreadPoolと連携した非同期物理クエリAPIの提供。（完了）
- [ ] **Bindless Resources (Descriptor Indexing) の完全移行完了**
    - 全テクスチャ/リソースを巨大な Descriptor Heap に格納し、Shader にインデックス(uint)だけを渡す方式へ移行。
    - **進行状況**: C++基盤、および `Application_solo` / `Application_team` 双方のほぼすべてのHLSLファイルの移行が完了。
    - **残タスク**: エディタ用シェーダー `OutlineComposite.PS.hlsl` と `SelectionOutlinePass.cpp` を Bindless 経由でテクスチャを読むように修正する。
    - **残タスク**: 上記の完了後、互換維持のために復刻した `RootSlot::LegacyPSTexture` を削除し、真の Bindless Root Signature を完成させる。
    - **残タスク**: 既存の `RenderPackets` 系（PrimitiveBatchPacket, ModelBatchPacket 等）に残存している明示的なテクスチャバインドのコードを完全に撤去し、コードをシンプルにする。
- [ ] **マルチスレッドコマンド録画 (Multi-threaded Command Recording)**
    - Job System と連携し、D3D12 の CommandList 構築を複数スレッドで並列に行い、CPUの描画ボトルネックを解消する。
- [ ] **DirectStorage API / Virtual Texturing の対応**
    - NVMe SSD から VRAM へのダイレクト転送（CPUバイパス）と、画面に映っているミップレベルだけをオンデマンドロードする Sampler Feedback 機構の実装。
- [ ] **Clustered Shading (Forward+) または Deferred Rendering の実装**
    - 現在の固定長定数バッファによるライト管理から脱却し、数百〜数千の動的ライトを効率的に処理するライティング基盤の構築。
- [ ] **Compute Skinning (GPUスキニング) の実装**
    - CPUで行っているアニメーションのボーン行列計算と頂点ブレンドを Compute Shader にオフロードする。
- [ ] **PSO (Pipeline State Object) キャッシュとバックグラウンドコンパイル**
    - ゲームプレイ中のカクつき（Stutter）を防ぐため、バックグラウンドでの事前コンパイルおよびディスクキャッシュ機構の構築。
- [ ] **GameObjectアーキテクチャの完全データ指向（ECS）への刷新（大手術）**
    - 現在の「ツリー構造（親子関係）＋ポインタベースのComponent」というオブジェクト指向の限界を突破するため、AAA基準の純粋な ECS (Entity Component System) へとエンジン根幹のアーキテクチャを書き換える。
    - EntityはただのID（数値）とし、全てのComponentを種類ごとの巨大な連続配列（SoA: Structure of Arrays）で管理。これによりCPUのキャッシュミスを極限まで減らし、数万〜数十万のオブジェクト更新をフレームレート低下なしで処理可能にする。
    - ※段階的移行として、エディタ上では従来の GameObject の見た目を保ちつつ、ランタイム実行時やビルド時に内部で純粋なデータ配列へと自動変換（Baking）する「ハイブリッド方式」から導入する。
- [ ] **空間分割 (Spatial Partitioning: Octree / BVH) の導入**
    - 現在 `CollisionManager` が総当りの二重ループ（$O(N^2)$）で判定しているため、大量の弾やオブジェクトが存在するとCPUが破綻する問題を解消。Broad-phase（広域判定）用のOctreeやBVHを構築し、計算量を $O(N \log N)$ に削減する。
- [ ] **SIMD (DirectXMath / SSE) を活用した算術ライブラリの刷新**
    - 現在の `Vector3` や行列計算がスカラ演算（float単位）で実装されているため、DirectXMath (`XMVECTOR`, `XMMATRIX`) などの SIMD 命令にバックエンドを差し替え、物理・Transform計算のボトルネックを解消する。
- [ ] **`StringId` (高速な文字列ハッシュ化) システムの導入**
    - `GameObject` の検索やタグ比較で多発している `std::string` のアロケーション・比較コストを削るため、コンパイル時ハッシュ等を利用したIDベースの文字列プールシステム（Unreal Engineの `FName` に相当）を導入する。
- [ ] **エンジングローバルなメモリアロケータの最適化**
    - `new / delete` のOS側のヒープ競合を防ぐため、汎用的なアロケータのオーバーライド（例: `mimalloc` や独自TLSFアロケータの組み込み）を行い、ゲームループ中の動的メモリ確保を高速化する。
- [ ] **アセットのバイナリベイク（JSONシリアライズからの脱却）**
    - 現在シーンデータや設定ファイルが `json` で保存・パースされているが、文字列パースはロード時間の大きなボトルネックになる。製品ビルド時には `FlatBuffers` や独自バイナリ形式にベイクし、ゼロコピーでメモリに展開する超高速ロードを実現する。
- [ ] **イベント駆動（Delegate / Message Bus）システムの構築**
    - コンポーネント同士のやり取り（例: 当たり判定から特定スクリプトを呼ぶ等）のハードコードな依存関係を無くすため、グローバルな Pub/Sub イベントバスや C++ Delegate 機構を導入し、疎結合なアーキテクチャにする。
- [ ] **固定フレームレート物理演算 (Fixed Timestep) の実装**
    - 現在のゲームループはフレームレート依存で Update が回っている。物理挙動の「抜け」や「デバイス毎の挙動の違い」を防ぐため、描画とは独立して必ず一定間隔（例: 60Hz/120Hz）で物理判定を回す `FixedUpdate` 機構を導入する。
- [ ] **FMOD (オーディオミドルウェア) の統合**
    - 現在のXAudio2ベースのオーディオ管理から、AAA業界標準である **FMOD** （NieR:Automata等で採用）へと完全に移行する。これにより、複雑なストリーミング再生や3D空間音響、動的なBGM遷移（インタラクティブオーディオ）を専用ツール(FMOD Studio)と連携して実現する。

### ✨ エディタの高度化と表現力の向上 (Editor & Rendering Features)
- [ ] **パーティクルエディタの充実化**
    - インスペクターからの直感的なスライダー操作とリアルタイムプレビューによるエフェクト作成環境の強化。
- [ ] **マテリアルエディタ＆シェーダバリアント**
    - Normal Map、Emission等のPBRパラメータをエディタで設定・即時反映する機能。
- [ ] **タイムライン/Tweenアニメーションシステム**
    - UIやオブジェクトの移動・フェードなどをエディタ上でキーフレーム設定・再生できる仕組みの導入。
- [ ] **エディタの選択状態（Selection）の安全な破棄・クリア機構**
    - シーンからオブジェクトが `RemoveGameObject` または `Destroy` された際に、エディタのインスペクター描画対象（選択中オブジェクト）に残存してクラッシュ（`GetScene()` == nullptr 等）を引き起こす問題の根本解決。破棄イベントのフックや Weak_ptr による管理を導入する。



---
<br><br><br><br>

## 📦 アーカイブ (完了済みタスク)
以下は過去のフェーズ（フェーズ1〜6）で達成・実装済みの機能一覧です。

<details>
<summary>▶ 完了したアーキテクチャと機能一覧を開く</summary>

### フェーズ8: リソース管理システムの抜本的改革（AAA基準）
- [x] **`ResourceHandle` および `ResourceCachePool` の実装**
    - [x] `std::shared_ptr` の依存を排除し、独自の ID（Handle） ベースの高速なアロケータを構築。
- [x] **`TextureManager` の Handle 化**
    - [x] メモリリーク防止と非同期ロードの安定化。
- [x] **`ModelManager` の Handle 化**
    - [x] VRAM メモリパージ機構を想定したアーキテクチャへの刷新。
- [x] **各種コンポーネント（MeshRenderer, AnimationModel 等）の対応**

### フェーズ7: アーキテクチャとパフォーマンスの最適化
- [x] **GPUParticleSystemのパフォーマンス最適化と共有化**
    - [x] `GPUParticleManager`（または類似の共有機構）の設計・実装。
    - [x] 各 `ParticleEmitterComponent` から放出命令（Emit）を一つの中央バッファに集約する（Instanced Emission）アーキテクチャへの改修。
    - [x] パフォーマンス測定と、安定性の確認。
    - [x] 最適化完了後、旧 `ParticleSystem` (CPU) の完全削除とコードベース統合。

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
</details>
