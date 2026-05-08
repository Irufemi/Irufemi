# IrufemiEngine 実装・リファクタリング計画

## ✅ [完了] フェーズ3: レンダーグラフ（Frame Graph）の導入とパケット分離
- [x] **RenderGraphの実装**: `IRenderPass` を基底とした描画パス (`ShadowPass`, `MainOpaquePass`, `MainTransparentPass`, `UIPass`) の構築。
- [x] **DrawManagerの分割**: 固定的な描画パイプラインを疎結合なパスの集合に置き換え。
- [x] **RenderPacketsの分離**: `DrawManager` に依存していた描画パケット群を `RenderPackets.h` に分離。パスのラムダ引数を `auto` 化し、簡潔で依存の少ないコードベースに整理。
- [x] **取扱説明書 (Manual.md) の作成**: エンジン機能の使い方やアーキテクチャルールをまとめたドキュメントの作成。

---

## 🚀 [進行中] フェーズ4: 計算（Compute）パスの統合と最適化

現在の基盤を活かし、さらなるパフォーマンスと拡張性を得るためのアーキテクチャ最適化フェーズ。

### 1. Compute Shader の RenderGraph 完全統合（優先度：高）
現在 `DrawManager::ExecuteComputeTasks()` でグラフ外で実行されているコンピュート処理（GPUパーティクルの更新等）を、レンダーグラフの仕組みに取り込む。
- [x] `IComputePass`（または `ComputePass`）クラスを新設し、`RenderGraph` に登録できるようにする。
- [x] 「計算 (Compute) → リソースバリア (UAV) → 描画 (Graphics)」というGPUの全処理フローを RenderGraph 1つで一元管理し、競合バグを根本から防ぐ。
- [x] 既存の `DrawManager::ExecuteComputePasses` を直接呼び出していた部分を廃止・移行する。

### 2. ポストプロセス用リソースの再利用（Transient Resources）（優先度：中）
マルチパスレンダリング（ブルーム、被写界深度等）による VRAM 消費を抑えるためのメモリ管理システム。
- [ ] RenderGraph内で、あるパスの出力テクスチャが不要になったら、次のパスの入力テクスチャとしてメモリ（VRAM）を使い回す（エイリアシング）仕組みを導入。

### 3. ECS (Entity Component System) への移行準備（優先度：低）
アプリケーション側のオブジェクト増加に備えたモダンな設計への準備。
- [ ] 「Update（更新）」するデータと「Draw（描画）」するデータを完全に分離し、`Player` や `EnemyBeam` などがそれぞれ描画パケットを投げる現状の設計を見直す基盤を作る。