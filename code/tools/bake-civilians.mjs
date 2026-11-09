import fs from 'node:fs';
import path from 'node:path';
import { AnimationMixer, Vector3 } from '../../build/asset-tools/node_modules/three/build/three.module.js';
import { GLTFLoader } from '../../build/asset-tools/node_modules/three/examples/jsm/loaders/GLTFLoader.js';

globalThis.ProgressEvent ??= class ProgressEvent {};
const directory = path.resolve('model/civilians');
for (const [name, pose, time] of [['casual', 'Idle_Neutral', 0.6], ['hoodie', 'Walk', 0.2], ['suit', 'Wave', 0.8]]) {
    const gltf = await new GLTFLoader().parseAsync(fs.readFileSync(path.join(directory, `${name}.gltf`), 'utf8'), '');
    const mixer = new AnimationMixer(gltf.scene);
    const clip = gltf.animations.find(animation => animation.name === pose);
    if (!clip) throw new Error(`Missing pose ${pose}`);
    mixer.clipAction(clip).play();
    mixer.update(time);
    gltf.scene.updateMatrixWorld(true);
    const output = [`mtllib ${name}.mtl`];
    const materials = [];
    let vertexOffset = 1;
    let meshIndex = 0;
    gltf.scene.traverse(mesh => {
        if (!mesh.isMesh) return;
        mesh.skeleton?.update();
        const material = mesh.material;
        if (Array.isArray(material)) throw new Error('Unexpected material array');
        const materialName = `part_${meshIndex++}`;
        const color = material.color;
        materials.push(`newmtl ${materialName}\nKd ${color.r} ${color.g} ${color.b}\nd 1\n`);
        output.push(`o ${materialName}`, `usemtl ${materialName}`);
        const positions = mesh.geometry.attributes.position;
        for (let index = 0; index < positions.count; ++index) {
            const point = mesh.getVertexPosition(index, new Vector3()).applyMatrix4(mesh.matrixWorld);
            output.push(`v ${point.x} ${point.y} ${point.z}`);
        }
        const indices = mesh.geometry.index;
        const count = indices ? indices.count : positions.count;
        for (let index = 0; index < count; index += 3) {
            const face = [0, 1, 2].map(corner => vertexOffset + (indices ? indices.getX(index + corner) : index + corner));
            output.push(`f ${face.join(' ')}`);
        }
        vertexOffset += positions.count;
    });
    fs.writeFileSync(path.join(directory, `${name}.obj`), output.join('\n'));
    fs.writeFileSync(path.join(directory, `${name}.mtl`), materials.join('\n'));
    console.log(`${name}: ${vertexOffset - 1} vertices, ${pose} at ${time}s`);
}