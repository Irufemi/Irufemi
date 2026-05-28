$PI = [Math]::PI
$SEGS = 16
$BARREL_R = 0.45
$BARREL_LEN = 5.0
$BW = 1.0; $BH = 1.0; $BD = 1.5
$Z_START = -$BD
$Z_END = $Z_START - $BARREL_LEN

$sb = [System.Text.StringBuilder]::new()

$sb.AppendLine("# Simple Cannon - playerCannon.obj") | Out-Null
$sb.AppendLine("mtllib playerCannon.mtl") | Out-Null
$sb.AppendLine("o playerCannon") | Out-Null

# === 頂点 ===
# Box 8頂点 (idx 1-8)
$BVS = @(
    @(-$BW, -$BH, -$BD), @(-$BW, $BH, -$BD),
    @( $BW, -$BH, -$BD), @( $BW, $BH, -$BD),
    @(-$BW, -$BH,  $BD), @(-$BW, $BH,  $BD),
    @( $BW, -$BH,  $BD), @( $BW, $BH,  $BD)
)
foreach ($v in $BVS) {
    $sb.AppendLine("v " + $v[0].ToString("F6") + " " + $v[1].ToString("F6") + " " + $v[2].ToString("F6")) | Out-Null
}

# 砲身根元 (idx 9 .. 9+SEGS-1)
$BACK_OFF = 9
for ($i = 0; $i -lt $SEGS; $i++) {
    $a = 2.0 * $PI * $i / $SEGS
    $x = $BARREL_R * [Math]::Cos($a)
    $y = $BARREL_R * [Math]::Sin($a)
    $sb.AppendLine("v " + $x.ToString("F6") + " " + $y.ToString("F6") + " " + $Z_START.ToString("F6")) | Out-Null
}

# 砲身先端 (idx 9+SEGS .. 9+2*SEGS-1)
$FRONT_OFF = 9 + $SEGS
for ($i = 0; $i -lt $SEGS; $i++) {
    $a = 2.0 * $PI * $i / $SEGS
    $x = $BARREL_R * [Math]::Cos($a)
    $y = $BARREL_R * [Math]::Sin($a)
    $sb.AppendLine("v " + $x.ToString("F6") + " " + $y.ToString("F6") + " " + $Z_END.ToString("F6")) | Out-Null
}

# 先端キャップ中心 (idx 9+2*SEGS)
$CAP_CENTER = 9 + 2 * $SEGS
$sb.AppendLine("v 0.000000 0.000000 " + $Z_END.ToString("F6")) | Out-Null

# === 法線 ===
$sb.AppendLine("vn 0.0000 0.0000 -1.0000") | Out-Null   # 1
$sb.AppendLine("vn 0.0000 0.0000  1.0000") | Out-Null   # 2
$sb.AppendLine("vn -1.0000 0.0000 0.0000") | Out-Null   # 3
$sb.AppendLine("vn  1.0000 0.0000 0.0000") | Out-Null   # 4
$sb.AppendLine("vn 0.0000 -1.0000 0.0000") | Out-Null   # 5
$sb.AppendLine("vn 0.0000  1.0000 0.0000") | Out-Null   # 6
for ($i = 0; $i -lt $SEGS; $i++) {
    $a = 2.0 * $PI * ($i + 0.5) / $SEGS
    $nx = [Math]::Cos($a)
    $ny = [Math]::Sin($a)
    $sb.AppendLine("vn " + $nx.ToString("F4") + " " + $ny.ToString("F4") + " 0.0000") | Out-Null  # 7+i
}

# === UV ===
# ネイビー用 (左60%)
$sb.AppendLine("vt 0.0000 0.0000") | Out-Null  # 1
$sb.AppendLine("vt 0.6000 0.0000") | Out-Null  # 2
$sb.AppendLine("vt 0.6000 1.0000") | Out-Null  # 3
$sb.AppendLine("vt 0.0000 1.0000") | Out-Null  # 4
# シアン用 (右40%)
$sb.AppendLine("vt 0.6500 0.0000") | Out-Null  # 5
$sb.AppendLine("vt 1.0000 0.0000") | Out-Null  # 6
$sb.AppendLine("vt 1.0000 1.0000") | Out-Null  # 7
$sb.AppendLine("vt 0.6500 1.0000") | Out-Null  # 8

$sb.AppendLine("s 0") | Out-Null

# ヘルパー：face文字列 v/t/n形式
function FV([int]$v, [int]$t, [int]$n) { return ($v.ToString() + "/" + $t.ToString() + "/" + $n.ToString()) }

# === Box面（DarkMetal = ネイビー）===
# ★裏面カリング対策：頂点の並び順を時計回り（CW）へと反転
$sb.AppendLine("usemtl Mtl_DarkMetal") | Out-Null
# 前面 z=-BD: v1,v2,v4,v3 (元: 3 4 2 1)
$sb.AppendLine("f " + (FV 1 4 1) + " " + (FV 2 3 1) + " " + (FV 4 2 1) + " " + (FV 3 1 1)) | Out-Null
# 後面 z=+BD: v7,v8,v6,v5 (元: 5 6 8 7)
$sb.AppendLine("f " + (FV 7 4 2) + " " + (FV 8 3 2) + " " + (FV 6 2 2) + " " + (FV 5 1 2)) | Out-Null
# 左面 -x: v5,v6,v2,v1 (元: 1 2 6 5)
$sb.AppendLine("f " + (FV 5 4 3) + " " + (FV 6 3 3) + " " + (FV 2 2 3) + " " + (FV 1 1 3)) | Out-Null
# 右面 +x: v8,v7,v3,v4 (元: 4 3 7 8)
$sb.AppendLine("f " + (FV 8 4 4) + " " + (FV 7 3 4) + " " + (FV 3 2 4) + " " + (FV 4 1 4)) | Out-Null
# 底面 -y: v5,v7,v3,v1 (元: 1 3 7 5)
$sb.AppendLine("f " + (FV 5 4 5) + " " + (FV 7 3 5) + " " + (FV 3 2 5) + " " + (FV 1 1 5)) | Out-Null
# 上面 +y: v6,v8,v4,v2 (元: 2 4 8 6)
$sb.AppendLine("f " + (FV 6 4 6) + " " + (FV 8 3 6) + " " + (FV 4 2 6) + " " + (FV 2 1 6)) | Out-Null

# === 砲身（LightMetal = シアン）===
# ★裏面カリング対策：頂点の並び順を時計回り（CW）へと反転
$sb.AppendLine("usemtl Mtl_LightMetal") | Out-Null
for ($i = 0; $i -lt $SEGS; $i++) {
    $ni = 7 + $i
    $b0 = $BACK_OFF  + $i
    $b1 = $BACK_OFF  + ($i + 1) % $SEGS
    $f0 = $FRONT_OFF + $i
    $f1 = $FRONT_OFF + ($i + 1) % $SEGS
    $sb.AppendLine("f " + (FV $f0 8 $ni) + " " + (FV $f1 7 $ni) + " " + (FV $b1 6 $ni) + " " + (FV $b0 5 $ni)) | Out-Null
}
# 先端キャップ
for ($i = 0; $i -lt $SEGS; $i++) {
    $f0 = $FRONT_OFF + $i
    $f1 = $FRONT_OFF + ($i + 1) % $SEGS
    $sb.AppendLine("f " + (FV $f0 7 1) + " " + (FV $f1 6 1) + " " + (FV $CAP_CENTER 5 1)) | Out-Null
}

$OUT = "c:\Users\k024g\Desktop\TD3_1\project\Application\resources\model\player\playerCannon.obj"
[System.IO.File]::WriteAllText($OUT, $sb.ToString())
Write-Host "OBJ written: $OUT"

# MTL
$mtl = @"
# playerCannon.mtl

newmtl Mtl_DarkMetal
Ns 60.0
Ka 0.900000 0.900000 0.900000
Kd 0.050000 0.120000 0.360000
Ks 0.400000 0.400000 0.400000
Ke 0.000000 0.000000 0.000000
Ni 1.500000
d 1.000000
illum 3
map_Kd ../../texture/player/playerCannon.png

newmtl Mtl_LightMetal
Ns 80.0
Ka 0.900000 0.900000 0.900000
Kd 0.000000 0.750000 1.000000
Ks 0.500000 0.500000 0.500000
Ke 0.000000 0.000000 0.000000
Ni 1.500000
d 1.000000
illum 3
map_Kd ../../texture/player/playerCannon.png
"@

$OUTM = "c:\Users\k024g\Desktop\TD3_1\project\Application\resources\model\player\playerCannon.mtl"
[System.IO.File]::WriteAllText($OUTM, $mtl)
Write-Host "MTL written: $OUTM"
