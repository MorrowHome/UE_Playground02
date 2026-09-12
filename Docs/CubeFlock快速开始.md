# GPU Cube 鸟群：入门操作

这个示例使用 2,000 个 UE 自带 Cube 表示鸟，不需要额外模型、Niagara 或 UE MCP。

## 开始观看

1. 用 UE 5.8 打开 Playground02.uproject。
2. 打开 Content / CubeFlockDemo / Lvl_CubeFlock（如果已在此关卡，跳过）。
3. 点击顶部绿色三角形 Play，然后点击画面捕获鼠标。
4. WASD 前后左右飞行，鼠标转动视角，Q/E 下降/上升。
5. Esc 停止运行；Shift+F1 在运行中释放鼠标。

未点击 Play 时，Cube 保持静止是正常现象。

## 调整效果

停止 Play，在右侧 Outliner 选中 CubeFlockDemo_2000_GPU_Cubes，在 Details 中搜索 Flock。

- Bird Count：默认 2000，支持 1～4096，下次 Play 生效。
- Cube Size：方块边长，单位厘米，默认 24。
- Flock Radius：水平飞行半径，默认 1400 厘米；竖直范围为其 45%。
- Speed：基础移动速度，默认 330 厘米/秒。
- Neighbor Radius：感知邻居的距离，默认 280 厘米。
- Separation Weight：避免贴近其他 Cube 的力度。
- Alignment Weight：跟随邻居方向的力度。
- Cohesion Weight：向邻居中心靠拢的力度。
- Random Seed：初始分布的随机种子；相同种子提供相同初始状态。

可以移动整个 Actor 改变群体中心。此示例请保持 Actor 旋转为 0、缩放为 1。
Cube 不使用物理碰撞，分离规则是软避让；没有实现对场景墙壁或地面的避障。

## 工作原理

C++ 创建 Cube 实例、初始化位置和速度。之后每帧只上传少量参数：

1. CubeFlock.usf 在 GPU 上读取上一帧的所有鸟。
2. 每只鸟按分离、对齐、聚合、目标吸引、边界回转规则更新。
3. 将新状态保留在 GPU Buffer，并将位置写入 64×64 浮点纹理。
4. 材质在顶点阶段读取该实例的位置，通过 World Position Offset 移动 Cube。

GPU Buffer 的输入和输出分离，避免线程互相读到半更新状态。
普通运行没有每帧 CPU 位置读回，也不会逐个更新 CPU 实例变换。
为了使源码容易理解，邻居计算采用共享内存分块的全体遍历，复杂度仍是 O(N²)。
它适用于这个 2000 个实例的入门演示；扩展到数万只前应改为空间网格。

## 代码入口

- Plugins/CubeFlock/Shaders/CubeFlock.usf：鸟群运动规则。
- Plugins/CubeFlock/Source/CubeFlock/Private/CubeFlockActor.cpp：GPU 调度与实例创建。
- Plugins/CubeFlock/Source/CubeFlock/Public/CubeFlockActor.h：编辑器参数。
- Scripts/CreateCubeFlockDemo.py：生成材质和独立演示关卡，日常观看不需要执行。

## 重新构建

关闭 UE 后运行（本机引擎路径为 D:\UE\UE_5.8）：

```powershell
& 'D:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat' Playground02Editor Win64 Development '-Project=E:\UEProjects\Playground02\Playground02.uproject' -WaitMutex
```

需要重建演示资源时运行下面命令。它会重建演示材质及带 CubeFlockDemo_ 前缀的演示 Actor：

```powershell
& 'D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\UEProjects\Playground02\Playground02.uproject' -run=pythonscript '-script=E:\UEProjects\Playground02\Scripts\CreateCubeFlockDemo.py' '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' -unattended -AllowCommandletRendering
```

## GPU 集成验证

```powershell
& 'D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\UEProjects\Playground02\Playground02.uproject' /Game/CubeFlockDemo/Lvl_CubeFlock -game -RenderOffscreen -unattended -BirdFlockValidate
```

该参数仅用于测试：在两个时刻读取 GPU 位置，检查所有实例都移动、数值有限、在边界内，并保存截图后退出。
日志中的 GPU_VALIDATION_PASSED 表示通过。正常 Play 不启用此读回检查。
生成的 .uasset / .umap 由现有 Git LFS 规则管理，插件编译产物已忽略。

