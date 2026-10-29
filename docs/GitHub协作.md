# GitHub 建仓与小组协作

## 1. 网页创建仓库

1. 注册或登录 https://github.com ，点击右上角 `+` → `New repository`。
2. Repository name 可填 `lantern-festival-opengl`。
3. 推荐选择 **Private**，是否公开以课程要求为准；老师需要访问时记得邀请或调整可见性。
4. **不要勾选**初始化 README、.gitignore、License，本地已经准备了文件；许可证需全组决定。
5. 点击 `Create repository`，复制 HTTPS 地址，例如 `https://github.com/你的用户名/lantern-festival-opengl.git`。

## 2. 将本地工程第一次推送

在 VS Code 打开 `final_work` 整个文件夹，终端应位于工程根目录。以下仅是操作说明，助手没有替你创建远程仓库、提交或推送。

首次使用 Git，先设置身份（邮箱建议使用 GitHub Settings → Emails 中的 noreply 邮箱）：

```powershell
git config --global user.name "你的名字"
git config --global user.email "你的GitHub邮箱"
```

再执行下面命令，将地址换成刚才复制的真实地址：

```powershell
git init
git add .
git status
git commit -m "chore: initialize OpenGL animation project"
git branch -M main
git remote add origin https://github.com/你的用户名/lantern-festival-opengl.git
git push -u origin main
```

`git status` 时确认没有 `build/`、可执行程序或大型无关素材。当前忽略规则特意保留 `model/` 中的 `.obj`，因为这里的 OBJ 是模型而不是编译中间文件。

推送时由 Git Credential Manager 提示浏览器登录；GitHub 不支持直接使用账户密码进行 HTTPS Git 推送。不要把令牌、密码或 SSH 私钥写入代码、聊天或提交记录。

如果 `origin` 已存在，先 `git remote -v` 检查，不要重复添加；确认地址错误后再使用 `git remote set-url origin 真实地址`。如果远程已初始化其他文件，不要强推，先协调合并或重新创建空仓库。

## 3. 邀请组员

仓库页面 → `Settings` → `Collaborators`（组织仓库可能显示 `Collaborators and teams`）→ `Add people` → 输入组员 GitHub 用户名。组员接受邀请后即可访问私有仓库并推送。

组员第一次下载：

```powershell
git clone https://github.com/仓库拥有者/lantern-festival-opengl.git
cd lantern-festival-opengl
```

每人需要独立安装编译环境，依赖由 CMake 下载，不共享各自的 build 文件夹。

## 4. 每次开发流程

在工作区无未提交改动时，从最新 main 新建自己的功能分支：

```powershell
git switch main
git pull --ff-only
git switch -c feat/lantern-model
```

修改、构建和测试后：

```powershell
git add model/ code/
git diff --cached
git commit -m "feat: improve lantern model"
git push -u origin feat/lantern-model
```

在 GitHub 点击 `Compare & pull request`，填写改了什么、怎样测试、效果截图；由另一位组员检查后合并。每项任务用新的分支名，例如 `feat/phong-lighting`、`feat/bezier-camera`。不要多人同时直接修改 main。

## 5. 协作约定

- 尽量按模块分工；尤其避免多人同时大改主循环和同一个 shader。
- 替换模型后检查尺度、朝向、原点、法线、UV，记录下载地址和许可。
- 普通 GitHub 单文件不能超过 100 MiB；大模型提前评估 Git LFS，视频优先放 Releases 或共享网盘，不直接塞进代码仓库。
- 不上传第三方编译产物、账号信息和不允许再分发的模型。
- 每次合并保证可构建，交作业前录制完整 20～30 秒动画，并固定最终提交版本。