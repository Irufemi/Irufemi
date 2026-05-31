import math

class OBJBuilder:
    def __init__(self):
        self.vertices = []
        self.normals = []
        self.texcoords = []
        self.faces = {}  # mat_name -> list of faces
        self.current_material = "Mtl_DarkMetal"

    def set_material(self, mat_name):
        self.current_material = mat_name
        if mat_name not in self.faces:
            self.faces[mat_name] = []

    def add_vertex(self, x, y, z):
        self.vertices.append((x, y, z))
        return len(self.vertices)

    def add_normal(self, x, y, z):
        length = math.sqrt(x*x + y*y + z*z)
        if length > 0.0001:
            x, y, z = x/length, y/length, z/length
        self.normals.append((x, y, z))
        return len(self.normals)

    def add_texcoord(self, u, v):
        self.texcoords.append((u, v))
        return len(self.texcoords)

    def add_box(self, cx, cy, cz, w, h, l, rx=0.0, ry=0.0, rz=0.0):
        dx, dy, dz = w/2.0, h/2.0, l/2.0
        local_vertices = [
            (-dx, -dy, -dz), (dx, -dy, -dz), (dx, dy, -dz), (-dx, dy, -dz),
            (-dx, -dy, dz), (dx, -dy, dz), (dx, dy, dz), (-dx, dy, dz)
        ]

        transformed_vertices = []
        for vx, vy, vz in local_vertices:
            if rx != 0.0:
                c, s = math.cos(rx), math.sin(rx)
                vy, vz = vy*c - vz*s, vy*s + vz*c
            if ry != 0.0:
                c, s = math.cos(ry), math.sin(ry)
                vx, vz = vx*c + vz*s, -vx*s + vz*c
            if rz != 0.0:
                c, s = math.cos(rz), math.sin(rz)
                vx, vy = vx*c - vy*s, vx*s + vy*c
            
            transformed_vertices.append((vx + cx, vy + cy, vz + cz))

        v_indices = [self.add_vertex(x, y, z) for x, y, z in transformed_vertices]

        faces_data = [
            ([0, 3, 2, 1], (0, 0, -1)),
            ([5, 6, 7, 4], (0, 0, 1)),
            ([4, 7, 3, 0], (-1, 0, 0)),
            ([1, 2, 6, 5], (1, 0, 0)),
            ([4, 0, 1, 5], (0, -1, 0)),
            ([3, 7, 6, 2], (0, 1, 0))
        ]

        for indices, normal in faces_data:
            n_idx = self.add_normal(normal[0], normal[1], normal[2])
            t1 = self.add_texcoord(0.0, 0.0)
            t2 = self.add_texcoord(0.0, 1.0)
            t3 = self.add_texcoord(1.0, 1.0)
            t4 = self.add_texcoord(1.0, 0.0)
            t_indices = [t1, t2, t3, t4]

            g_v = [v_indices[i] for i in indices]
            self.faces[self.current_material].append(
                ((g_v[0], t_indices[0], n_idx),
                 (g_v[1], t_indices[1], n_idx),
                 (g_v[2], t_indices[2], n_idx),
                 (g_v[3], t_indices[3], n_idx))
            )

    def add_cylinder(self, cx, cy, cz, r, length, segments=12, rx=0.0, ry=0.0, rz=0.0):
        z_start = -length / 2.0
        z_end = length / 2.0

        local_vertices = []
        for i in range(segments):
            theta = 2.0 * math.pi * i / segments
            x = r * math.cos(theta)
            y = r * math.sin(theta)
            local_vertices.append((x, y, z_start))
            local_vertices.append((x, y, z_end))

        transformed_vertices = []
        for vx, vy, vz in local_vertices:
            if rx != 0.0:
                c, s = math.cos(rx), math.sin(rx)
                vy, vz = vy*c - vz*s, vy*s + vz*c
            if ry != 0.0:
                c, s = math.cos(ry), math.sin(ry)
                vx, vz = vx*c + vz*s, -vx*s + vz*c
            if rz != 0.0:
                c, s = math.cos(rz), math.sin(rz)
                vx, vy = vx*c - vy*s, vx*s + vy*c
            
            transformed_vertices.append((vx + cx, vy + cy, vz + cz))

        v_indices = [self.add_vertex(x, y, z) for x, y, z in transformed_vertices]

        # Side faces
        for i in range(segments):
            next_i = (i + 1) % segments
            
            v0_idx = v_indices[i * 2]
            v1_idx = v_indices[i * 2 + 1]
            v2_idx = v_indices[next_i * 2 + 1]
            v3_idx = v_indices[next_i * 2]

            mid_theta = 2.0 * math.pi * (i + 0.5) / segments
            nx, ny, nz = math.cos(mid_theta), math.sin(mid_theta), 0.0
            
            if rx != 0.0:
                c, s = math.cos(rx), math.sin(rx)
                ny, nz = ny*c - nz*s, ny*s + nz*c
            if ry != 0.0:
                c, s = math.cos(ry), math.sin(ry)
                nx, nz = nx*c + nz*s, -nx*s + nz*c
            if rz != 0.0:
                c, s = math.cos(rz), math.sin(rz)
                nx, ny = nx*c - ny*s, nx*s + ny*c

            n_idx = self.add_normal(nx, ny, nz)
            
            t0 = self.add_texcoord(float(i)/segments, 0.0)
            t1 = self.add_texcoord(float(i)/segments, 1.0)
            t2 = self.add_texcoord(float(i+1)/segments, 1.0)
            t3 = self.add_texcoord(float(i+1)/segments, 0.0)

            self.faces[self.current_material].append(
                ((v0_idx, t0, n_idx),
                 (v1_idx, t1, n_idx),
                 (v2_idx, t2, n_idx),
                 (v3_idx, t3, n_idx))
            )

        # Cap faces
        # End Cap (+Z)
        end_cap_v = []
        for i in range(segments):
            end_cap_v.append(v_indices[i * 2 + 1])
        end_center = (0.0, 0.0, z_end)
        ecx, ecy, ecz = end_center
        if rx != 0.0:
            c, s = math.cos(rx), math.sin(rx)
            ecy, ecz = ecy*c - ecz*s, ecy*s + ecz*c
        if ry != 0.0:
            c, s = math.cos(ry), math.sin(ry)
            ecx, ecz = ecx*c + ecz*s, -ecx*s + ecz*c
        if rz != 0.0:
            c, s = math.cos(rz), math.sin(rz)
            ecx, ecy = ecx*c - ecy*s, ecx*s + ecy*c
        ec_v_idx = self.add_vertex(ecx + cx, ecy + cy, ecz + cz)
        
        enx, eny, enz = 0.0, 0.0, 1.0
        if rx != 0.0:
            c, s = math.cos(rx), math.sin(rx)
            eny, enz = eny*c - enz*s, eny*s + enz*c
        if ry != 0.0:
            c, s = math.cos(ry), math.sin(ry)
            enx, enz = enx*c + enz*s, -enx*s + enz*c
        if rz != 0.0:
            c, s = math.cos(rz), math.sin(rz)
            enx, eny = enx*c - eny*s, enx*s + eny*c
        e_n_idx = self.add_normal(enx, eny, enz)

        for i in range(segments):
            next_i = (i + 1) % segments
            t0 = self.add_texcoord(0.5, 0.5)
            t1 = self.add_texcoord(0.5 + 0.5*math.cos(2.0*math.pi*i/segments), 0.5 + 0.5*math.sin(2.0*math.pi*i/segments))
            t2 = self.add_texcoord(0.5 + 0.5*math.cos(2.0*math.pi*next_i/segments), 0.5 + 0.5*math.sin(2.0*math.pi*next_i/segments))
            
            self.faces[self.current_material].append(
                ((ec_v_idx, t0, e_n_idx),
                 (end_cap_v[i], t1, e_n_idx),
                 (end_cap_v[next_i], t2, e_n_idx))
            )

        # Start Cap (-Z)
        start_cap_v = []
        for i in range(segments):
            start_cap_v.append(v_indices[i * 2])
        start_center = (0.0, 0.0, z_start)
        scx, scy, scz = start_center
        if rx != 0.0:
            c, s = math.cos(rx), math.sin(rx)
            scy, scz = scy*c - scz*s, scy*s + scz*c
        if ry != 0.0:
            c, s = math.cos(ry), math.sin(ry)
            scx, scz = scx*c + scz*s, -scx*s + scz*c
        if rz != 0.0:
            c, s = math.cos(rz), math.sin(rz)
            scx, scy = scx*c - scy*s, scx*s + scy*c
        sc_v_idx = self.add_vertex(scx + cx, scy + cy, scz + cz)

        snx, sny, snz = 0.0, 0.0, -1.0
        if rx != 0.0:
            c, s = math.cos(rx), math.sin(rx)
            sny, snz = sny*c - snz*s, sny*s + snz*c
        if ry != 0.0:
            c, s = math.cos(ry), math.sin(ry)
            snx, snz = snx*c + snz*s, -snx*s + snz*c
        if rz != 0.0:
            c, s = math.cos(rz), math.sin(rz)
            snx, sny = snx*c - sny*s, snx*s + sny*c
        s_n_idx = self.add_normal(snx, sny, snz)

        for i in range(segments):
            next_i = (i + 1) % segments
            t0 = self.add_texcoord(0.5, 0.5)
            t1 = self.add_texcoord(0.5 + 0.5*math.cos(2.0*math.pi*next_i/segments), 0.5 + 0.5*math.sin(2.0*math.pi*next_i/segments))
            t2 = self.add_texcoord(0.5 + 0.5*math.cos(2.0*math.pi*i/segments), 0.5 + 0.5*math.sin(2.0*math.pi*i/segments))
            
            self.faces[self.current_material].append(
                ((sc_v_idx, t0, s_n_idx),
                 (start_cap_v[next_i], t1, s_n_idx),
                 (start_cap_v[i], t2, s_n_idx))
            )

    def write_obj(self, filename, mtl_name):
        with open(filename, 'w') as f:
            f.write("# SF Super Simple Cannon 3D Model\n")
            f.write(f"mtllib {mtl_name}\n\n")
            
            for v in self.vertices:
                f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            f.write("\n")
            
            for vt in self.texcoords:
                f.write(f"vt {vt[0]:.6f} {vt[1]:.6f}\n")
            f.write("\n")
            
            for vn in self.normals:
                f.write(f"vn {vn[0]:.6f} {vn[1]:.6f} {vn[2]:.6f}\n")
            f.write("\n")
            
            for mat, face_list in self.faces.items():
                f.write(f"usemtl {mat}\n")
                for face in face_list:
                    face_str = " ".join([f"{idx[0]}/{idx[1]}/{idx[2]}" for idx in face])
                    f.write(f"f {face_str}\n")
                f.write("\n")

def main():
    builder = OBJBuilder()

    # --- 1. Cannon Base (大砲の四角い土台/本体) ---
    builder.set_material("Mtl_DarkMetal")
    builder.add_box(0.0, 0.4, 0.0, 0.35, 0.45, 0.8)

    # --- 2. Single Barrel (シンプルで太い1本砲身) ---
    builder.set_material("Mtl_LightMetal")
    builder.add_cylinder(0.0, 0.4, -1.0, 0.15, 2.0, segments=16, rx=math.pi/2.0)

    # Output files
    builder.write_obj(
        "c:/Users/k024g/Desktop/TD3_1/project/Application/resources/model/player/playerCannon.obj",
        "playerCannon.mtl"
    )

    # Write MTL file
    with open("c:/Users/k024g/Desktop/TD3_1/project/Application/resources/model/player/playerCannon.mtl", "w") as f:
        f.write("# Material library for SF Super Simple Cannon\n\n")
        
        f.write("newmtl Mtl_DarkMetal\n")
        f.write("Kd 0.15 0.15 0.16\n")
        f.write("Ks 0.50 0.50 0.52\n")
        f.write("Ns 32\n")
        f.write("illum 2\n\n")
        
        f.write("newmtl Mtl_LightMetal\n")
        f.write("Kd 0.35 0.36 0.38\n")
        f.write("Ks 0.70 0.70 0.75\n")
        f.write("Ns 64\n")
        f.write("illum 2\n\n")

    print("Success: Generated playerCannon.obj and playerCannon.mtl")

if __name__ == "__main__":
    main()
