"""Convert a 3ds Max .max file to OBJ using Blender (headless) + io_scene_max.

Run as a Blender background script:
    blender --background --python convert_max_to_obj.py -- <input.max> <output.obj>
"""
import sys

import bpy

argv = sys.argv
argv = argv[argv.index('--') + 1:] if '--' in argv else []
if len(argv) < 2:
    print(__doc__)
    sys.exit(1)
input_path, output_path = argv[0], argv[1]

# Empty scene, then enable the importer and bring in the .max.
bpy.ops.wm.read_factory_settings(use_empty=True)
try:
    bpy.ops.preferences.addon_enable(module='io_scene_max')
except Exception as exc:  # addon may already be enabled
    print(f'addon_enable: {exc}')

try:
    bpy.ops.import_scene.max(filepath=input_path)
except Exception as exc:
    print(f'import failed: {exc}')
    sys.exit(1)

# Drop any empties/curves/cameras so only mesh data remains.
for obj in list(bpy.data.objects):
    if obj.type != 'MESH':
        bpy.data.objects.remove(obj, do_unlink=True)

bpy.ops.wm.obj_export(filepath=output_path, export_materials=True, export_uv=True)
print(f'exported: {output_path}  meshes={len(bpy.data.objects)}')
