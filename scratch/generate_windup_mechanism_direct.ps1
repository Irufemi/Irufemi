# Toy-style Wind-up Key (with Magnet Base) 3D Model Direct Static Writer
# FIXED: Coordinate system corrected. Y=up, rings axis along Z (facing viewer).
# Output: project/Application/resources/model/player/windup_mechanism.obj

$dest_dir = "c:/Users/k024g/Desktop/TD3_1/project/Application/resources/model/player"
if (-not (Test-Path $dest_dir)) {
    New-Item -ItemType Directory -Path $dest_dir -Force | Out-Null
}

$obj_path = "$dest_dir/windup_mechanism.obj"
$mtl_path = "$dest_dir/windup_mechanism.mtl"

$vertices = [System.Collections.Generic.List[string]]::new()
$normals = [System.Collections.Generic.List[string]]::new()
$faces = [System.Collections.Generic.List[string]]::new()

$global:v_count = 0
$global:n_count = 0

function Add-Vertex([double]$x, [double]$y, [double]$z) {
    $vertices.Add(("v {0:F6} {1:F6} {2:F6}" -f $x, $y, $z))
    $global:v_count++
    return $global:v_count
}

function Add-Normal([double]$x, [double]$y, [double]$z) {
    $len = [Math]::Sqrt($x*$x + $y*$y + $z*$z)
    if ($len -gt 0.0001) { $x /= $len; $y /= $len; $z /= $len }
    $normals.Add(("vn {0:F6} {1:F6} {2:F6}" -f $x, $y, $z))
    $global:n_count++
    return $global:n_count
}

# --- Geometry Generators ---

function Generate-Box([double]$w, [double]$h, [double]$d, [double]$cx, [double]$cy, [double]$cz, [string]$mat) {
    # w=X width, h=Y height, d=Z depth
    $faces.Add("usemtl $mat")
    $x1 = $cx - $w/2; $x2 = $cx + $w/2
    $y1 = $cy - $h/2; $y2 = $cy + $h/2
    $z1 = $cz - $d/2; $z2 = $cz + $d/2

    $v1 = Add-Vertex $x1 $y1 $z1; $v2 = Add-Vertex $x2 $y1 $z1
    $v3 = Add-Vertex $x2 $y2 $z1; $v4 = Add-Vertex $x1 $y2 $z1
    $v5 = Add-Vertex $x1 $y1 $z2; $v6 = Add-Vertex $x2 $y1 $z2
    $v7 = Add-Vertex $x2 $y2 $z2; $v8 = Add-Vertex $x1 $y2 $z2

    $nf = Add-Normal 0 0 -1; $nb = Add-Normal 0 0 1
    $nl = Add-Normal -1 0 0;  $nr = Add-Normal 1 0 0
    $nd = Add-Normal 0 -1 0;  $nu = Add-Normal 0 1 0

    $faces.Add("f $v1//$nf $v4//$nf $v3//$nf $v2//$nf")
    $faces.Add("f $v5//$nb $v6//$nb $v7//$nb $v8//$nb")
    $faces.Add("f $v4//$nu $v8//$nu $v7//$nu $v3//$nu")
    $faces.Add("f $v1//$nd $v2//$nd $v6//$nd $v5//$nd")
    $faces.Add("f $v1//$nl $v5//$nl $v8//$nl $v4//$nl")
    $faces.Add("f $v2//$nr $v3//$nr $v7//$nr $v6//$nr")
}

function Generate-Cylinder-Y([double]$r, [double]$y1, [double]$y2, [int]$segs, [double]$cx, [double]$cz, [string]$mat) {
    # Cylinder along Y axis, center at (cx, cz) in X-Z plane
    $faces.Add("usemtl $mat")
    $bot = [System.Collections.Generic.List[int]]::new()
    $top = [System.Collections.Generic.List[int]]::new()
    for ($i = 0; $i -lt $segs; $i++) {
        $th = 2.0 * [Math]::PI * $i / $segs
        $x = $cx + $r * [Math]::Cos($th)
        $z = $cz + $r * [Math]::Sin($th)
        $bot.Add((Add-Vertex $x $y1 $z))
        $top.Add((Add-Vertex $x $y2 $z))
    }
    $vbc = Add-Vertex $cx $y1 $cz; $vtc = Add-Vertex $cx $y2 $cz
    $nb = Add-Normal 0 -1 0; $nt = Add-Normal 0 1 0
    $sn = [System.Collections.Generic.List[int]]::new()
    for ($i = 0; $i -lt $segs; $i++) {
        $th = 2.0 * [Math]::PI * $i / $segs
        $sn.Add((Add-Normal ([Math]::Cos($th)) 0 ([Math]::Sin($th))))
    }
    for ($i = 0; $i -lt $segs; $i++) {
        $j = ($i + 1) % $segs
        $faces.Add("f $($bot[$i])//$($sn[$i]) $($bot[$j])//$($sn[$j]) $($top[$j])//$($sn[$j]) $($top[$i])//$($sn[$i])")
        $faces.Add("f $($bot[$j])//$nb $($bot[$i])//$nb $vbc//$nb")
        $faces.Add("f $($top[$i])//$nt $($top[$j])//$nt $vtc//$nt")
    }
}

function Generate-Ring-Z([double]$r_out, [double]$r_in, [double]$z1, [double]$z2, [int]$segs, [double]$cx, [double]$cy, [string]$mat) {
    # Ring/Donut whose HOLE faces along Z axis (viewer direction). Center at (cx, cy) in X-Y plane.
    $faces.Add("usemtl $mat")

    $ib1 = [System.Collections.Generic.List[int]]::new() # inner z1
    $ib2 = [System.Collections.Generic.List[int]]::new() # inner z2
    $ob1 = [System.Collections.Generic.List[int]]::new() # outer z1
    $ob2 = [System.Collections.Generic.List[int]]::new() # outer z2

    for ($i = 0; $i -lt $segs; $i++) {
        $th = 2.0 * [Math]::PI * $i / $segs
        $cos = [Math]::Cos($th); $sin = [Math]::Sin($th)
        $ib1.Add((Add-Vertex ($cx + $r_in  * $cos) ($cy + $r_in  * $sin) $z1))
        $ib2.Add((Add-Vertex ($cx + $r_in  * $cos) ($cy + $r_in  * $sin) $z2))
        $ob1.Add((Add-Vertex ($cx + $r_out * $cos) ($cy + $r_out * $sin) $z1))
        $ob2.Add((Add-Vertex ($cx + $r_out * $cos) ($cy + $r_out * $sin) $z2))
    }

    $nz1 = Add-Normal 0 0 -1; $nz2 = Add-Normal 0 0 1
    $on = [System.Collections.Generic.List[int]]::new()
    $inn = [System.Collections.Generic.List[int]]::new()
    for ($i = 0; $i -lt $segs; $i++) {
        $th = 2.0 * [Math]::PI * $i / $segs
        $cos = [Math]::Cos($th); $sin = [Math]::Sin($th)
        $on.Add((Add-Normal $cos $sin 0))
        $inn.Add((Add-Normal (-1.0 * $cos) (-1.0 * $sin) 0))
    }

    for ($i = 0; $i -lt $segs; $i++) {
        $j = ($i + 1) % $segs
        # Outer wall
        $faces.Add("f $($ob1[$i])//$($on[$i]) $($ob1[$j])//$($on[$j]) $($ob2[$j])//$($on[$j]) $($ob2[$i])//$($on[$i])")
        # Inner wall
        $faces.Add("f $($ib2[$i])//$($inn[$i]) $($ib2[$j])//$($inn[$j]) $($ib1[$j])//$($inn[$j]) $($ib1[$i])//$($inn[$i])")
        # Front cap (z1)
        $faces.Add("f $($ib1[$i])//$nz1 $($ib1[$j])//$nz1 $($ob1[$j])//$nz1 $($ob1[$i])//$nz1")
        # Back cap (z2)
        $faces.Add("f $($ob2[$i])//$nz2 $($ob2[$j])//$nz2 $($ib2[$j])//$nz2 $($ib2[$i])//$nz2")
    }
}

# ===================================================================
# Assembling the Toy Wind-up Key
# Coordinate system: X=left/right, Y=UP, Z=depth(toward viewer)
# ===================================================================

Write-Host "Constructing 3D Toy Wind-up Key (FIXED both ears)..." -ForegroundColor Cyan

# Layout (Y = height):
#   Magnet base:  Y = -0.04 .. 0.04
#   Collar:       Y = 0.04 .. 0.06
#   Shaft:        Y = 0.06 .. 0.60
#   Neck:         Y = 0.48 .. 0.70  (overlaps shaft top)
#   Bridge:       Y = 0.60 .. 0.96
#   Left ear:     center (X=-0.40, Y=0.78), ring axis=Z
#   Right ear:    center (X= 0.40, Y=0.78), ring axis=Z

# 1. Magnet base (black block)
Generate-Box 0.24 0.08 0.18 0.0 0.0 0.0 "Mtl_WindupMagnet"

# 2. Collar (thin chrome ring at shaft base)
Generate-Cylinder-Y 0.095 0.04 0.065 16 0.0 0.0 "Mtl_WindupChrome"

# 3. Shaft (tall chrome cylinder, Y axis)
Generate-Cylinder-Y 0.08 0.06 0.60 24 0.0 0.0 "Mtl_WindupChrome"

# 4. Neck (wider transition from shaft to key head)
Generate-Box 0.20 0.22 0.08 0.0 0.59 0.0 "Mtl_WindupChrome"

# 5. Bridge plate connecting left and right ears
#    Spans from X=-0.40 to X=0.40 (width=0.80), Y=0.60..0.96 (height=0.36), thin Z
Generate-Box 0.80 0.36 0.08 0.0 0.78 0.0 "Mtl_WindupChrome"

# 6. LEFT ear ring (donut hole faces Z, center at X=-0.40, Y=0.78)
Generate-Ring-Z 0.34 0.19 -0.04 0.04 32 -0.40 0.78 "Mtl_WindupChrome"

# 7. RIGHT ear ring (donut hole faces Z, center at X=0.40, Y=0.78)
Generate-Ring-Z 0.34 0.19 -0.04 0.04 32 0.40 0.78 "Mtl_WindupChrome"


# --- Output ---

$sb = [System.Text.StringBuilder]::new()
$sb.AppendLine("# Toy Wind-up Key Model (Both Ears Fixed)") | Out-Null
$sb.AppendLine("mtllib windup_mechanism.mtl") | Out-Null
$sb.AppendLine("o windup_mechanism") | Out-Null

foreach ($v in $vertices) { $sb.AppendLine($v) | Out-Null }
foreach ($vn in $normals) { $sb.AppendLine($vn) | Out-Null }
foreach ($f in $faces) { $sb.AppendLine($f) | Out-Null }

[System.IO.File]::WriteAllText($obj_path, $sb.ToString())
Write-Host "OBJ exported: $obj_path" -ForegroundColor Green

$mtl_content = @"
# Material Library for Toy Wind-up Key

newmtl Mtl_WindupChrome
Ns 128.0
Ka 0.780000 0.780000 0.800000
Kd 0.780000 0.780000 0.800000
Ks 0.950000 0.950000 0.950000
Ke 0.000000 0.000000 0.000000
Ni 1.500000
d 1.000000
illum 3

newmtl Mtl_WindupMagnet
Ns 24.0
Ka 0.080000 0.080000 0.090000
Kd 0.080000 0.080000 0.090000
Ks 0.150000 0.150000 0.150000
Ke 0.000000 0.000000 0.000000
Ni 1.000000
d 1.000000
illum 2
"@

[System.IO.File]::WriteAllText($mtl_path, $mtl_content)
Write-Host "MTL exported: $mtl_path" -ForegroundColor Green
Write-Host "Done! Both ears are now correctly generated." -ForegroundColor Green
