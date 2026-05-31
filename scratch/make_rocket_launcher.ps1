$PI = [Math]::PI
$SEGS = 12 # 4本あるので少しポリゴン数を抑えめに12分割
$R = 0.28  # チューブの半径

# ポッドケース本体サイズ
$BW = 1.0
$BH = 0.85
$BD = 1.5

# チューブ位置オフセット (2x2)
$offsetX = 0.45
$offsetY = 0.38

# チューブのZ範囲 (少し前へ突き出させ、後部は奥める)
$Z_START = 1.2
$Z_END = -1.8

$sb = [System.Text.StringBuilder]::new()

$sb.AppendLine("# Player Rocket Launcher - playerRocketLauncher.obj") | Out-Null
$sb.AppendLine("mtllib playerRocketLauncher.mtl") | Out-Null
$sb.AppendLine("o playerRocketLauncher") | Out-Null

# === 1. 頂点 (Vertices) ===

# ポッド本体 Box 8頂点 (idx 1-8)
$BVS = @(
    @(-$BW, -$BH, -$BD), @(-$BW, $BH, -$BD),
    @( $BW, -$BH, -$BD), @( $BW, $BH, -$BD),
    @(-$BW, -$BH,  $BD), @(-$BW, $BH,  $BD),
    @( $BW, -$BH,  $BD), @( $BW, $BH,  $BD)
)
foreach ($v in $BVS) {
    $sb.AppendLine("v " + $v[0].ToString("F6") + " " + $v[1].ToString("F6") + " " + $v[2].ToString("F6")) | Out-Null
}

$v_count = 8

# 4本のチューブ頂点生成
$tubeCenters = @(
    @($offsetX, $offsetY),
    @(-$offsetX, $offsetY),
    @($offsetX, -$offsetY),
    @(-$offsetX, -$offsetY)
)

# 各チューブの開始インデックスとキャップ中心を記録する配列
$tubeStartIdxs = @()
$tubeCapCenters = @()

foreach ($center in $tubeCenters) {
    $cx = $center[0]
    $cy = $center[1]
    
    # 根元（後端）の円周頂点
    $start_idx = $v_count + 1
    $tubeStartIdxs += $start_idx
    
    for ($i = 0; $i -lt $SEGS; $i++) {
        $a = 2.0 * $PI * $i / $SEGS
        $x = $cx + $R * [Math]::Cos($a)
        $y = $cy + $R * [Math]::Sin($a)
        $sb.AppendLine("v " + $x.ToString("F6") + " " + $y.ToString("F6") + " " + $Z_START.ToString("F6")) | Out-Null
    }
    
    # 先端（前端）の円周頂点
    for ($i = 0; $i -lt $SEGS; $i++) {
        $a = 2.0 * $PI * $i / $SEGS
        $x = $cx + $R * [Math]::Cos($a)
        $y = $cy + $R * [Math]::Sin($a)
        $sb.AppendLine("v " + $x.ToString("F6") + " " + $y.ToString("F6") + " " + $Z_END.ToString("F6")) | Out-Null
    }
    
    # 先端キャップの中心頂点
    $cap_center = $v_count + 2 * $SEGS + 1
    $tubeCapCenters += $cap_center
    $sb.AppendLine("v " + $cx.ToString("F6") + " " + $cy.ToString("F6") + " " + $Z_END.ToString("F6")) | Out-Null
    
    $v_count += 2 * $SEGS + 1
}

# === 2. 法線 (Normals) ===

# 基本法線 6方向
$sb.AppendLine("vn 0.0000 0.0000 -1.0000") | Out-Null   # 1: 前
$sb.AppendLine("vn 0.0000 0.0000  1.0000") | Out-Null   # 2: 後
$sb.AppendLine("vn -1.0000 0.0000 0.0000") | Out-Null   # 3: 左
$sb.AppendLine("vn  1.0000 0.0000 0.0000") | Out-Null   # 4: 右
$sb.AppendLine("vn 0.0000 -1.0000 0.0000") | Out-Null   # 5: 下
$sb.AppendLine("vn 0.0000  1.0000 0.0000") | Out-Null   # 6: 上

$n_count = 6

# 各チューブの円周法線
$tubeNormalsStart = @()
foreach ($center in $tubeCenters) {
    $tubeNormalsStart += ($n_count + 1)
    for ($i = 0; $i -lt $SEGS; $i++) {
        $a = 2.0 * $PI * ($i + 0.5) / $SEGS
        $nx = [Math]::Cos($a)
        $ny = [Math]::Sin($a)
        $sb.AppendLine("vn " + $nx.ToString("F4") + " " + $ny.ToString("F4") + " 0.0000") | Out-Null
    }
    $n_count += $SEGS
}

# === 3. UV (Texture Coordinates) ===
# ネイビー用 (下半分 V:0.0~0.5)
$sb.AppendLine("vt 0.0000 0.0000") | Out-Null  # 1
$sb.AppendLine("vt 1.0000 0.0000") | Out-Null  # 2
$sb.AppendLine("vt 1.0000 0.5000") | Out-Null  # 3
$sb.AppendLine("vt 0.0000 0.5000") | Out-Null  # 4

# シアン用 (上半分 V:0.5~1.0)
$sb.AppendLine("vt 0.0000 0.5000") | Out-Null  # 5
$sb.AppendLine("vt 1.0000 0.5000") | Out-Null  # 6
$sb.AppendLine("vt 1.0000 1.0000") | Out-Null  # 7
$sb.AppendLine("vt 0.0000 1.0000") | Out-Null  # 8

$sb.AppendLine("s 0") | Out-Null

# ヘルパー関数：face文字列 v/t/n形式
function FV([int]$v, [int]$t, [int]$n) {
    return ($v.ToString() + "/" + $t.ToString() + "/" + $n.ToString())
}

# === 4. 面定義 (Faces) ===

# --- ポッドケース本体 (Mtl_DarkMetal = ネイビー) ---
# ★裏面カリング対策：頂点の並び順を時計回り（CW）へと反転
$sb.AppendLine("usemtl Mtl_DarkMetal") | Out-Null

# 前面 z=-BD: v1,v2,v4,v3 (元: 3 4 2 1)
$sb.AppendLine("f " + (FV 1 4 1) + " " + (FV 2 3 1) + " " + (FV 4 2 1) + " " + (FV 3 1 1)) | Out-Null

# 後面 z=BD: v7,v8,v6,v5 (元: 5 6 8 7)
$sb.AppendLine("f " + (FV 7 4 2) + " " + (FV 8 3 2) + " " + (FV 6 2 2) + " " + (FV 5 1 2)) | Out-Null

# 左面 -x: v5,v6,v2,v1 (元: 1 2 6 5)
$sb.AppendLine("f " + (FV 5 4 3) + " " + (FV 6 3 3) + " " + (FV 2 2 3) + " " + (FV 1 1 3)) | Out-Null

# 右面 +x: v8,v7,v3,v4 (元: 4 3 7 8)
$sb.AppendLine("f " + (FV 8 4 4) + " " + (FV 7 3 4) + " " + (FV 3 2 4) + " " + (FV 4 1 4)) | Out-Null

# 底面 -y: v5,v7,v3,v1 (元: 1 3 7 5)
$sb.AppendLine("f " + (FV 5 4 5) + " " + (FV 7 3 5) + " " + (FV 3 2 5) + " " + (FV 1 1 5)) | Out-Null

# 上面 +y: v6,v8,v4,v2 (元: 2 4 8 6)
$sb.AppendLine("f " + (FV 6 4 6) + " " + (FV 8 3 6) + " " + (FV 4 2 6) + " " + (FV 2 1 6)) | Out-Null


# --- 4本の発射管チューブ (Mtl_LightMetal = シアン) ---
# ★裏面カリング対策：頂点の並び順を時計回り（CW）へと反転
$sb.AppendLine("usemtl Mtl_LightMetal") | Out-Null

for ($tIdx = 0; $tIdx -lt 4; $tIdx++) {
    $BACK_OFF = $tubeStartIdxs[$tIdx]
    $FRONT_OFF = $BACK_OFF + $SEGS
    $CAP_CENTER = $tubeCapCenters[$tIdx]
    $normStart = $tubeNormalsStart[$tIdx]
    
    # 側面 (円柱の周りの面)
    for ($i = 0; $i -lt $SEGS; $i++) {
        $ni = $normStart + $i
        
        $b0 = $BACK_OFF + $i
        $b1 = $BACK_OFF + (($i + 1) % $SEGS)
        
        $f0 = $FRONT_OFF + $i
        $f1 = $FRONT_OFF + (($i + 1) % $SEGS)
        
        # 逆順マッピング
        $sb.AppendLine("f " + (FV $f0 8 $ni) + " " + (FV $f1 7 $ni) + " " + (FV $b1 6 $ni) + " " + (FV $b0 5 $ni)) | Out-Null
    }
    
    # 先端キャップ (弾の発射口部分)
    for ($i = 0; $i -lt $SEGS; $i++) {
        $f0 = $FRONT_OFF + $i
        $f1 = $FRONT_OFF + (($i + 1) % $SEGS)
        
        # 逆順マッピング
        $sb.AppendLine("f " + (FV $f0 7 1) + " " + (FV $f1 6 1) + " " + (FV $CAP_CENTER 5 1)) | Out-Null
    }
}

# === 5. 出力 ===
$OUT = "c:\Users\k024g\Desktop\TD3_1\project\Application\resources\model\player\playerRocketLauncher.obj"

# 親ディレクトリ作成
$parentDir = Split-Path $OUT
if (-not (Test-Path $parentDir)) {
    New-Item -ItemType Directory -Force -Path $parentDir | Out-Null
}

[System.IO.File]::WriteAllText($OUT, $sb.ToString())
Write-Host "OBJ written: $OUT"

# === 6. MTLファイル生成 ===
$mtl = @"
# playerRocketLauncher.mtl

newmtl Mtl_DarkMetal
Ns 60.0
Ka 0.900000 0.900000 0.900000
Kd 0.050000 0.120000 0.360000
Ks 0.400000 0.400000 0.400000
Ke 0.000000 0.000000 0.000000
Ni 1.500000
d 1.000000
illum 3
map_Kd ../../texture/player/playerRocketLauncher.png

newmtl Mtl_LightMetal
Ns 80.0
Ka 0.900000 0.900000 0.900000
Kd 0.000000 0.750000 1.000000
Ks 0.500000 0.500000 0.500000
Ke 0.000000 0.000000 0.000000
Ni 1.500000
d 1.000000
illum 3
map_Kd ../../texture/player/playerRocketLauncher.png
"@

$OUTM = "c:\Users\k024g\Desktop\TD3_1\project\Application\resources\model\player\playerRocketLauncher.mtl"
[System.IO.File]::WriteAllText($OUTM, $mtl)
Write-Host "MTL written: $OUTM"
