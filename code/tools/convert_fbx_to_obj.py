"""Convert an FBX (or any Assimp-supported model) to Wavefront OBJ + MTL.

Usage:
    python convert_fbx_to_obj.py <input.fbx> <output.obj> [output.mtl]

Meshes are flattened with baked node transforms. PBR texture slots that
Assimp did not resolve are skipped; materials keep their diffuse color and
opacity instead.
"""
import os
import sys

import assimp_py as assimp


def convert(fbx_path, obj_path, mtl_path):
    flags = (assimp.Process_Triangulate | assimp.Process_JoinIdenticalVertices
             | assimp.Process_GenNormals | assimp.Process_GenUVCoords
             | assimp.Process_PreTransformVertices)
    scene = assimp.import_file(fbx_path, flags)

    # ---- write MTL ----
    mtl_lines = []
    for i, mat in enumerate(scene.materials):
        name = mat.get('NAME', f'mat_{i}')
        diffuse = mat.get('COLOR_DIFFUSE', [0.5, 0.5, 0.5])
        opacity = mat.get('OPACITY', 1.0)
        mtl_lines.append(f'newmtl mat_{i}')
        mtl_lines.append(f'Kd {diffuse[0]:.4f} {diffuse[1]:.4f} {diffuse[2]:.4f}')
        if opacity < 1.0:
            mtl_lines.append(f'd {opacity:.4f}')
        mtl_lines.append('')
    with open(mtl_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(mtl_lines))

    # ---- write OBJ ----
    vertex_offset = 0  # running 1-based index offset for faces
    with open(obj_path, 'w', encoding='utf-8') as f:
        f.write(f'mtllib {os.path.basename(mtl_path)}\n')
        for mi, m in enumerate(scene.meshes):
            f.write(f'o mesh_{mi}\n')
            f.write(f'usemtl mat_{m.material_index}\n')
            nv = m.num_vertices
            for i in range(nv):
                x, y, z = m.vertices[3 * i], m.vertices[3 * i + 1], m.vertices[3 * i + 2]
                f.write(f'v {x:.6f} {y:.6f} {z:.6f}\n')
            for i in range(nv):
                x, y, z = m.normals[3 * i], m.normals[3 * i + 1], m.normals[3 * i + 2]
                f.write(f'vn {x:.6f} {y:.6f} {z:.6f}\n')
            if m.texcoords:
                for i in range(nv):
                    u, v = m.texcoords[0][2 * i], m.texcoords[0][2 * i + 1]
                    f.write(f'vt {u:.6f} {v:.6f}\n')
            # faces: triangulated, index shared by v/vt/vn (1:1 after join)
            for j in range(0, len(m.indices), 3):
                ia = m.indices[j] + vertex_offset + 1
                ib = m.indices[j + 1] + vertex_offset + 1
                ic = m.indices[j + 2] + vertex_offset + 1
                f.write(f'f {ia}/{ia}/{ia} {ib}/{ib}/{ib} {ic}/{ic}/{ic}\n')
            vertex_offset += nv

    print(f'Done: {obj_path} ({vertex_offset} vertices)')


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    fbx = sys.argv[1]
    obj = sys.argv[2]
    mtl = sys.argv[3] if len(sys.argv) > 3 else os.path.splitext(obj)[0] + '.mtl'
    convert(fbx, obj, mtl)
