"""
シンプルな大砲モデル生成スクリプト
- 本体（箱）: 幅2.0 x 高さ2.0 x 奥行き3.0
- 砲身（円柱）: 半径0.5, 長さ5.0  前方に延びる
合計サイズが機関銃モデルと近い 6ユニットスケール
"""
import math
import os

OUT_OBJ = r"c:\Users\k024g\Desktop\TD3_1\project\Application\resources\model\player\playerCannon.obj"
OUT_MTL = r"c:\Users\k024g\Desktop\TD3_1\project\Application\resources\model\player\playerCannon.mtl"

SEGS = 16  # 円柱の分割数

# ---- ボックス頂点 ----
# 本体: X[-1,1], Y[-1,1], Z[-1.5,1.5]  (Z正が後ろ、Z負が前)
BW, BH, BD = 1.0, 1.0, 1.5
box_verts = [
    (-BW, -BH, -BD), (-BW,  BH, -BD),
    ( BW, -BH, -BD), ( BW,  BH, -BD),
    (-BW, -BH,  BD), (-BW,  BH,  BD),
    ( BW, -BH,  BD), ( BW,  BH,  BD),
]

# ---- 円柱頂点（砲身） ----
# 中心軸はZ、前方（Z負）へ延びる
# 砲身は本体前面(Z=-BD)から、さらにZBARREL分前に出る
BARREL_R = 0.45
BARREL_LEN = 5.0
Z_START = -BD  # 本体前面
Z_END   = Z_START - BARREL_LEN  # 砲身の先端

cyl_verts_front = []  # Z_END  (先端)
cyl_verts_back  = []  # Z_START(根元)

for i in range(SEGS):
    angle = 2 * math.pi * i / SEGS
    x = BARREL_R * math.cos(angle)
    y = BARREL_R * math.sin(angle)
    cyl_verts_front.append((x, y, Z_END))
    cyl_verts_back.append( (x, y, Z_START))

verts = list(box_verts) + cyl_verts_back + cyl_verts_front
# index base: box 1-8, back 9-24, front 25-40  (1-indexed)
BOX_OFF   = 1
BACK_OFF  = BOX_OFF + len(box_verts)      # 9
FRONT_OFF = BACK_OFF + SEGS               # 25

# ---- 法線 ----
norms = [
    (0,0,-1), (0,0,1),   # Z面
    (-1,0,0), (1,0,0),   # X面
    (0,-1,0), (0,1,0),   # Y面
]
# 円柱側面法線 (各セグメント)
for i in range(SEGS):
    angle = 2 * math.pi * (i + 0.5) / SEGS
    norms.append((math.cos(angle), math.sin(angle), 0))

# ---- UV（簡易） ----
uvs = [(0,0),(1,0),(1,1),(0,1)]

# ---- 面の構築 ----
faces_dark  = []  # 本体
faces_light = []  # 砲身

# Box faces (各面を時計回りで外向き法線)
def bi(i): return BOX_OFF + i  # 1-indexed
# 前面(Z_END側) v3,v4,v2,v1  法線(0,0,-1)=1
faces_dark.append(((bi(2),1,1),(bi(3),2,1),(bi(1),3,1),(bi(0),4,1)))
# 後面(Z_START側) v5,v6,v8,v7  法線(0,0,1)=2
faces_dark.append(((bi(4),1,2),(bi(5),2,2),(bi(7),3,2),(bi(6),4,2)))
# 左面 v1,v2,v6,v5  法線(-1,0,0)=3
faces_dark.append(((bi(0),1,3),(bi(1),2,3),(bi(5),3,3),(bi(4),4,3)))
# 右面 v4,v3,v7,v8  法線(1,0,0)=4
faces_dark.append(((bi(3),1,4),(bi(2),2,4),(bi(6),3,4),(bi(7),4,4)))
# 底面 v1,v3,v7,v5  法線(0,-1,0)=5
faces_dark.append(((bi(0),1,5),(bi(2),2,5),(bi(6),3,5),(bi(4),4,5)))
# 上面 v2,v4,v8,v6  法線(0,1,0)=6
faces_dark.append(((bi(1),1,6),(bi(3),2,6),(bi(7),3,6),(bi(5),4,6)))

# 砲身側面
for i in range(SEGS):
    ni = 7 + i  # norm index (1-based)
    b0 = BACK_OFF  + i
    b1 = BACK_OFF  + (i+1) % SEGS
    f0 = FRONT_OFF + i
    f1 = FRONT_OFF + (i+1) % SEGS
    faces_light.append(((b0,1,ni),(b1,2,ni),(f1,3,ni),(f0,4,ni)))

# 先端キャップ（ファン）
cap_center_idx = len(verts) + 1
cap_norm_idx = 1  # (0,0,-1) = 前向き
verts.append((0, 0, Z_END))
cap_face = []
for i in range(SEGS):
    f0 = FRONT_OFF + i
    f1 = FRONT_OFF + (i+1) % SEGS
    faces_light.append(((cap_center_idx,1,cap_norm_idx),(f1,2,cap_norm_idx),(f0,3,cap_norm_idx)))

# 根元キャップ（砲身が本体につながる部分は省略=見えない）

# ---- OBJ 書き出し ----
os.makedirs(os.path.dirname(OUT_OBJ), exist_ok=True)

with open(OUT_OBJ, 'w') as f:
    f.write("# Simple Cannon - playerCannon.obj\n")
    f.write(f"mtllib playerCannon.mtl\n")
    f.write("o playerCannon\n")
    for v in verts:
        f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
    for n in norms:
        f.write(f"vn {n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n")
    for u in uvs:
        f.write(f"vt {u[0]:.4f} {u[1]:.4f}\n")
    f.write("s 0\n")
    f.write("usemtl Mtl_DarkMetal\n")
    for face in faces_dark:
        parts = " ".join(f"{vi}/{ui}/{ni}" for vi,ui,ni in face)
        f.write(f"f {parts}\n")
    f.write("usemtl Mtl_LightMetal\n")
    for face in faces_light:
        parts = " ".join(f"{vi}/{ui}/{ni}" for vi,ui,ni in face)
        f.write(f"f {parts}\n")

# ---- MTL 書き出し ----
with open(OUT_MTL, 'w') as f:
    f.write("# playerCannon.mtl\n\n")
    f.write("newmtl Mtl_DarkMetal\n")
    f.write("Kd 0.15 0.15 0.16\n")
    f.write("Ka 0.05 0.05 0.05\n")
    f.write("Ks 0.4 0.4 0.4\n")
    f.write("Ns 60.0\n\n")
    f.write("newmtl Mtl_LightMetal\n")
    f.write("Kd 0.35 0.36 0.38\n")
    f.write("Ka 0.05 0.05 0.05\n")
    f.write("Ks 0.5 0.5 0.5\n")
    f.write("Ns 80.0\n")

print("Done! playerCannon.obj / .mtl generated.")
print(f"  Body   : {BW*2:.1f}w x {BH*2:.1f}h x {BD*2:.1f}d")
print(f"  Barrel : radius={BARREL_R}, length={BARREL_LEN}")
print(f"  Total Z range: {Z_END:.1f} ~ {BD:.1f}")
