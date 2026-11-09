# Plaza Visitors

Source: Quaternius, Ultimate Modular Men Pack (February 2022).

https://quaternius.com/packs/ultimatemodularcharacters.html

License: CC0 1.0 Universal, https://creativecommons.org/publicdomain/zero/1.0/

Original files: Casual_2.gltf, Casual_Hoodie.gltf, Suit.gltf.
The included OBJ/MTL files bake Idle_Neutral, Walk and Wave animation poses.
They are static visitors, not a runtime skeletal animation system.

To regenerate from the included glTF sources:

```sh
npm install --prefix build/asset-tools three
node code/tools/bake-civilians.mjs
```