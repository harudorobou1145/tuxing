# 阅兵动画：25 秒 OpenGL 工程

统一命名：CMake 工程 `ParadeAnimation`，可执行目标 `parade_scene`，GitHub 新仓库建议名 `parade-animation-opengl`。已有远程仓库不必重建。当前为五段分镜的简化阅兵动画原型，不是最终成片。

## 运行与分镜

主程序默认加载精简天安门核心场景、四种坦克、真实军人和歼-20 OBJ。空格暂停，R 重播，左右方向键跳转分镜，B 切换凹凸效果，Esc 退出。

核心场景为 [model/tiananmen/tiananmen_core.obj](model/tiananmen/tiananmen_core.obj)，保留包围盒完全位于源坐标 X=-16000 到 14000、Z=-6000 到 26000 内的对象，不切割建筑。删除远处街区及超大背景，程序地面补足空地。共 253 个对象、875552 个多边形，约 77.5 MB；原始约 600 MB 文件保持不变，可通过下方自定义路径命令加载。原材质缺失，提取版仅保留几何、UV 和法线，使用默认白色材质。重新提取运行 `./code/tools/extract-scene.ps1`。下文有关完整广场的大小与加载成本仅适用于手动加载原文件。

| 时间 | 当前简化效果 |
| --- | --- |
| 0～5 秒 | 镜头从远处向城楼推进 |
| 5～10 秒 | 展示四种坦克沿道路通过 |
| 10～15 秒 | 5×4 真实军人 OBJ 队列平移，暂为静态 T 姿态 |
| 15～20 秒 | 镜头转向风中旗帜并靠近 |
| 20～25 秒 | 镜头拉开抬向天空，三机编队沿 Bézier 路径飞过 |

本机直接运行：`./build/parade_scene.exe`。也可使用 `./build/parade_scene.exe --capture build/captures` 导出五个固定时间点的 PNG，用于检查分镜；截图模式完成后自动退出，不是视频导出。

本机现有 Ninja 构建目录生成的程序为 `build/parade_scene.exe`。旧的 `lantern_scene.exe` 即使仍留在构建目录中，也不再是当前构建目标；若曾设置 Windows 独显偏好，需要为新程序路径重新设置一次。

也可以传入另一个 OBJ 路径，例如 Visual Studio Debug 构建：

```powershell
.\build\Debug\parade_scene.exe ".\model\tiananmen\Tiananmen+Square+3D+Scene2.obj"
```

默认使用约 600 MB 的完整广场，保留全部几何，以城楼坐标而非巨大的背景地面包围盒定位。解析及展开三角形会额外占用大量内存，首次加载需要等待。其引用的 MTL 当前缺失，因此广场使用默认白色材质，尚非最终配色。也可以传入小城楼 OBJ 作快速预览。自定义模型仍使用固定分镜，不保证取景合适。场景假定 Y 轴向上。坦克读取 MTL 漫反射颜色，飞机和军人读取漫反射贴图。透明、镜面材质参数尚未完整还原。损坏 UV 使用临时平面映射并输出读取警告。

完整广场文件超过 GitHub 普通 Git 的 100 MiB 单文件限制，不要直接提交推送；共享前应配置 Git LFS 或使用共享下载地址，并检查素材授权。

使用 C++17、OpenGL 3.3 Core、GLSL、GLFW、GLEW、GLM、tinyobjloader 与 stb_image/stb_image_write。军人使用 Blender 导出的 OBJ，跳过其中的默认 Cube，并从相邻 textures 目录回退查找导出时失效的绝对贴图路径。OBJ 不保留骨骼，所以目前仍是 T 姿态平移，尚无正式步态。坦克朝向已反转，独立 TrackMesh 履带使用循环程序纹理表现滚动，不是履带链节几何运动，车轮尚未旋转。飞机增加原模型前向校正、沿路径俯仰及随航向旋转的编队偏移。

## 目录与分工

```text
final_work/
├── CMakeLists.txt          构建入口、依赖版本、测试
├── code/
│   ├── include/            Shader / Mesh / Timeline 接口
│   ├── src/                窗口、主循环、OBJ 加载、GPU 资源
│   ├── shaders/            顶点变换与 Phong 片元着色器
│   └── tests/              时间轴与 Bézier 路径测试
├── model/                 共享 OBJ 模型和资源来源记录
└── docs/                  GitHub 操作和任务规划
```

建议分工：一人负责天安门与广场场景，一人负责人物模型和方队步态，一人负责光照贴图与镜头，一人负责整合、录制与报告。人数较少可合并分工。

## Windows 构建

推荐安装 **Visual Studio 2022 的「使用 C++ 的桌面开发」工作负载**和 **CMake 3.20+**，不是只安装 VS Code。已安装 Git。安装后重新打开终端，从项目根目录执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\parade_scene.exe
```

使用 MSYS2 UCRT64 / MinGW 的组员，可在 UCRT64 环境安装 `mingw-w64-ucrt-x86_64-cmake` 和 `mingw-w64-ucrt-x86_64-ninja`，确认编译器及其 DLL 所在目录已加入当前终端 PATH，再使用另一构建目录：

```powershell
cmake -S . -B build-mingw -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-mingw --parallel
ctest --test-dir build-mingw --output-on-failure
.\build-mingw\parade_scene.exe
```

首次配置需要联网从 GitHub 下载依赖；不同生成器不要共用构建目录。需要显卡和驱动支持 OpenGL 3.3。模型和 shader 路径由 CMake 配置，移动工程后应重新配置，不要把本机生成的程序当成可移植发布包。

只测试动画数学逻辑，不下载图形依赖：

```powershell
cmake -S . -B build-test -DBUILD_SCENE=OFF
cmake --build build-test --config Debug
ctest --test-dir build-test -C Debug --output-on-failure
```

## 当前效果

- 自动播放 25 秒，到终点停止；空格暂停/继续，R 从头播放，Esc 退出。
- 完整场景城楼推进、坦克通过、真实人物队列平移、红旗风动、飞机编队五段连续镜头。
- 飞机实际沿三次 Bézier 曲线运动，朝向由相邻曲线点估计；镜头使用平滑分段插值。
- 从 OBJ 读取几何，缺少法线时计算面法线；支持 UV，缺少 UV 时仅使用临时平面映射。
- 显式 VAO/VBO → 顶点着色器 → 光栅化 → Phong 片元着色器。
- MTL 漫反射颜色与外部图片贴图；程序地砖纹理同时作为凹凸高度图，通过屏幕空间导数扰动法线，可用 B 键对比。
- 细分旗面在顶点着色器中随时间形变并更新法线；五星图案在片元着色器中生成，属于程序图案而非额外高级贴图加分项。

## 对照评分要求

| 要求 | 骨架现状 | 最终交付还需完成 |
| --- | --- | --- |
| 限定主题、OBJ、美感（20） | 阅兵主题、多 OBJ 场景 | 完善构图与素材出处，主题按课程解释确认 |
| Shader pipeline（20） | 独立 GLSL 顶点/片元着色器 | 答辩说明输入、坐标变换与输出 |
| Phong 与纹理（20） | Phong 三项、飞机 PNG、MTL 颜色、地砖纹理 | 正式城楼材质、精调光照 |
| 合理动画（30） | 坦克平移及履带表面滚动、真实人物平移、旗面风动、飞机飞行 | 正式人物步态、履带链节与轮子运动、改善接地与镜头节奏 |
| 高级纹理（每项10） | 地面高度图 bump mapping，可开关对比 | 保留对比截图并解释法线扰动；未实现环境贴图 |
| Bézier 路径（20） | 飞机实际使用三次曲线，含独立测试 | 展示控制点与公式；尚未做弧长匀速 |

详细协作步骤见 [docs/GitHub协作.md](docs/GitHub协作.md)，分镜建议见 [docs/制作计划.md](docs/制作计划.md)。不要将初始骨架直接作为最终作品提交。
