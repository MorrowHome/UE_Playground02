# Chapter5_2：从 Unity 到 UE 的 Mass-spring 课程详解

> 根据老师本地成品源码整理，核对日期：2026-09-12。目标项目为 Playground02，`.uproject` 声明 UE 5.8，本机存在 `D:/UE/UE_5.8/Engine/Source`。本文解释已读到的代码，不代表看过课程视频；UE 部分是建议实现方案，尚未实现或实测。

## 1. 先纠正一个容易走错方向的理解

这一节做的是**基于粒子表示的 GPU 刚体碰撞模拟**：一个方块由许多小球代表碰撞形状，小球接触时产生弹簧、阻尼和切向力，再把力汇总到方块，使方块平移和旋转。

它没有建立一张连接相邻质点的弹簧拓扑，也没有让各个质点独立变形。因此，它并不是常见的“布料上的质点弹簧网格”。这里的 spring 是碰撞发生时的临时接触弹簧，mass 主要由刚体质量参与积分。

想象用一盒固定排列的小球代表一个硬方块。小球的位置相对于盒子保持不变。碰撞时，它们负责感知哪里被挤压、产生多大的推力；真正更新运动的是整个盒子。

这种结构与 NVIDIA 的粒子刚体方法相符，但仅凭代码不能断定老师的引用来源。[GPU Gems 3，第 29 章](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-29-real-time-rigid-body-simulation-gpus)

## 2. 阅读入口与场景参数

请按以下顺序阅读老师文件，路径中的实际目录名是 `Baidu_net_disk`：

1. [Chapter5_2.cs](<E:/Baidu_net_disk/课程源码及课件/3. Compute Shaders/Assets/Scripts/Chapter5_2.cs>)：初始化数据、绑定参数、调度和绘制。
2. [Chapter5_2.compute](<E:/Baidu_net_disk/课程源码及课件/3. Compute Shaders/Assets/Shaders/Chapter5_2.compute>)：四个计算阶段和接触力。
3. [Chapter5_2.shader](<E:/Baidu_net_disk/课程源码及课件/3. Compute Shaders/Assets/Shaders/Chapter5_2.shader>)：按实例编号读取刚体位姿，变换方块顶点。
4. [Chapter5_2_debugParticle.shader](<E:/Baidu_net_disk/课程源码及课件/3. Compute Shaders/Assets/Shaders/Chapter5_2_debugParticle.shader>)：把碰撞采样粒子画出来。
5. [Chapter5_2.unity](<E:/Baidu_net_disk/课程源码及课件/3. Compute Shaders/Assets/Scenes/Chapter5_2.unity>)：检查 Inspector 保存的实际参数。

**脚本默认值与成品场景值不同。** 脚本默认 1000 个刚体、边长 1、质量 10；场景保存的是 100 个刚体、生成范围 50、边长 10、每边 4 个粒子、质量 1。场景的弹簧系数为 50，阻尼为 0.1，切向系数为 0.01，重力、线性和角运动缩放均为 1。

这些是老师的演示数值，不能直接当作 SI 物理参数移植到 UE。

## 3. 两层数据：刚体负责运动，粒子负责接触

### 3.1 RigidBody

- `position`：刚体中心的世界位置。
- `quaternion`：方向，四元数分量为 xyz、w。
- `linearVelocity`：中心每秒移动多少。
- `angularVelocity`：世界空间角速度，单位应为弧度/秒。
- `particleIndex`、`particleCount`：该刚体在粒子数组中的连续区间。

这里的 `RigidBody` 是自定义结构体，并不是 Unity 的 `Rigidbody` 组件；同样，移植到 UE 后也不需要给每个方块创建一个 Actor 或 Chaos 刚体。

### 3.2 Particle

- `localPosition`：相对于刚体中心、在刚体局部坐标系里的固定位置。
- `offsetPosition`：局部位置经过刚体旋转后的世界空间偏移。
- `position`：最终世界位置。
- `velocity`：刚体运动使这个采样点产生的速度。
- `force`：本次接触计算得到的合力。

要点：`localPosition` 不会被接触力拉开。只要它保持固定，形状就保持刚性。

### 3.3 如何在方块里放小球

设方块边长为 L，每边粒子数为 n：

```text
每个刚体粒子数 S = n³
粒子直径 d = L / n，半径 r = d / 2
起始坐标 start = -(n - 1) × d / 2
某轴坐标 = start + index × d，index ∈ [0, n-1]
总粒子数 P = 刚体数 B × S
```

当 L=1、n=4 时，每轴坐标为 -0.375、-0.125、0.125、0.375，共 64 个小球。外侧球面到达 ±0.5，但球的并集并不精确覆盖立方体表面、棱角和全部体积，因此碰撞形状只是近似。

老师填充的是整个体积，不仅是表面。第一版保留这种布局便于对照，后续才考虑表面采样；改变采样后需要重新标定接触刚度，不能假设行为不变。

## 4. 一步模拟的四个阶段

```text
刚体状态（位置、朝向、速度）
  → UpdateParticle：重建粒子位置和速度
  → ComputeParticle：遍历接触，计算每粒子受力
  → ComputeRigidbody：汇总力、力矩，更新速度
  → ComputeRigidbodyMovement：更新位置、朝向
  → 绘制方块；下一子步再次重建粒子
```

这些阶段有数据依赖。GPU 上一个线程组的同步屏障不能同步整个 Dispatch，跨阶段应使用独立 Pass 和正确资源依赖。

### 4.1 UpdateParticle：刚体如何带着采样点运动

记中心为 x，朝向为 q，局部位置为 l，中心速度为 v，角速度为 ω：

```text
r = Rotate(q, l)
p = x + r
v_particle = v + ω × r
```

`ω × r` 是旋转造成的线速度。比如绕 Z 轴正向旋转，位于 +X 的采样点沿 +Y 运动。忽略这一项，旋转着的方块接触地面时，阻尼和切向反应就会算错。

老师的一个 GPU 线程处理一个刚体，然后循环它的所有粒子。UE 初版可保留这个分工；大量粒子时也可以改成每个线程负责一个粒子，通过 BodyID 找到所属刚体。

### 4.2 ComputeParticle：挤压为什么会产生反弹

设当前粒子为 i，另一粒子为 j，方向从 i 指向 j：

```text
delta = p_j - p_i
distance = length(delta)
n = delta / distance
penetration = d - distance
```

只有 `distance < d` 时，两球重叠。老师的排斥力为：

```text
F_spring = -k × penetration × n
```

负号让当前粒子远离另一粒子。例如 j 在 i 的右边，n 指向右，i 得到向左的力。压得越深，反弹越强；不重叠时就没有这根接触弹簧，也没有远距离吸引。

这叫罚力法：允许少量穿透，用惩罚力把物体推出。静止时通常也需要一点压入量来产生平衡重力的支撑力。把刚度调大可以减少压入，但会使积分更难稳定。

### 4.3 阻尼和切向力分别做什么

老师代码使用：

```text
v_rel = v_j - v_i
F_damping = c × v_rel
v_t = v_rel - dot(v_rel, n) × n
F_tangent = μ_t × v_t
F_contact = F_spring + F_damping + F_tangent
```

阻尼耗散相对运动，避免不断弹跳；切向项抑制接触面的滑动。注意原版阻尼作用于完整相对速度，已经包含切向阻尼，再加切向项会进一步增强切向耗散。

`tangentialCoefficient` 并不是严格的库仑摩擦系数。原版没有静摩擦求解，也没有 `|F_t| ≤ μ F_n` 的摩擦上限。

建议改进版使用接触法线分量，并限制法向力非负：

```text
v_n = dot(v_rel, n)
F_n = max(0, k × penetration - c_n × v_n)
F_normal = -F_n × n
F_tangent = min(c_t × |v_t|, μ × F_n) × safeNormalize(v_t)
```

这里仍沿用“n 从 i 指向 j”的定义。两球相向靠近时 `v_n < 0`，阻尼增大排斥力；分离时减弱排斥，截断避免接触产生吸引。`safeNormalize` 在零向量时返回零。这依然只是简化摩擦模型，不能据此宣称获得可靠静摩擦堆叠。

### 4.4 地面是怎么来的

原版以 `otherID=-1` 表示地面，并构造虚拟小球，让采样点受到向上的推力。地面不是从场景碰撞系统查询出来的。

UE 版建议直接使用解析平面。设向上单位法线 N、平面上一点 p0，粒子中心 p、半径 r：

```text
signedDistance = dot(p - p0, N)
penetration = r - signedDistance
```

`penetration > 0` 时产生沿 N 的接触力。UE 中水平地面可用 N=(0,0,1)、p0=(0,0,0)。对于静止地面，法向力标量可写为 `max(0, k×penetration - c_n×dot(v_particle,N))`。

这只支持你实现的平面，并不会自动碰撞 UE 里的墙、角色或所有 Static Mesh。

### 4.5 ComputeRigidbody：为什么碰角会旋转

每个接触粒子都把自己的力贡献给刚体：

```text
F_total = Σ F_particle
τ_total = Σ (r_particle × F_particle)
```

F 改变中心速度，τ 是力矩，改变角速度。力越偏离中心，通常越容易引起旋转。对称的支撑力可以相互抵消力矩，因此平放方块不会凭空旋转。

老师线速度积分为 `v += linearForceScalar × F_total × h / M`。角速度积分却是 `ω += angularForceScalar × τ_total × h`，用经验缩放替代了转动惯量。

物理一致版本应考虑惯量 I。均匀实心立方体的三个主惯量相同：`I = M L² / 6`，因此本章方块可用 `ω += τ_total × h / I`。一般刚体需要世界空间惯量张量，完整方程还包含陀螺项 `ω × (Iω)`；不要把立方体的标量公式推广到任意形状。

### 4.6 ComputeRigidbodyMovement：速度变成位置和朝向

先更新速度，再用新速度更新位置，是半隐式欧拉的基本顺序：

```text
v_next = v + a × h
x_next = x + v_next × h
q_next = normalize(q + 0.5 × h × (ω,0) ⊗ q)
```

最后一式采用世界空间角速度、Hamilton 四元数乘法、q 将局部向量转到世界的约定，和老师的左乘顺序一致。不要把 `q ⊗ (ω,0)` 与 `(ω,0) ⊗ q` 随意交换。

归一化能维持旋转四元数的单位长度，但不能解决时间步过大引起的物理不稳定。用绕单轴转 90° 的小例子验证 UE/HLSL 的约定，比直接猜符号可靠。

## 5. 绘制不是逐个移动 GameObject

CPU 初始化数组并上传 ComputeBuffer；GPU 更新刚体状态；顶点 Shader 用 `SV_InstanceID` 查到刚体位置和四元数，把同一份 Cube Mesh 变换成许多实例。

`DrawMeshInstancedIndirect` 的参数缓冲决定绘制索引和实例数。原版实例数由 CPU 初始化后固定，并没有做 GPU 可见性压缩。Debug 模式改为每个采样点画一个小球。

这节课的渲染价值在于：**状态保留在 GPU，渲染直接消费模拟结果**。在 UE 中每帧读回所有位置、再循环设置 Actor Transform，虽然能动，却丢掉了这条核心设计。

## 6. 原版需要理解并修正的地方

### 6.1 时间步设置晚了一拍

`Update()` 在四次 Dispatch 之后才设置 `deltaTime`。这些 Dispatch 使用的是此前绑定的值，首帧也没有在 `InitShader()` 中明确初始化它。应在调度前设置当前子步 h。

正式模拟用固定步长累加器，并限制单帧子步数。`h=1/120 秒`可以作为调试起点，但不是稳定性保证。弹簧单自由度的特征频率约为 `sqrt(k/m_eff)`；接触越多、质量越小、刚度越高，通常越需要小步长。单弹簧的稳定界不能直接当作整个接触系统的保证。

### 6.2 地面分支之前读取了负索引

`ComputeParticleForce` 在判断 `otherID < 0` 前就执行 `particleBuffer[otherID]`。传入 -1 时源码存在越界读取风险，应把另一粒子的加载移入有效索引分支，地面使用独立函数。

### 6.3 缺少线程范围检查和零距离保护

四个 Kernel 都直接用 `id.x` 访问数据，CPU 却使用向上取整调度。场景中 100 个刚体以每组 8 线程调度，会启动 104 个线程，最后 4 个需要退出。

粒子完全重合时 `relativePosition / rpMag` 会除零。用距离平方与 epsilon 检查；对于不同粒子的完全重合，可用由有序粒子 ID 对决定的反对称备用法线，让 i→j 与 j→i 的方向相反，避免两端同时被推向同一方向。

### 6.4 重力随采样数量变化

老师对每个粒子的 force 都减去 9.8，汇总后再除以刚体质量。因此重力加速度数值变为 `9.8 × S / M`，还会乘相应缩放参数。场景 S=64、M=1 时为 627.2 个坐标单位/秒²。

修正版在刚体积分阶段直接加 `g`，或每粒子加 `(M/S)g`，两种方式择一，不能重复。改变每边粒子数后，自由落体加速度不应改变。

### 6.5 同刚体粒子与资源读写

原版只跳过自身，没有过滤同一刚体的其他粒子。理想布局中相邻小球恰好接触，无需内部排斥；浮点误差却可能触发多余内部力。增加 BodyID 并过滤同刚体配对。

原版还从同一 RW 结构化缓冲读取粒子结构、同时写入其中的 force 字段。移植时用只读粒子运动快照和独立 ForceBuffer，让跨线程读写关系清晰，不依赖编译器是否消除了未用字段加载。

### 6.6 O(P²) 才是大规模瓶颈

每个粒子遍历全部粒子。场景 100×64=6400 个粒子，一步约 4096 万次候选配对；脚本默认 1000×64=64000，一步约 40.96 亿次。这里只是数量级估算，尚未计入跳过项，也不是性能实测。

GPU 并行不会消除平方增长。空间网格是扩大规模前的重要步骤。相同直径的小球可先用 cellSize=d 的均匀网格，检查本格及周围 26 格；高密度单元依然可能退化，必须处理容量溢出。

### 6.7 调试显示与剔除

原版在刚体积分后绘制刚体，但粒子位置仍是步首重建的值，所以 Debug 小球和最新方块存在一步相位差。UE Debug 显示前应重建最终粒子位置。

原版绘制 Bounds 固定在原点且大小只有 edgeLength，无法代表散布的大群体。UE 的 Niagara Bounds 或自定义组件 Bounds 也必须覆盖模拟区域，否则镜头移动时可能整群消失。

## 7. Niagara 是必需的吗？“最先进”应该怎样理解

**不是必需。对于你要学习 Compute Shader 原理、同时熟悉 UE 的目标，我建议：C++ Global Shader + RDG 求解器，后接 Niagara GPU Mesh Renderer。** 这是针对本项目的工程判断，不是“唯一最先进”的官方排名。

RDG 负责声明计算 Pass 及资源依赖，管理相关资源转换和生命周期；Global Shader 承载独立计算入口。它适合把老师四阶段逻辑保留下来，并让每阶段的输入输出可检查。[Epic RDG 文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)

Niagara 提供粒子系统和 Mesh Renderer；GPU Simulation Stages 能安排额外模拟阶段。因此，纯 Niagara 也能实现自定义模拟，但仍需要你处理刚体—粒子索引、接触、归约和阶段依赖，普通 Collision 模块不会自动复现本章算法。[Simulation Stage API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraSimulationStageGeneric)、[Niagara 渲染模块](https://dev.epicgames.com/documentation/unreal-engine/render-module-reference-for-niagara-effects-in-unreal-engine?lang=en-US)

建议分清三条路线：

- **本章学习主线**：手写 GPU 求解器，学习受力、积分、缓冲和并行；Niagara 负责可视化。
- **以特效美术为主**：纯 Niagara GPU + Scratch Pad / Simulation Stages，接受需要自己搭建数据模型和调试工具。
- **以游戏刚体交互为主**：优先评估 Chaos，获得引擎物理体系的能力；这与从头复现课程算法是不同学习目标。[Epic 物理系统概览](https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-in-unreal-engine)

Niagara、RDG、XPBD 不是同层替代品：前两者分别提供效果框架与 GPU 调度基础，XPBD 是约束求解方法。XPBD 可作为后续布料、软体或刚体约束研究方向，但直接替换本章会改变所学模型；它也不是最新才出现的技术，原论文发表于 2016 年。[XPBD 原论文](https://matthias-research.github.io/pages/publications/XPBD.pdf)

对于布料成品，Chaos Cloth 的 Panel 工作流支持 XPBD 约束，但这并不意味着本章方块必须改成布料系统。[Epic Panel Cloth 概览](https://dev.epicgames.com/documentation/unreal-engine/panel-cloth-editor-overview)

## 8. UE 版建议架构

```text
Game Thread：Actor 参数、固定步长计划、初始化/重置命令
  ↓ 复制参数，提交渲染线程命令
Render Thread：持久模拟状态
  ↓ RDG：重建粒子 → 接触力 → 合力/力矩 → 刚体积分
  ↓ 发布已完成的显示快照
自定义 Niagara Data Interface → GPU 粒子属性 → Mesh Renderer
```

每个 Niagara 显示粒子对应一个刚体，而不是一个碰撞采样点。Debug Emitter 才对应全部采样点。Niagara 只读取位置、朝向、速度，不再叠加重力或执行第二套速度积分。

自定义 Data Interface 是让 Niagara Shader 访问外部模拟数据的桥梁；C++ UObject 负责配置，渲染线程 Proxy 负责对应 GPU 数据与参数绑定。它不是“把 Buffer 拖到 Niagara 参数面板”就能完成的事情。[UNiagaraDataInterface API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraDataInterface)

本机 5.8 的 `NiagaraDataInterface.h` 已确认存在 `SetShaderParameters`、`GetFunctionsInternal` 和 HLSL 生成入口，且旧版 `GetFunctions` 已标记弃用。实施时参考本机同版本 Data Interface 实现，不照抄旧教程签名。

**同步是独立验收项。** Actor Tick 先运行，并不等于 GPU 模拟一定在 Niagara 读取之前完成。先设计只读已完成快照、允许固定一帧显示延迟的方案，明确生产/消费顺序和资源转换；需要无延迟时，再接入正确的 Niagara GPU 调度时点。跨独立 RDG 图的外部资源关系不会仅凭相同指针就自动完整解决。

项目已有 [CubeFlockActor.cpp](E:/UEProjects/Playground02/Plugins/CubeFlock/Source/CubeFlock/Private/CubeFlockActor.cpp)，可以参考其中 RDG Buffer 注册、结果提取及调度模式，但现有鸟群算法不能直接当作接触求解器。推荐新建 MassSpring 插件，保留已有示例。

## 9. 单位和坐标：建议从第一天固定

建议模拟内部使用米、千克、秒和 Z 向上：`g=(0,0,-9.81)`；显示时位置乘 100 转成 UE 厘米，速度同样乘 100。质量为 kg，弹簧系数为 N/m，阻尼为 N·s/m，惯量为 kg·m²。

这样不用同时把力、力矩和刚度转换到厘米体系。模拟原点先固定在 Actor 初始位置，旋转为零、缩放为一；后续明确实现移动模拟原点和大世界坐标处理。

无需直接导入 Unity 随机四元数。UE 中重新初始化方向更简单。若确实导入已有姿态，必须用一致基变换转换旋转，例如 `R_UE = C R_Unity C⁻¹`，不要仅交换位置轴、保留原四元数。

## 10. 学完应能独立回答的问题

1. 为什么 100 个方块可能需要 6400 个碰撞粒子，却只需 100 个可见方块实例？
2. 为什么粒子碰撞力没有让方块变形？
3. 为什么角速度要通过叉积贡献到接触点速度？
4. 为什么偏心碰撞产生力矩，对称碰撞不应凭空旋转？
5. 为什么增加粒子密度不该改变自由落体加速度，却可能改变接触刚度？
6. 为什么读写分离和 Pass 顺序比简单增加线程数更重要？
7. 为什么先做两个方块的正确性，再做网格优化，比直接跑 1000 个更容易定位问题？

接下来按 [逐步实施 Checklist](E:/UEProjects/Playground02/Docs/Chapter5_2_MassSpring_UE_TODO.md) 执行。先完成小规模正确版本，再扩展性能与 Niagara 集成。
