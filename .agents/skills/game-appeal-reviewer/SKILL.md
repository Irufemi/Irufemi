---
name: game-appeal-reviewer
description: "Evaluates the overall appeal, core hook, pacing, and technical showcase of the game (Gravity Shooter) from the perspective of players, contest judges, and game industry recruiters."
---

# Game Appeal Reviewer Skill

このスキルは、プロのゲームディレクター、コンテスト審査員、およびゲーム会社の採用担当者（テクニカルディレクター・リードエンジニア）の視点に立ち、**「このゲームの魅力・独自性は何か」「開始30秒でプレイヤーを惹きつけられるか」「自作エンジンの技術的強みが画面に結実しているか」** を客観的かつ厳格に審査・レポートするためのスキルです。

---

## 1. スコープと参照ファイル

デフォルトの対象は **`project/Application_solo`** です。
（ユーザーから「チーム制作用で実行して」などの明示的な指定があった場合のみ `project/Application_team` を対象とします。チーム制作側のファイルを誤って参照・変更しないよう厳格にスコープを分離してください。）

### 主要参照ドキュメント & データ
1. **ゲーム仕様書**:
   - `project/Application_solo/GameDesignDocument.md`（世界観、コアメカニクス、操作感、ボス戦構想）
2. **進捗・タスク管理**:
   - `project/Application_solo/LE3B_15_スエヒロ_コウイチ_タスクリスト.md`
   - `project/Application_solo/LE3B_15_スエヒロ_コウイチ_ロードマップ.md`
3. **シーン・プレハブ構成**:
   - `project/Application_solo/resources/scenes/InGame.json`
   - `project/Application_solo/resources/prefabs/` 配下の各種プレハブ（ガレキ、エフェクト、弾幕）
4. **戦闘・演出・プレイヤー実装**:
   - `project/Application_solo/Combat/`（`PlayerBulletManagerComponent`, `BossComponent`, `BossBulletManagerComponent`, `EnemyComponent` 等）
   - `project/Application_solo/Effects/`（`EffectManagerComponent`, `BossDamageVisualizerComponent` 等）
   - `project/Application_solo/Environment/`（`DebrisManagerComponent` 等）

---

## 2. 審査の4大評価軸

以下の4つの視点からプロジェクトを多角的に分析し、評価を行ってください。

### 軸1: コアフックと独自性（Core Hook & Novelty）
- **開始30秒の体験**: プレイヤーが操作を開始して最初の敵と遭遇するまでの間に、「このゲームならではの強み（『グラビティデイズ』風のガレキ引き寄せ・一斉掃射）」を直感的に味わえる構成になっているか？
- **差別化**: 単に弾を撃ち合う「普通の3Dレールシューティング」に陥っていないか？ 重力アクションとしての必然性と爽快感があるか？

### 軸2: 技術アピールの視覚的結実（Tech-to-Visual Showcase）★就活・ポートフォリオで最重要
- **エンジンの強みが画面に出ているか**:
  - Compute ShaderによるGPUパーティクル
  - 破砕演出（`VoxelParticleManager` / `VoxelParticleComponent`）
  - 高速当たり判定・空間分割（BVH）やオブジェクトプール
  - Prefab Linking による完全データ駆動化
- **説得力**: 技術的な苦労（DirectX12、メモリ管理、ゼロアロケーション）が、ただの自己満足ではなく「大量のガレキが舞い散る派手さ」「60fps/120fpsの滑らかさ」「リッチなエフェクト」という目に見えるゲーム体験として結実しているか？

### 軸3: テンポとドラマチックな起伏（Pacing & Tension Curve）
- **レール進行の緩急**:
  - 移動中に何も起きない「虚無の時間」がないか？
  - 敵の出現パターン（ウェーブ）に緊張と緩和があるか？
- **ボス戦のドラマ性**:
  - チェイスバトル（時間・距離制限内に削り切る焦燥感）から、将来構想であるアリーナ決戦への移行など、クライマックスに向けた盛り上がりが設計されているか？

### 軸4: ビジュアル・世界観の一貫性（Tone & Juice）
- **トーン＆マナー**: 『DEATH STRANDING』風の退廃的な空気感やサイバーパンクの世界観と、エフェクトの色調（シアン、ネオンパープル、オレンジ等）、UI、サウンド構想が一致しているか？
- **ゲームの手応え（Game Juice）**: 攻撃が当たった瞬間のヒット感、爆発の重厚感、敵撃破時の達成感が十分に伝わる設計になっているか？

---

## 3. レポートの出力フォーマット

審査結果は必ず **日本語** で、以下の構成で出力してください。

```markdown
# 🎮 ゲーム魅力・ポートフォリオ総合審査レポート

## ■ 総合評価
- **総合ランク**: 【 S / A / B / C 】
- **一言要約**: （例: 「自作エンジンのボクセル技術と重力アクションが美しく融合。あとは敵ウェーブの緩急が揃えば即座にコンテスト上位を狙える作品」）

## ■ 4大評価軸の詳細採点
| 評価軸 | ランク | 評価コメント |
| :--- | :---: | :--- |
| **1. コアフック・独自性** | A | ... |
| **2. 技術アピール度** | S | ... |
| **3. テンポ・起伏** | B | ... |
| **4. 世界観・演出の一致** | A | ... |

## ■ 審査員・採用担当者目線での「キラーポイント（絶賛点）」
- （ポートフォリオや面接で最も刺さる強みを箇条書きで具体的に記載）

## ■ 伸びしろ・惜しいポイント（改善の余地）
- （何が欠けていて、どうすればさらに魅力が跳ね上がるかを指摘）

## ■ 今すぐできる魅力爆上げアクション TOP 3
1. 【高優先度】...
2. 【中優先度】...
3. 【低優先度】...
```
