# 元宵灯会：25 秒 OpenGL 动画工程

这是课程大作业的**初始架构和可运行占位场景**，不是最终成片。主题暂定中国传统文化「元宵灯会」，可由小组调整。使用 C++17、OpenGL 3.3 Core、GLSL、GLFW、GLEW、GLM、tinyobjloader。

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

建议分工：一人负责模型与场景，一人负责着色器与贴图，一人负责时间轴与镜头，一人负责整合、录制与报告。人数较少可合并分工。

## Windows 构建

推荐安装 **Visual Studio 2022 的「使用 C++ 的桌面开发」工作负载**和 **CMake 3.20+**，不是只安装 VS Code。已安装 Git。安装后重新打开终端，从项目根目录执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\lantern_scene.exe
```

使用 MSYS2 UCRT64 / MinGW 的组员，可在 UCRT64 环境安装 `mingw-w64-ucrt-x86_64-cmake` 和 `mingw-w64-ucrt-x86_64-ninja`，确认编译器及其 DLL 所在目录已加入当前终端 PATH，再使用另一构建目录：

```powershell
cmake -S . -B build-mingw -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-mingw --parallel
ctest --test-dir build-mingw --output-on-failure
.\build-mingw\lantern_scene.exe
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
- 一个灯笼沿三次 Bézier 曲线移动，其余灯笼轻微摆动，镜头缓慢转动。
- 从 OBJ 读取几何，缺少法线时计算面法线；支持 UV，缺少 UV 时仅使用临时平面映射。
- 显式 VAO/VBO → 顶点着色器 → 光栅化 → Phong 片元着色器。
- CPU 生成红金色纹理并上传 GPU，使用采样器进行纹理贴图；尚未实现外部图片与 MTL 材质加载。

## 对照评分要求

| 要求 | 骨架现状 | 最终交付还需完成 |
| --- | --- | --- |
| 传统文化主题、OBJ、美感（20） | 元宵主题与原创 OBJ 占位 | 正式灯笼、牌坊/庭院场景，构图与素材出处 |
| Shader pipeline（20） | 独立 GLSL 顶点/片元着色器 | 答辩说明输入、坐标变换与输出 |
| Phong 与纹理（20） | 环境光、漫反射、镜面反射与程序纹理 | 正式 UV 与图片贴图，调参；确认老师是否要求图片文件 |
| 合理动画（30） | 25 秒绝对时间轴、位移、摆动、镜头 | 分镜、节奏、动作衔接与成片录制 |
| 高级纹理（每项10） | **未实现** | 可选法线/凹凸贴图或环境贴图，先完成基础项 |
| Bézier 路径（20） | 三次曲线与独立测试 | 调整路径叙事，答辩展示控制点与公式 |

详细协作步骤见 [docs/GitHub协作.md](docs/GitHub协作.md)，分镜建议见 [docs/制作计划.md](docs/制作计划.md)。不要将初始骨架直接作为最终作品提交。