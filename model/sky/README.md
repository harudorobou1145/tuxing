# 日间天空

`day_sky.png` 为内置图像生成工具生成的 2:1 经纬度天空全景。使用 `sky.vert` / `sky.frag` 按摄像机朝向采样，在场景几何前绘制，不写入深度，也不随摄像机平移产生视差。水平方向重复采样，垂直方向限制边界。

天空图用于背景与远处地面的大气过渡（水平距离 90～280 场景单位渐变），消除超大地面形成的深色地平线带。没有加入物体反射；相机、模型比例、路径和光照设置均保持原值。

同名替换 PNG 并重启程序即可更换天空。新图片应采用 2:1 经纬度全景，地平线在中线附近，上半张对应头顶天空；普通照片会产生明显拉伸。可在 `sky.frag` 的经度计算处增加偏移调整朝向。

## 生成提示词

Use case: photorealistic-natural. Asset type: 360-degree equirectangular panoramic sky texture for a real-time OpenGL daytime architectural parade scene. Generate a high-resolution landscape 2:1 image, 4096x2048 preferred. Latitude-longitude spherical projection: entire sphere is SKY ONLY, upper half is the visible northern sky hemisphere, zenith at top image edge, horizon precisely at the horizontal center line, lower half a calm pale sky-colored continuation below horizon. Clear pleasant daytime azure blue zenith smoothly fading to pale blue near horizon, sparse small soft white fair-weather cumulus clouds concentrated just above the horizon, very few clouds toward zenith. Natural photographic cloud texture with soft diffuse daylight, no dramatic storm, no visible sun disk or glare, no birds, no ground, no buildings, no landscape, no text or watermark. Left and right edges must join continuously for 360-degree longitude wrap, top pole smooth blue with no seam. Clean uncluttered backdrop supporting the buildings rather than dominating. This is a texture map, not a perspective shot or a scene render.
