# 屋顶与石墙纹理

使用内置图像生成工具制作。屋顶贴图已绑定到模型，石墙贴图尚未绑定。两张都是基础颜色图片，不包含独立法线、高度、粗糙度通道；重复铺贴时需在模型上检查接缝和尺度。

- `roof_glazed_tiles.png`：金黄色琉璃瓦，瓦垄沿图片纵向，供屋面使用。
- `stone_wall.png`：浅暖灰色砌石，供台基、石墙使用。

## 屋顶应用与尺寸

`Mesh.cpp` 为天安门屋顶对象加载 `roof_glazed_tiles.png`，以白色基色显示图片，避免叠乘原屋顶黄色。使用模型坐标按屋面方向投影，并启用重复铺贴和 mipmap。

`code/shaders/phong.frag` 中 `if (roofSurface == 1) surfaceUV = uv / 650.0;` 控制铺贴尺度：数值越大，瓦片越大；越小，瓦片越密。修改后重启程序即可。当前为颜色贴图，未增加独立凹凸或法线贴图。

## 屋顶提示词

Use case: historical-scene. Asset type: seamless tileable PBR base-color/albedo texture for golden yellow glazed Chinese imperial roof tiles, Tiananmen architectural model. Generate a square 2048x2048 bitmap filled edge to edge by a regular repeating grid of traditional golden ochre yellow glazed cylindrical barrel roof tiles and intervening shallow pan tiles. Orthographic view directly perpendicular to roof surface, no perspective, no roof silhouette, no eaves, no ridge ornaments. Approximately 8 vertical tile channels across and 8 overlapping horizontal courses down. Tile channels run vertically in image; staggered subtle overlap joints, fine realistic glazed ceramic grain, slight natural ochre variation, elegant golden yellow not metallic gold. Even diffuse illumination suitable for an albedo map, no strong specular highlights, no baked shadows, no AO, no directional brightness gradient. Truly seamless and periodic on all four edges, continuous pattern. No text, border, labels, scenery, people, watermarks. Only the flat material texture.

## 石墙提示词

Use case: historical-scene. Asset type: tileable diffuse/albedo texture for the pale stone foundation masonry of a Chinese imperial gate architectural 3D model. Generate a square high-resolution bitmap, 2048x2048 preferred. Entire canvas filled edge to edge with pale warm gray limestone dressed rectangular blocks in evenly spaced horizontal running-bond courses, alternating half-block offset, approximately four blocks across and eight courses high. Subtle warm ivory gray stone color, restrained natural mineral mottling and fine chisel grain, narrow light gray mortar joints. Clean maintained monumental masonry, gentle age variation, no moss, cracks, graffiti or severe decay. Orthographic frontal surface scan, perfectly flat perspective-free view. Flat uniform diffuse lighting for albedo use, no directional illumination or cast shadows, no AO, no hard specular. Design periodic continuous pattern across all four edges for seamless repeat. No red brick, no pavement, no framing, no scenery, no architecture silhouette, no people, no text, no labels or watermark. Only material texture.
