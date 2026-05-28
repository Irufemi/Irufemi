# SF Super Simple Cannon 3D Model Static Writer
# Output files: 
#   project/Application/resources/model/player/playerCannon.obj
#   project/Application/resources/model/player/playerCannon.mtl

$dest_dir = "c:/Users/k024g/Desktop/TD3_1/project/Application/resources/model/player"
if (-not (Test-Path $dest_dir)) {
    New-Item -ItemType Directory -Path $dest_dir -Force | Out-Null
}

$obj_path = "$dest_dir/playerCannon.obj"
$mtl_path = "$dest_dir/playerCannon.mtl"

# --- OBJ File Text ---
$obj_text = @"
# SF Super Simple Cannon
# Pre-calculated static 3D mesh - Clean, beautiful, exactly 2 parts: 1 base cube and 1 barrel cylinder.
mtllib playerCannon.mtl

# --- Material 1: Mtl_DarkMetal (Breach/Body Base) ---
usemtl Mtl_DarkMetal

# 1. Base Box Vertices (1 to 8)
# cx=0, cy=0.4, cz=0, w=0.35, h=0.45, l=0.8
v -0.175000 0.175000 -0.400000
v 0.175000 0.175000 -0.400000
v 0.175000 0.625000 -0.400000
v -0.175000 0.625000 -0.400000
v -0.175000 0.175000 0.400000
v 0.175000 0.175000 0.400000
v 0.175000 0.625000 0.400000
v -0.175000 0.625000 0.400000

# --- Material 2: Mtl_LightMetal (Single Barrel Cylinder) ---
usemtl Mtl_LightMetal

# 2. Barrel Cylinder Vertices (9 to 26 - 8 segments along Z from 0.0 to -2.0)
# Radius = 0.15, Center X = 0, Center Y = 0.4
# Th = 0, 45, 90, 135, 180, 225, 270, 315
# x = 0.15*cos(th), y = 0.4 + 0.15*sin(th)
v 0.150000 0.400000 -2.000000
v 0.150000 0.400000 0.000000
v 0.106066 0.506066 -2.000000
v 0.106066 0.506066 0.000000
v 0.000000 0.550000 -2.000000
v 0.000000 0.550000 0.000000
v -0.106066 0.506066 -2.000000
v -0.106066 0.506066 0.000000
v -0.150000 0.400000 -2.000000
v -0.150000 0.400000 0.000000
v -0.106066 0.293934 -2.000000
v -0.106066 0.293934 0.000000
v 0.000000 0.250000 -2.000000
v 0.000000 0.250000 0.000000
v 0.106066 0.293934 -2.000000
v 0.106066 0.293934 0.000000

# Center points for Cylinder Caps (25 = Front Center, 26 = Back Center)
v 0.000000 0.400000 -2.000000
v 0.000000 0.400000 0.000000

# --- Texture Coordinates ---
vt 0.000000 0.000000
vt 0.000000 1.000000
vt 1.000000 1.000000
vt 1.000000 0.000000
vt 0.500000 0.500000

# --- Vertex Normals ---
vn 0.000000 0.000000 -1.000000
vn 0.000000 0.000000 1.000000
vn -1.000000 0.000000 0.000000
vn 1.000000 0.000000 0.000000
vn 0.000000 -1.000000 0.000000
vn 0.000000 1.000000 0.000000

# Cylinder Normals (8 directions)
vn 1.000000 0.000000 0.000000
vn 0.707107 0.707107 0.000000
vn 0.000000 1.000000 0.000000
vn -0.707107 0.707107 0.000000
vn -1.000000 0.000000 0.000000
vn -0.707107 -0.707107 0.000000
vn 0.000000 -1.000000 0.000000
vn 0.707107 -0.707107 0.000000

# --- Faces Definitions ---

usemtl Mtl_DarkMetal
# 1. Base Box Faces (6 quads)
f 1/1/1 4/2/1 3/3/1 2/4/1
f 6/1/2 7/2/2 8/3/2 5/4/2
f 5/1/3 8/2/3 4/3/3 1/4/3
f 2/1/4 3/2/4 7/3/4 6/4/4
f 5/1/5 1/2/5 2/3/5 6/4/5
f 4/1/6 8/2/6 7/3/6 3/4/6

usemtl Mtl_LightMetal
# 2. Barrel Cylinder Side Faces (8 quads)
f 9/1/7 11/2/7 12/3/7 10/4/7
f 11/1/8 13/2/8 14/3/8 12/4/8
f 13/1/9 15/2/9 16/3/9 14/4/9
f 15/1/10 17/2/10 18/3/10 16/4/10
f 17/1/11 19/2/11 20/3/11 18/4/11
f 19/1/12 21/2/12 22/3/12 20/4/12
f 21/1/13 23/2/13 24/3/13 22/4/13
f 23/1/14 9/2/14 10/3/14 24/4/14

# Cylinder Front Cap (Z = -2.0) (8 triangles using vertex 25)
f 25/5/1 11/1/1 9/4/1
f 25/5/1 13/1/1 11/4/1
f 25/5/1 15/1/1 13/4/1
f 25/5/1 17/1/1 15/4/1
f 25/5/1 19/1/1 17/4/1
f 25/5/1 21/1/1 19/4/1
f 25/5/1 23/1/1 21/4/1
f 25/5/1 9/1/1 23/4/1

# Cylinder Back Cap (Z = 0.0) (8 triangles using vertex 26)
f 26/5/2 10/1/2 12/4/2
f 26/5/2 12/1/2 14/4/2
f 26/5/2 14/1/2 16/4/2
f 26/5/2 16/1/2 18/4/2
f 26/5/2 18/1/2 20/4/2
f 26/5/2 20/1/2 22/4/2
f 26/5/2 22/1/2 24/4/2
f 26/5/2 24/1/2 10/4/2
"@

[System.IO.File]::WriteAllText($obj_path, $obj_text)

# --- MTL File Text ---
$mtl_text = @"
# Material library for SF Super Simple Cannon

newmtl Mtl_DarkMetal
Kd 0.15 0.15 0.16
Ks 0.50 0.50 0.52
Ns 32
illum 2

newmtl Mtl_LightMetal
Kd 0.35 0.36 0.38
Ks 0.70 0.70 0.75
Ns 64
illum 2
"@

[System.IO.File]::WriteAllText($mtl_path, $mtl_text)

Write-Host "Success: Super Simple Cannon OBJ and MTL written perfectly!"
