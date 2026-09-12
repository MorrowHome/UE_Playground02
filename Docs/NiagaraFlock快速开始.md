# Niagara Cube 鸟群（给 Unity 用户）

对比原来的 `CubeFlockDemo`（C++ Compute + WPO），这是 **UE 更常见的做法：Niagara GPU Mesh Particles**。

## 和 Unity 的对应关系

| Unity | Unreal |
|------|--------|
| Particle System / VFX Graph | **Niagara System**（`NS_CubeFlock`） |
| Particle System 组件 | Niagara Component / Niagara Actor |
| Mesh Renderer（粒子） | Niagara **Mesh Renderer** |
| Update Particle（力/速度） | Particle Update 模块栈 |
| Exposed Property | User Parameters（本 Demo 先用模块本地参数） |
| Compute Buffer 手写 Boids | Neighbor Grid3D + Scratch Pad（进阶） |

## 怎么看

1. 打开 `Content/NiagaraFlockDemo/Lvl_NiagaraFlock`
2. Play（飞行控制与 CubeFlockDemo 相同：WASD / 鼠标 / QE）
3. **移动鼠标**引导鸟群；**按住右键**转视角（此时不跟踪光标）
4. 双击 `NS_CubeFlock` 可打开 Niagara 编辑器看模块栈

未 Play 时预览窗口里也能看到粒子在动（Niagara 编辑器或关卡里的系统预览）。

鼠标跟随写入 User 参数：`User.FollowTarget` / `User.FollowStrength`（由 `CubeFlockDemoPlayerController` 每帧更新）。

## 这个 Demo 里有什么

路径：`/Game/NiagaraFlockDemo/`

- `NS_CubeFlock`：2000 个 GPU Cube 粒子
- `M_NiagaraCube`：橙色材质（可自己换到 Mesh Renderer Override Materials）
- `Lvl_NiagaraFlock`：独立演示关卡

**Emitter `Cubes`（GPU）大致流程：**

1. **Emitter Update**：`Spawn Burst Instantaneous` × 2000  
2. **Particle Spawn**：初始化寿命 / 球形分布 / 初始速度 / Mesh 朝向  
3. **Particle Update**：
   - Curl Noise（不规则扰动）
   - Point Attraction（缓慢移动的吸引点，类似旧 Demo 的共享目标）
   - Vortex（群体旋转感）
   - Drag + Solve Forces And Velocity
   - Update Mesh Orientation → **Flight Orientation**（朝速度方向飞，比旧 WPO Cube 更像“鸟”）
4. **Mesh Renderer**：引擎自带 Cube，并写了运动矢量（减轻 TSR 残影）

## 和 CubeFlockDemo（C++）的区别

| | CubeFlockDemo | NiagaraFlockDemo |
|--|---------------|------------------|
| 实现 | 自定义 Compute Shader + ISM WPO | Niagara GPU + Mesh Renderer |
| 邻居规则 | 真 Reynolds O(N²) | **力场近似**（噪声+吸引+涡流） |
| 残影 | WPO 无速度 → TSR 残影 | Niagara 默认有粒子速度 → 好很多 |
| 调试方式 | 改 `.usf` / C++ | 打开 Niagara 编辑器调滑条 |
| 扩展 | 自己写空间网格 | 官方 Neighbor Grid3D 模块 |

对 Unity 转 UE：日常特效/鸟群优先学 **Niagara**；只有要深度定制算法或特殊渲染路径时才写 C++ Compute。

## 想做成“真 Boids”时怎么升级

引擎已有：

- `InitializeNeighborGrid` / `PopulateNeighborGrid`
- `SampleNeighbors` / `CalculateNeighbors` / `NeighborBehaviours`

步骤概要：

1. Emitter 设为 **GPU**，打开 **Simulation Stages**
2. 增加 Emitter 级 `NeighborGrid3D` 数据接口
3. 一帧：Populate Grid → 下一阶段：按邻居算 Separation / Alignment / Cohesion
4. 仍用 Mesh Renderer 出图

`Docs/NiagaraFlock_BoidsScratchPad.hlsl` 里有一段可粘贴的 Scratch Pad 参考 HLSL（固定粒子数 O(N) 采样，适合 2k 量级；上万请用 Neighbor Grid）。

## 调参（Niagara 编辑器）

打开 `NS_CubeFlock` → Emitter `Cubes`：

- Spawn Count：鸟数量（改完需重新编译/重启 Play）
- Sphere Radius：初始分布半径
- Curl Noise → Noise Strength / Frequency：乱飞程度
- Point Attraction → Strength / Attractor Position：聚拢与目标
- Vortex Force Amount：旋转感
- Speed Limit：最大速度
- Mesh Uniform Scale：方块大小（约 0.24 ≈ 边长 24cm）

## 重建关卡（可选）

若只要重建关卡/材质（不重建 Niagara 图）：

```powershell
& 'D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\UEProjects\Playground02\Playground02.uproject' -run=pythonscript '-script=E:\UEProjects\Playground02\Scripts\CreateNiagaraFlockDemo_Level.py' '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' -unattended -AllowCommandletRendering
```

Niagara 系统本体已在编辑器里用 Niagara Toolset 搭好并保存；一般不需要重跑。
