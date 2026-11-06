"""Convert the nuclear missile launcher GLB to OBJ with per-mesh military colors.

The source model is a flat olive-green placeholder. A DF ICBM TEL has a green
chassis and a light-gray missile canister, so we classify each mesh by its
height (canister sits on top) and paint it accordingly.
"""
import os
import sys

import assimp_py as assimp


def convert(glb_path, obj_path, mtl_path):
    flags = (assimp.Process_Triangulate | assimp.Process_JoinIdenticalVertices
             | assimp.Process_GenNormals | assimp.Process_GenUVCoords)
    scene = assimp.import_file(glb_path, flags)

    # The export carries two overlapping LOD copies; the second is scaled ~83x
    # and offset far away. Keep only the first, identity-transformed copy.
    meshes = scene.meshes[:len(scene.meshes) // 2]

    OLIVE = (0.33, 0.42, 0.20)   # military olive-green chassis
    GRAY = (0.74, 0.75, 0.72)    # missile canister

    with open(obj_path, 'w', encoding='utf-8') as obj, open(mtl_path, 'w', encoding='utf-8') as mtl:
        obj.write(f'mtllib {os.path.basename(mtl_path)}\n')
        offset = 0
        for mi, m in enumerate(meshes):
            nv = m.num_vertices
            ys = [m.vertices[3 * j + 1] for j in range(nv)]
            center_y = sum(ys) / nv if nv else 0.0
            color = GRAY if center_y > 0.9 else OLIVE
            mtl.write(f'newmtl mat_{mi}\n')
            mtl.write(f'Kd {color[0]:.4f} {color[1]:.4f} {color[2]:.4f}\n\n')

            obj.write(f'o mesh_{mi}\n')
            obj.write(f'usemtl mat_{mi}\n')
            for j in range(nv):
                x, y, z = m.vertices[3 * j], m.vertices[3 * j + 1], m.vertices[3 * j + 2]
                obj.write(f'v {x:.6f} {y:.6f} {z:.6f}\n')
            for j in range(nv):
                x, y, z = m.normals[3 * j], m.normals[3 * j + 1], m.normals[3 * j + 2]
                obj.write(f'vn {x:.6f} {y:.6f} {z:.6f}\n')
            for j in range(0, len(m.indices), 3):
                a = m.indices[j] + offset + 1
                b = m.indices[j + 1] + offset + 1
                c = m.indices[j + 2] + offset + 1
                obj.write(f'f {a}//{a} {b}//{b} {c}//{c}\n')
            offset += nv

    print(f'Done: {obj_path} ({offset} vertices)')


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    convert(sys.argv[1], sys.argv[2],
            sys.argv[3] if len(sys.argv) > 3 else os.path.splitext(sys.argv[2])[0] + '.mtl')
