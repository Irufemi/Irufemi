# SF Heavy Machine Gun 3D Model Generator in PowerShell
# Output files: 
#   project/Application/resources/model/player/playerMachineGun.obj
#   project/Application/resources/model/player/playerMachineGun.mtl

$vertices = [System.Collections.Generic.List[string]]::new()
$normals = [System.Collections.Generic.List[string]]::new()
$texcoords = [System.Collections.Generic.List[string]]::new()
$faces = @{
    "Mtl_DarkMetal"  = [System.Collections.Generic.List[string]]::new()
    "Mtl_LightMetal" = [System.Collections.Generic.List[string]]::new()
    "Mtl_Accent"     = [System.Collections.Generic.List[string]]::new()
}
$global:current_material = "Mtl_DarkMetal"

function Set-Material([string]$matName) {
    $global:current_material = $matName
}

function Add-Vertex([double]$x, [double]$y, [double]$z) {
    $vertices.Add(("{0:F6} {1:F6} {2:F6}" -f $x, $y, $z))
    return $vertices.Count
}

function Add-Normal([double]$x, [double]$y, [double]$z) {
    $len = [Math]::Sqrt($x*$x + $y*$y + $z*$z)
    if ($len -gt 0.0001) {
        $x /= $len
        $y /= $len
        $z /= $len
    }
    $normals.Add(("{0:F6} {1:F6} {2:F6}" -f $x, $y, $z))
    return $normals.Count
}

function Add-TexCoord([double]$u, [double]$v) {
    $texcoords.Add(("{0:F6} {1:F6}" -f $u, $v))
    return $texcoords.Count
}

function Rotate-And-Translate {
    param(
        [double]$x, [double]$y, [double]$z,
        [double]$cx, [double]$cy, [double]$cz,
        [double]$rx, [double]$ry, [double]$rz
    )
    # Rotate X
    if ($rx -ne 0.0) {
        $c = [Math]::Cos($rx)
        $s = [Math]::Sin($rx)
        $ny = $y * $c - $z * $s
        $nz = $y * $s + $z * $c
        $y = $ny
        $z = $nz
    }
    # Rotate Y
    if ($ry -ne 0.0) {
        $c = [Math]::Cos($ry)
        $s = [Math]::Sin($ry)
        $nx = $x * $c + $z * $s
        $nz = -$x * $s + $z * $c  # Fully fixed the $vz typo to $z
        $x = $nx
        $z = $nz
    }
    # Rotate Z
    if ($rz -ne 0.0) {
        $c = [Math]::Cos($rz)
        $s = [Math]::Sin($rz)
        $nx = $x * $c - $y * $s
        $ny = $x * $s + $y * $c
        $x = $nx
        $y = $ny
    }
    return [double[]]@($x + $cx, $y + $cy, $z + $cz)
}

function Add-Box {
    param(
        [double]$cx, [double]$cy, [double]$cz,
        [double]$w, [double]$h, [double]$l,
        [double]$rx=0.0, [double]$ry=0.0, [double]$rz=0.0
    )

    $dx = $w / 2.0
    $dy = $h / 2.0
    $dz = $l / 2.0

    # Define 8 corners locally
    $p0 = Rotate-And-Translate -x (-$dx) -y (-$dy) -z (-$dz) -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
    $p1 = Rotate-And-Translate -x $dx -y (-$dy) -z (-$dz) -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
    $p2 = Rotate-And-Translate -x $dx -y $dy -z (-$dz) -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
    $p3 = Rotate-And-Translate -x (-$dx) -y $dy -z (-$dz) -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
    $p4 = Rotate-And-Translate -x (-$dx) -y (-$dy) -z $dz -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
    $p5 = Rotate-And-Translate -x $dx -y (-$dy) -z $dz -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
    $p6 = Rotate-And-Translate -x $dx -y $dy -z $dz -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
    $p7 = Rotate-And-Translate -x (-$dx) -y $dy -z $dz -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz

    $v_indices = @(
        (Add-Vertex $p0[0] $p0[1] $p0[2]),
        (Add-Vertex $p1[0] $p1[1] $p1[2]),
        (Add-Vertex $p2[0] $p2[1] $p2[2]),
        (Add-Vertex $p3[0] $p3[1] $p3[2]),
        (Add-Vertex $p4[0] $p4[1] $p4[2]),
        (Add-Vertex $p5[0] $p5[1] $p5[2]),
        (Add-Vertex $p6[0] $p6[1] $p6[2]),
        (Add-Vertex $p7[0] $p7[1] $p7[2])
    )

    $faces_data = @(
        # Front (-Z)
        @(@(0, 3, 2, 1), @(0, 0, -1)),
        # Back (+Z)
        @(@(5, 6, 7, 4), @(0, 0, 1)),
        # Left (-X)
        @(@(4, 7, 3, 0), @(-1, 0, 0)),
        # Right (+X)
        @(@(1, 2, 6, 5), @(1, 0, 0)),
        # Bottom (-Y)
        @(@(4, 0, 1, 5), @(0, -1, 0)),
        # Top (+Y)
        @(@(3, 7, 6, 2), @(0, 1, 0))
    )

    foreach ($face in $faces_data) {
        $indices = $face[0]
        $norm = $face[1]
        
        $n_idx = Add-Normal $norm[0] $norm[1] $norm[2]
        
        $t1 = Add-TexCoord 0.0 0.0
        $t2 = Add-TexCoord 0.0 1.0
        $t3 = Add-TexCoord 1.0 1.0
        $t4 = Add-TexCoord 1.0 0.0
        
        $t_indices = @($t1, $t2, $t3, $t4)

        $gv0 = $v_indices[$indices[0]]
        $gv1 = $v_indices[$indices[1]]
        $gv2 = $v_indices[$indices[2]]
        $gv3 = $v_indices[$indices[3]]

        $face_str = "{0}/{1}/{2} {3}/{4}/{2} {5}/{6}/{2} {7}/{8}/{2}" -f $gv0, $t_indices[0], $n_idx, $gv1, $t_indices[1], $gv2, $t_indices[2], $gv3, $t_indices[3]
        $faces[$global:current_material].Add($face_str)
    }
}

function Add-Cylinder {
    param(
        [double]$cx, [double]$cy, [double]$cz,
        [double]$r, [double]$length,
        [int]$segments=12,
        [double]$rx=0.0, [double]$ry=0.0, [double]$rz=0.0
    )

    $z_start = -$length / 2.0
    $z_end = $length / 2.0

    $v_indices = [System.Collections.Generic.List[int]]::new()
    
    # Generate side vertices and rotate them directly
    for ($i = 0; $i -lt $segments; $i++) {
        $theta = 2.0 * [Math]::PI * $i / $segments
        $x = $r * [Math]::Cos($theta)
        $y = $r * [Math]::Sin($theta)
        
        # Start cap point (-Z)
        $pStart = Rotate-And-Translate -x $x -y $y -z $z_start -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
        $v_indices.Add((Add-Vertex $pStart[0] $pStart[1] $pStart[2]))
        
        # End cap point (+Z)
        $pEnd = Rotate-And-Translate -x $x -y $y -z $z_end -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
        $v_indices.Add((Add-Vertex $pEnd[0] $pEnd[1] $pEnd[2]))
    }

    # Side faces
    for ($i = 0; $i -lt $segments; $i++) {
        $next_i = ($i + 1) % $segments

        $v0 = $v_indices[$i * 2]
        $v1 = $v_indices[$i * 2 + 1]
        $v2 = $v_indices[$next_i * 2 + 1]
        $v3 = $v_indices[$next_i * 2]

        $mid_theta = 2.0 * [Math]::PI * ($i + 0.5) / $segments
        $nx = [Math]::Cos($mid_theta)
        $ny = [Math]::Sin($mid_theta)
        $nz = 0.0

        # Rotate normal
        if ($rx -ne 0.0) {
            $c = [Math]::Cos($rx)
            $s = [Math]::Sin($rx)
            $n_ny = $ny * $c - $nz * $s
            $n_nz = $ny * $s + $nz * $c
            $ny = $n_ny
            $nz = $n_nz
        }
        if ($ry -ne 0.0) {
            $c = [Math]::Cos($ry)
            $s = [Math]::Sin($ry)
            $n_nx = $nx * $c + $nz * $s
            $n_nz = -$nx * $s + $z * $c # Fixed $vz typo
            $nx = $n_nx
            $nz = $n_nz
        }
        if ($rz -ne 0.0) {
            $c = [Math]::Cos($rz)
            $s = [Math]::Sin($rz)
            $n_nx = $nx * $c - $ny * $s
            $n_ny = $nx * $s + $ny * $c
            $nx = $n_nx
            $ny = $n_ny
        }

        $n_idx = Add-Normal $nx $ny $nz

        $t0 = Add-TexCoord ($i / $segments) 0.0
        $t1 = Add-TexCoord ($i / $segments) 1.0
        $t2 = Add-TexCoord (($i + 1) / $segments) 1.0
        $t3 = Add-TexCoord (($i + 1) / $segments) 0.0

        $face_str = "{0}/{1}/{2} {3}/{4}/{2} {5}/{6}/{2} {7}/{8}/{2}" -f $v0, $t0, $n_idx, $v1, $t1, $v2, $t2, $v3, $t3
        $faces[$global:current_material].Add($face_str)
    }

    # End Cap (+Z)
    $end_cap_v = @()
    for ($i = 0; $i -lt $segments; $i++) {
        $end_cap_v += $v_indices[$i * 2 + 1]
    }

    # Center vertex for End Cap
    $pEndCenter = Rotate-And-Translate -x 0.0 -y 0.0 -z $z_end -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
    $ec_v_idx = Add-Vertex $pEndCenter[0] $pEndCenter[1] $pEndCenter[2]

    # End Cap Normal
    $enx = 0.0; $eny = 0.0; $enz = 1.0
    if ($rx -ne 0.0) {
        $c = [Math]::Cos($rx); $s = [Math]::Sin($rx)
        $n_eny = $eny * $c - $enz * $s; $n_enz = $eny * $s + $enz * $c
        $eny = $n_eny; $enz = $n_enz
    }
    if ($ry -ne 0.0) {
        $c = [Math]::Cos($ry); $s = [Math]::Sin($ry)
        $n_enx = $enx * $c + $enz * $s; $n_enz = -$enx * $s + $enz * $c
        $enx = $n_enx; $enz = $n_enz
    }
    if ($rz -ne 0.0) {
        $c = [Math]::Cos($rz); $s = [Math]::Sin($rz)
        $n_enx = $enx * $c - $eny * $s; $n_eny = $enx * $s + $eny * $c
        $enx = $n_enx; $eny = $n_eny
    }
    $e_n_idx = Add-Normal $enx $eny $enz

    for ($i = 0; $i -lt $segments; $i++) {
        $next_i = ($i + 1) % $segments
        $t0 = Add-TexCoord 0.5 0.5
        
        $theta = 2.0 * [Math]::PI * $i / $segments
        $t1 = Add-TexCoord (0.5 + 0.5 * [Math]::Cos($theta)) (0.5 + 0.5 * [Math]::Sin($theta))
        
        $next_theta = 2.0 * [Math]::PI * $next_i / $segments
        $t2 = Add-TexCoord (0.5 + 0.5 * [Math]::Cos($next_theta)) (0.5 + 0.5 * [Math]::Sin($next_theta))

        $face_str = "{0}/{1}/{2} {3}/{4}/{2} {5}/{6}/{2}" -f $ec_v_idx, $t0, $e_n_idx, $end_cap_v[$i], $t1, $end_cap_v[$next_i], $t2
        $faces[$global:current_material].Add($face_str)
    }

    # Start Cap (-Z)
    $start_cap_v = @()
    for ($i = 0; $i -lt $segments; $i++) {
        $start_cap_v += $v_indices[$i * 2]
    }

    # Center vertex for Start Cap
    $pStartCenter = Rotate-And-Translate -x 0.0 -y 0.0 -z $z_start -cx $cx -cy $cy -cz $cz -rx $rx -ry $ry -rz $rz
    $sc_v_idx = Add-Vertex $pStartCenter[0] $pStartCenter[1] $pStartCenter[2]

    # Start Cap Normal
    $snx = 0.0; $sny = 0.0; $snz = -1.0
    if ($rx -ne 0.0) {
        $c = [Math]::Cos($rx); $s = [Math]::Sin($rx)
        $n_sny = $sny * $c - $snz * $s; $n_snz = $sny * $s + $snz * $c
        $sny = $n_sny; $snz = $n_snz
    }
    if ($ry -ne 0.0) {
        $c = [Math]::Cos($ry); $s = [Math]::Sin($ry)
        $n_snx = $snx * $c + $snz * $s; $n_snz = -$snx * $s + $snz * $c
        $snx = $n_snx; $snz = $n_snz
    }
    if ($rz -ne 0.0) {
        $c = [Math]::Cos($rz); $s = [Math]::Sin($rz)
        $n_snx = $snx * $c - $sny * $s; $n_sny = $snx * $s + $sny * $c
        $snx = $n_snx; $sny = $n_sny
    }
    $s_n_idx = Add-Normal $snx $sny $snz

    for ($i = 0; $i -lt $segments; $i++) {
        $next_i = ($i + 1) % $segments
        $t0 = Add-TexCoord 0.5 0.5
        
        $next_theta = 2.0 * [Math]::PI * $next_i / $segments
        $t1 = Add-TexCoord (0.5 + 0.5 * [Math]::Cos($next_theta)) (0.5 + 0.5 * [Math]::Sin($next_theta))
        
        $theta = 2.0 * [Math]::PI * $i / $segments
        $t2 = Add-TexCoord (0.5 + 0.5 * [Math]::Cos($theta)) (0.5 + 0.5 * [Math]::Sin($theta))

        $face_str = "{0}/{1}/{2} {3}/{4}/{2} {5}/{6}/{2}" -f $sc_v_idx, $t0, $s_n_idx, $start_cap_v[$next_i], $t1, $start_cap_v[$i], $t2
        $faces[$global:current_material].Add($face_str)
    }
}

# --- 1. Base / Mount ---
Set-Material "Mtl_DarkMetal"
Add-Box -cx 0.0 -cy 0.0 -cz 0.0 -w 0.4 -h 0.3 -l 0.6
Add-Box -cx 0.0 -cy 0.2 -cz 0.0 -w 0.2 -h 0.4 -l 0.4

# --- 2. Receiver (Gun Body) ---
Set-Material "Mtl_DarkMetal"
Add-Box -cx 0.0 -cy 0.6 -cz 0.2 -w 0.5 -h 0.5 -l 1.4
Add-Box -cx 0.0 -cy 0.6 -cz 1.0 -w 0.55 -h 0.6 -l 0.6

# Gold/Accent Cover (Top of Receiver)
Set-Material "Mtl_Accent"
Add-Box -cx 0.0 -cy 0.9 -cz 0.2 -w 0.35 -h 0.15 -l 1.2
Add-Box -cx 0.0 -cy 0.95 -cz 0.7 -w 0.25 -h 0.1 -l 0.4

# --- 3. Gatling Rotary Shaft & Barrels ---
Set-Material "Mtl_LightMetal"
Add-Cylinder -cx 0.0 -cy 0.6 -cz (-0.9) -r 0.20 -length 0.8 -segments 12

# Inner cooling cylinder
Set-Material "Mtl_DarkMetal"
Add-Cylinder -cx 0.0 -cy 0.6 -cz (-1.8) -r 0.12 -length 1.2 -segments 12

# Rotary disks that hold the barrels
Set-Material "Mtl_LightMetal"
Add-Cylinder -cx 0.0 -cy 0.6 -cz (-1.3) -r 0.22 -length 0.08 -segments 12
Add-Cylinder -cx 0.0 -cy 0.6 -cz (-2.5) -r 0.22 -length 0.08 -segments 12

# 4 Barrels arranged circularly
Set-Material "Mtl_DarkMetal"
$barrel_radius = 0.045
$barrel_length = 2.0
$dist_from_center = 0.12
for ($i = 0; $i -lt 4; $i++) {
    $angle = $i * ([Math]::PI / 2.0)
    $bx = $dist_from_center * [Math]::Cos($angle)
    $by = 0.6 + $dist_from_center * [Math]::Sin($angle)
    Add-Cylinder -cx $bx -cy $by -cz (-2.1) -r $barrel_radius -length $barrel_length -segments 8
    
    # Bright tips (Muzzle tips)
    Set-Material "Mtl_Accent"
    Add-Cylinder -cx $bx -cy $by -cz (-3.1) -r ($barrel_radius + 0.005) -length 0.1 -segments 8
    Set-Material "Mtl_DarkMetal"
}

# --- 4. Drum Magazine ---
Set-Material "Mtl_DarkMetal"
Add-Cylinder -cx (-0.45) -cy 0.4 -cz 0.3 -r 0.45 -length 0.35 -segments 16 -ry ([Math]::PI / 2.0)
Set-Material "Mtl_Accent"
Add-Cylinder -cx (-0.63) -cy 0.4 -cz 0.3 -r 0.3 -length 0.05 -segments 16 -ry ([Math]::PI / 2.0)

# --- 5. Sensors / Laser Scope (Top rear) ---
Set-Material "Mtl_LightMetal"
Add-Box -cx 0.0 -cy 1.05 -cz 0.8 -w 0.1 -h 0.15 -l 0.15
Set-Material "Mtl_DarkMetal"
Add-Cylinder -cx 0.0 -cy 1.15 -cz 0.8 -r 0.09 -length 0.4 -segments 12
Set-Material "Mtl_Accent"
Add-Cylinder -cx 0.0 -cy 1.15 -cz 0.6 -r 0.095 -length 0.05 -segments 12
Add-Cylinder -cx 0.0 -cy 1.15 -cz 1.0 -r 0.095 -length 0.05 -segments 12

# Write OBJ file
$obj_content = New-Object System.Text.StringBuilder
[void]$obj_content.AppendLine("# SF Heavy Machine Gun 3D Model")
[void]$obj_content.AppendLine("mtllib playerMachineGun.mtl`n")

foreach ($v in $vertices) {
    [void]$obj_content.AppendLine("v $v")
}
[void]$obj_content.AppendLine("")
foreach ($vt in $texcoords) {
    [void]$obj_content.AppendLine("vt $vt")
}
[void]$obj_content.AppendLine("")
foreach ($vn in $normals) {
    [void]$obj_content.AppendLine("vn $vn")
}
[void]$obj_content.AppendLine("")

foreach ($key in $faces.Keys) {
    [void]$obj_content.AppendLine("usemtl $key")
    foreach ($face in $faces[$key]) {
        [void]$obj_content.AppendLine("f $face")
    }
    [void]$obj_content.AppendLine("")
}

$dest_dir = "c:/Users/k024g/Desktop/TD3_1/project/Application/resources/model/player"
if (-not (Test-Path $dest_dir)) {
    New-Item -ItemType Directory -Path $dest_dir -Force | Out-Null
}

[System.IO.File]::WriteAllText("$dest_dir/playerMachineGun.obj", $obj_content.ToString())

# Write MTL file
$mtl_content = @"
# Material library for SF Heavy Machine Gun

newmtl Mtl_DarkMetal
Kd 0.18 0.18 0.20
Ks 0.50 0.50 0.52
Ns 32
illum 2

newmtl Mtl_LightMetal
Kd 0.50 0.53 0.55
Ks 0.70 0.70 0.75
Ns 64
illum 2

newmtl Mtl_Accent
Kd 0.90 0.55 0.05
Ks 0.80 0.80 0.80
Ns 45
illum 2
"@

[System.IO.File]::WriteAllText("$dest_dir/playerMachineGun.mtl", $mtl_content)

Write-Host "Success: Generated playerMachineGun.obj and playerMachineGun.mtl"
