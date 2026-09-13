# Chapter5_2 Mass-spring：UE 5.8 逐步实施 Checklist

> 配套阅读：[课程详解](E:/UEProjects/Playground02/Docs/Chapter5_2_MassSpring_课程详解.md)。新手版更新：2026-09-13。面向熟悉 Unity、刚开始使用 UE 和 Rider 的读者。保留你已经勾选的阶段 0；这些勾选是你的进度，不代表本文对关卡效果进行了运行验证。本次只修改文档，文中代码是供你随后创建的教学示例，尚未在此项目编译验证。

## 先读：你现在从哪里开始

上一版把“创建模块、写 Global Shader、接 Data Interface”压成几句话，对初学者跨度太大。这一版先带你完成 **Rider 打开项目 → 插件加载 → 创建 Actor → 在关卡里看到线框方块 → 注册一个 Compute Shader**，再进入模拟算法。

不要一口气把所有代码贴进去。每次只做到一个验收点，编译通过、运行结果正确后再继续。

2026-09-13 检查到的项目状态：

- 已有 `Plugins/MassSpring`，是空白 Runtime 插件骨架，不用再创建一个同名插件。
- 模块入口是 `Public/MassSpring.h` 与 `Private/MassSpring.cpp`。本文沿用它们，**不要额外创建第二份模块入口 `MassSpringModule.cpp`**。
- `MassSpring.uplugin` 当前加载阶段为 `Default`，需要在阶段 1 修改为 `PostConfigInit`。
- 已有 `Content/MassSpringDemo/Lvl_MassSpring.umap`。只核实文件存在，没有核实灯光和地面设置。
- `Playground02.uproject` 的 Plugins 数组出现两条 `MassSpring`，以及一条 `NewPlugin`。项目的 Plugins 目录目前只有 CubeFlock 和 MassSpring；NewPlugin 是否安装在引擎其他目录还未核实。按准备步骤检查，不要直接覆盖整个 `.uproject`。

**今天建议先做到阶段 1C：让自己写的 Actor 出现在 UE，点击 Play 能画出线框方块。** 阶段 1D 以后开始进入渲染编程；空间网格、Niagara 自定义数据接口留到后面。

## 准备 A：把熟悉的 Unity 概念对应过来

这里只建立够用的近似对应，不表示两引擎完全一样：

- Unity Scene → UE Level / Map，关卡文件是 `.umap`。
- Unity Hierarchy → UE World Outliner / 世界大纲，查看关卡中的对象。
- Unity Inspector → UE Details / 细节面板，修改选中对象的参数。
- Unity Project 窗口 → UE Content Browser / 内容浏览器；底部临时展开的是 Content Drawer / 内容抽屉。
- Unity GameObject → UE Actor，大致都是场景对象；UE Actor 的变换通常由 Root Component 提供。
- Unity Component / MonoBehaviour → UE ActorComponent 或自定义 Actor。本教程用一个 Actor 管理整套实验。
- Unity Prefab → UE Blueprint Class 可承担类似的可复用对象配置工作，但 Blueprint 还可以写可视化逻辑。
- Unity `Start()` → 本例使用 UE `BeginPlay()`；`Update()` → `Tick(float DeltaSeconds)`。
- Unity `[SerializeField]` → UE 常见 `UPROPERTY(EditAnywhere, ...)`，让成员出现在反射和编辑器系统中。
- Unity `Debug.Log` → UE `UE_LOG`；`Gizmos` 的部分调试用途 → `DrawDebugBox` 等函数。
- Unity `.compute` → UE `.usf` 里写 Compute Shader 入口，另有 C++ 注册和调度代码。

最重要的区别：**UE 的 C++ 类不是像 C# 脚本一样，保存后就自动编译成可挂载的新组件。** 首先需要构建成功，让 UE 加载对应模块，类才会在编辑器中可用。

文件后缀先认识这些：

- `.h`：声明类有哪些成员和函数，通常放接口。
- `.cpp`：写函数实现；一对 `.h/.cpp` 可以共同表示一个类。
- `.generated.h`：Unreal Header Tool（UHT）生成的反射辅助代码，你只写 include，不自己创建这个文件。
- `.Build.cs`：虽然用 C# 写，但它是 Unreal Build Tool（UBT）的模块构建规则，不是游戏脚本。
- `.uplugin` / `.uproject`：JSON 格式的插件/项目描述，不要有重复对象或漏逗号。
- `.usf`：Shader 主文件；`.ush`：多个 Shader 共享的 HLSL 声明/函数。
- `.uasset`：材质、Blueprint、Niagara System 等二进制资产，用 UE 编辑，不在 Rider 里写文本。

## 准备 B：第一次正确打开 Rider 工程

### B1. 使用项目入口，不是随便打开一个文件夹

- [x] 在 UE 中保存当前关卡和资产，然后退出编辑器。
- [x] 打开 Rider，在欢迎页选择 Open，或在菜单 File → Open 中选择 [Playground02.uproject](E:/UEProjects/Playground02/Playground02.uproject)。
- [x] 等待 Rider 完成项目加载和索引。首次处理 UE 的大量头文件可能较慢，先不要根据暂时红线判断代码坏了。
- [x] 在左侧项目窗口找到 Playground02 和 Plugins。树可能是逻辑视图，不完全等同资源管理器；找不到文件可使用 Navigate → Go to File，输入文件名。
- [x] 能打开 [MassSpring.Build.cs](E:/UEProjects/Playground02/Plugins/MassSpring/Source/MassSpring/MassSpring.Build.cs) 和 [MassSpring.cpp](E:/UEProjects/Playground02/Plugins/MassSpring/Source/MassSpring/Private/MassSpring.cpp)。

Rider 支持直接打开 `.uproject`，通常无需先生成 `.sln`。如果你一直用现有 `.sln` 且项目正常，也不用为了跟教程重复建项目；新加模块后注意重新加载工程模型。[JetBrains：项目打开方式](https://www.jetbrains.com/help/rider/Unreal_Engine__Before_You_Start.html)

### B2. Rider 与编译工具是两件事

Rider 负责编辑、导航和启动构建，Windows C++ 工具链负责实际编译。装了 Rider 不等于已经有 MSVC 和 Windows SDK。先尝试下面的基线构建；只有日志明确提示缺工具时，再通过 Visual Studio Installer / Build Tools 安装当前 UE 5.8 要求的 C++ 组件，不要照抄旧版教程的固定工具链版本。[JetBrains：构建工具要求](https://www.jetbrains.com/help/rider/Unreal_Engine__Before_You_Start.html)

- [x] 在 Rider 顶部选择 Playground02 的 **Editor** 构建目标，配置为 **Development**、平台为 **Win64**。界面可能把它合并显示为 Development Editor。
- [x] 不要选 Shipping，也不要选择 UnrealEditor 引擎工程做全量引擎构建。
- [x] 使用 Build 菜单中针对项目的构建操作。找不到按钮时，直接在 Rider 底部 Terminal 中运行本文阶段 1 的 Build.bat 命令，Shell 选 PowerShell。
- [x] 在构建输出中找到成功信息；失败时记录第一条实际 `error` 及附近上下文，最后的 exit code 只是结果。

当前项目存在待核对插件记录，所以基线构建可能在进入 C++ 编译前就报插件错误。遇到这种情况，先做阶段 1A 的配置检查，再重试。

### B3. UE 中设置 Rider，RiderLink 可以后装

- [x] 在 UE 的 Edit → Editor Preferences（编辑器偏好设置）中搜索 Source Code，选择 Rider 为源码编辑器。中文翻译可能略有差异，以英文搜索词定位。
- [x] 若 Rider 提示安装 RiderLink，可选择 Game（只安装到当前项目）；也可以先跳过，优先确认普通 C++ 编译成功。
- [x] 理解 UnrealLink 是 Rider 侧集成，RiderLink 是 UE 侧插件。它们提供日志、Blueprint 导航等便利，不是自定义 Actor 编译成功的必要条件。[JetBrains：UnrealLink / RiderLink](https://www.jetbrains.com/help/rider/Unreal_Engine__UnrealLink_RiderLink.html)

### B4. 本教程固定使用这个修改循环

```text
UE 保存关卡 → 停止 Play → 退出 UE 编辑器
→ Rider 修改并保存代码 → 构建 Playground02Editor
→ 构建成功后重新打开 UE → 打开实验关卡 → Play → 检查日志和画面
```

初期不要混用 Live Coding、Hot Reload 和关编辑器构建。Live Coding 适合部分函数体迭代，但本教程会改模块、反射类和 Shader 注册，统一冷启动更容易确认真实结果。[JetBrains：UE 调试与 Live Coding](https://www.jetbrains.com.cn/en-us/help/rider/Unreal_Engine__Debugger.html)

## 准备 C：在 Rider 创建文件、看错误、打断点

- [x] 新建目录/文件时，在项目树的对应物理目录上右键，使用 New / Add 菜单；菜单因 Rider 版本和视图不同会变化。
- [x] 如果树不显示 Shaders 文件夹，用 Windows 资源管理器在本文给定路径建目录和文件，再用 Rider Open 打开。打开 Windows“显示文件扩展名”，避免生成 `.usf.txt`。
- [x] 如果使用 Rider 的 Unreal Class 向导，模块一定选 **MassSpring**。自动生成模板若与本文重名，修改已有文件，不重复创建。
- [x] 修改 `.Build.cs`、`.uplugin` 后接受 Rider 的项目重新加载提示；没有提示且树未更新时重新打开 `.uproject`。
- [x] 学会三种定位操作：Go to File 找文件；Go to Declaration 找声明；Find Usages 找调用。可以从菜单选择，快捷键取决于你的 Keymap。
- [x] C++ 报错以 Build 输出为准。找不到 `.generated.h` 时先编译，让 UHT 生成它；仍失败就先处理 UHT 报出的原始错误。
- [x] 调试 Actor 时，用 Rider 的 Debug 启动该 Editor 目标；或者在 UE 已运行时使用 Attach to Process，选择打开本项目的 UnrealEditor 进程。
- [x] 在 `BeginPlay()` 内点击行号旁放断点，再点击 UE Play；停住后观察变量和 Call Stack，之后 Resume 继续运行。

**注意：Rider 的普通 C++ 断点不能进入 GPU 上运行的 `.usf`。** 调试 Shader 先用小规模数据读回、诊断颜色/位置、计数 Buffer，后续再学 GPU 捕获工具。

**准备部分验收：** 能找到 MassSpring 源码，知道 Editor 构建与 Play 的区别，能找到第一条构建错误以及 UE 的 Output Log。

## 目标与执行规则

做出一个 GPU 方块刚体模拟：用固定小球采样刚体，计算接触弹簧力、阻尼及力矩，更新位姿，最后由 Niagara 显示。**第一版不做布料、不做软体、不追求完整游戏物理引擎。**

推荐主线是 **C++ Global Shader + RDG + GPU 常驻状态 → 自定义 Niagara Data Interface → GPU Mesh Renderer**。Niagara 不是模拟必需项；把显示桥接放到算法正确之后，方便定位问题。

按阶段推进，每一阶段通过“验收”后再继续。下面的参数是建议实验起点，不是老师数值，也不是经过本机实测的稳定预设。

## 阶段 0：明确模型与建立独立实验区

你已勾选下面各项，可以直接继续阶段 1。若需要核对关卡：在 Content Browser 打开 `Content/MassSpringDemo/Lvl_MassSpring`；`/Game/MassSpringDemo` 是它的 UE 资产路径，不是磁盘根目录下的 Game 文件夹。

空白关卡若没有可视地面，可以从 Place Actors / 放置 Actor 搜索 Plane，设 Location Z=0；加 Directional Light / Sky Light，或暂时用 Unlit 视图检查几何体。用 Plane 避免把 Cube 中心 Z=0 误当成地面顶面 Z=0。

视口常用操作：右键按住配合 WASD 移动镜头，选中对象按 F 聚焦，W/E/R 切换移动/旋转/缩放，Details 直接输入 Transform。Play 时若被现有游戏角色控制影响观察，可以使用 Play 下拉中的 Simulate；需要回到编辑器操作时停止运行。

- [x] 阅读详解第 1～4 节，能够口述“粒子计算接触，刚体积分运动”。
- [x] 打开老师 `Chapter5_2.compute`，标出四个 Kernel、`ComputeParticleForce` 和四元数乘法。
- [x] 记录原场景配置：100 个刚体，每边 4 粒子，总共 6400 粒子；与脚本默认值区分。
- [x] 确认使用 `Playground02.uproject` 对应的 UE 5.8；启动前保存现有工作。
- [x] 新建独立关卡 `/Game/MassSpringDemo/Lvl_MassSpring`，放相机、灯光及 Z=0 的可视地面。
- [x] 约定模拟内部单位为 m、kg、s，Z 向上；显示位置和速度乘 100。
- [x] 约定初版 Actor 旋转为零、缩放为一，运行时不移动模拟原点。
- [x] 约定固定随机种子，最初不用随机重叠初始化。

**验收：** 能明确说出：地面模型是自己写的平面接触，不是场景碰撞组件自动提供；方块不会发生形变。

## 阶段 1：建立 Global Shader 插件骨架

按你已经创建的插件继续。下面的 Actor、Simulation 和 Shader 文件将逐步增加，别一次创建一堆空文件后直接跳到模拟：

```text
Plugins/MassSpring/
  MassSpring.uplugin
  Shaders/Private/MassSpringCommon.ush
  Shaders/Private/MassSpring.usf
  Source/MassSpring/MassSpring.Build.cs
  Source/MassSpring/Public/MassSpringActor.h
  Source/MassSpring/Public/MassSpring.h
  Source/MassSpring/Private/MassSpringActor.cpp
  Source/MassSpring/Private/MassSpring.cpp
  Source/MassSpring/Private/MassSpringSimulation.h
  Source/MassSpring/Private/MassSpringSimulation.cpp
  Source/MassSpring/Private/MassSpringShaders.cpp
```

### 1A. 修改插件描述与依赖，先保证模块能加载

**在哪里做：Rider；UE 编辑器保持关闭。**

1. 打开 [MassSpring.uplugin](E:/UEProjects/Playground02/Plugins/MassSpring/MassSpring.uplugin)。在 Modules 中找到 Name 为 MassSpring 的对象，只把 `"LoadingPhase": "Default"` 改为 `"LoadingPhase": "PostConfigInit"`。其他元数据不用重写。
2. 保留已有 `CanContainContent=true`，这允许以后在插件目录存资产；本教程关卡仍放项目 Content 中。
3. 打开 [Playground02.uproject](E:/UEProjects/Playground02/Playground02.uproject)，Plugins 数组内只保留一条 `{"Name":"MassSpring","Enabled":true}`。注意删除多余对象后的逗号。
4. 核对 NewPlugin 是不是此前误创建的练习插件。若不需要它，就移除它在 Plugins 数组中的启用对象；若确实依赖它，先找回对应插件，不要用删除记录掩盖依赖缺失。这里只调整描述记录，不删除插件目录。
5. 打开 [MassSpring.Build.cs](E:/UEProjects/Playground02/Plugins/MassSpring/Source/MassSpring/MassSpring.Build.cs)。当前是空白模板，可整理成下面的内容。将来若你已经添加其他用途的依赖，应保留实际用到的部分。

```csharp
using UnrealBuildTool;

public class MassSpring : ModuleRules
{
    public MassSpring(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Projects", "RenderCore", "RHI"
        });
    }
}
```

`Engine` 支持 Actor 等类型；`Projects` 用来查询插件路径；`RenderCore`、`RHI` 提供后续渲染接口。Public/Private 依赖分别服务对外头文件和内部实现；它们不是 Unity 的 public/private 字段访问权限。

- [x] 配置检查完成，MassSpring 启用记录只有一条。
- [x] 修改构建规则后，执行一次 Editor 构建，确认没有插件查找或依赖错误。

### 1B. 注册 Shader 目录映射

**在哪里做：Rider + 资源管理器。目的：让 UE 能从虚拟 Shader 路径找到磁盘文件。**

1. 建立磁盘目录 `E:/UEProjects/Playground02/Plugins/MassSpring/Shaders/Private`。注册映射前 `Shaders` 目录必须存在。
2. 保留现有 `Public/MassSpring.h`，它已声明 StartupModule 和 ShutdownModule。
3. 在现有 `Private/MassSpring.cpp` 中加入包含文件并实现启动函数。空白模板可改成下面这样：

```cpp
#include "MassSpring.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

void FMassSpringModule::StartupModule()
{
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("MassSpring"));
    check(Plugin.IsValid());
    const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/Plugin/MassSpring"), ShaderDir);
    UE_LOG(LogTemp, Display, TEXT("MassSpring module loaded. Shaders: %s"), *ShaderDir);
}

void FMassSpringModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FMassSpringModule, MassSpring)
```

4. 构建成功后打开 UE，在 Window 菜单寻找 Output Log（输出日志，可能位于 Developer Tools 子菜单），搜索 `MassSpring module loaded`。也可搜索磁盘日志 `E:/UEProjects/Playground02/Saved/Logs/Playground02.log`。

以后 C++ 里的 `/Plugin/MassSpring/Private/MassSpring.usf` 将对应磁盘 `Plugins/MassSpring/Shaders/Private/MassSpring.usf`，不需要把磁盘 E 盘路径写死到 Shader 注册宏里。

- [ ] Output Log 中能看到模块加载日志及正确的 Shaders 路径。

**常见错误：** 两个 cpp 都写 `IMPLEMENT_MODULE` 会重复定义模块；只有修改 `.uplugin` 没重启，不能验证新的加载阶段。Global Shader 需要早期注册的依据见 [Epic Global Shader 指南](https://dev.epicgames.com/documentation/unreal-engine/adding-global-shaders-to-unreal-engine)。

**早期模块不要在 Actor 构造函数加载 Niagara 资产。** UE 启动时会构造类默认对象（CDO），即使关卡没有放这个 Actor，其构造函数也可能运行。若在这里用 `ConstructorHelpers::FObjectFinder<UNiagaraSystem>` 同步加载资源，可能遇到 `Tried to get module interface for unloaded module: 'Niagara'`。本项目 CubeFlock 曾触发这个问题：保留 Shader 模块的早期加载，将默认 Niagara 资产保存为 `TSoftObjectPtr`，在 BeginPlay 等正常运行阶段再加载。不要把“Plugins 中启用了 Niagara”理解成“任何启动阶段都已经可以加载 Niagara 资产”。后续规模扩大时可将早期 Shader 注册与正常阶段的玩法/Niagara 集成拆成不同模块。

### 1C. 创建第一个 Actor：先画一个线框方块

**这段是 CPU 入门练习，不是最终 GPU 模拟。** 它让你先练会“写类 → 编译 → 放进关卡 → 调参数 → Play”。

在 `Plugins/MassSpring/Source/MassSpring/Public` 新建 `MassSpringActor.h`：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassSpringActor.generated.h"

UCLASS()
class MASSSPRING_API AMassSpringActor : public AActor
{
    GENERATED_BODY()

public:
    AMassSpringActor();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category = "MassSpring|Debug", meta = (ClampMin = "0.01"))
    float EdgeLengthMeters = 1.0f;

    UPROPERTY(EditAnywhere, Category = "MassSpring|Debug")
    FVector CenterMeters = FVector(0.0, 0.0, 2.0);

protected:
    virtual void BeginPlay() override;
};
```

再在 `Plugins/MassSpring/Source/MassSpring/Private` 新建 `MassSpringActor.cpp`：

```cpp
#include "MassSpringActor.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"

AMassSpringActor::AMassSpringActor()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AMassSpringActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Display, TEXT("MassSpring BeginPlay: %s"), *GetName());
}

void AMassSpringActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const FVector CenterCm = GetActorLocation() + CenterMeters * 100.0;
    const FVector HalfExtentCm(FMath::Max(EdgeLengthMeters, 0.01f) * 50.0f);
    DrawDebugBox(GetWorld(), CenterCm, HalfExtentCm, FColor::Green,
                 false, -1.0f, 0, 2.0f);
}
```

逐项理解：

- `A` 前缀表示 Actor 类型；编辑器里名字通常显示为 MassSpringActor 或分词后的 Mass Spring Actor。
- `MASSSPRING_API` 来自模块名 MassSpring，是导出宏；不是 `PLAYGROUND02_API`。
- `GENERATED_BODY()` 让 UHT 插入反射代码；`.generated.h` 必须是这个头文件最后一个 include。
- 构造函数只创建默认组件和默认值，不开始逐帧模拟；`BeginPlay` 在 Play 时进入，`Tick` 每帧调用。
- `UPROPERTY` 让 Details 显示两个调试参数。这里 `FVector` 是 CPU 参数，后续上传 GPU 要转为 float 类型。
- 1 m 边长变成 100 cm，DrawDebugBox 要的是半尺寸，所以乘 50。
- Actor 世界位置作为模拟原点；本练习只处理平移，按约定保持旋转零、缩放一。

接下来回到 UE：

1. 退出 UE 后构建 Editor，成功后重新打开 `Lvl_MassSpring`。
2. Content Browser 的设置中启用 Show C++ Classes；插件类可能还需要 Show Plugin Content。尝试在 C++ Classes / MassSpring 下找到类；也可以在 Place Actors 搜索类名。
3. 把这个 **Actor 类**拖到关卡。不是把 `.cpp` 文件拖进去，也不是在已有 Cube 上 Add Component。
4. 选中它，在 Details 设 Actor Location=(0,0,0)、Rotation=(0,0,0)、Scale=(1,1,1)。
5. 在 MassSpring / Debug 分类中确认 EdgeLengthMeters=1、CenterMeters=(0,0,2)。
6. 点击 Play 或 Simulate，应在原点上方 200 cm 看到边长 100 cm 的绿色线框；尚未 Play 时没有线框是正常的，因为它只在 Tick 中绘制。
7. Output Log 搜索 `MassSpring BeginPlay`。停止 Play 后修改 EdgeLengthMeters=2，再运行，线框应变大。
8. 保存关卡。Play 中修改的实例值一般不会自动保存为编辑状态，想保留参数就在停止后再设置。

- [x] 类能在编辑器中找到，能拖进关卡。
- [x] Details 中有两个调试参数，BeginPlay 日志只在开始运行时出现。
- [x] Play 能显示并调整线框方块。

**若类找不到：** 先看构建是否真正成功和插件是否加载；检查文件是否误建到游戏模块或其他目录；最后再考虑 Rider 工程刷新。不要自己创建 generated.h，也不要把删除 Binaries/Intermediate 当成第一步。

### 1D. 注册一个最小 Compute Shader：先验证编译

完成 1C 后再做这一步。**注册/编译 Shader 与运行 Dispatch 是两件事；这一小步只验证注册和编译。**

在 `Plugins/MassSpring/Shaders/Private/MassSpring.usf` 写：

```hlsl
#include "/Engine/Public/Platform.ush"

RWStructuredBuffer<float4> OutValues;
uint ValueCount;

[numthreads(64, 1, 1)]
void TestCS(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    uint Index = DispatchThreadId.x;
    if (Index >= ValueCount)
        return;
    OutValues[Index] = float4(float(Index), 2.0, 3.0, 1.0);
}
```

在 `Plugins/MassSpring/Source/MassSpring/Private/MassSpringShaders.cpp` 写：

```cpp
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphResources.h"

class FMassSpringTestCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FMassSpringTestCS);
    SHADER_USE_PARAMETER_STRUCT(FMassSpringTestCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(uint32, ValueCount)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float4>, OutValues)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

IMPLEMENT_GLOBAL_SHADER(FMassSpringTestCS,
    "/Plugin/MassSpring/Private/MassSpring.usf", "TestCS", SF_Compute);
```

其中 `OutValues`、`ValueCount` 要与 HLSL 完全同名；`TestCS` 是入口名，对应 Unity 的 kernel 名；`64` 是一组线程数，不是总元素数。数量 9 时也会启动一组 64 个线程，因此必须判断越界。

- [x] 保存文件，关闭 UE，构建 C++，重新启动 UE，等待 Shader 编译完成。
- [x] 没有 Shader 文件找不到、入口找不到或参数类型不匹配错误。
- [x] 记住这时绿色线框仍来自 CPU Actor，不能据此认为 Compute Shader 已执行。

### 1E. 第一次 Dispatch 与读回：需要编写调度代码

这一步不再是菜单操作。需要在 `MassSpringShaders.cpp` 中继续写使用 `FMassSpringTestCS` 的函数，并把公开给 Actor 的函数声明放在同目录的内部头文件，例如 `MassSpringSimulation.h`。

先只做 **9 个 float4 的一次性计算**，不用接完整刚体结构：

1. 在 Actor 的 BeginPlay 发起一次测试请求；不要从 Tick 每帧创建一个新读回对象。
2. 请求通过 `ENQUEUE_RENDER_COMMAND` 交给渲染线程，复制必要参数；不要在游戏线程直接构造并执行渲染图。
3. 在渲染命令中构建 `FRDGBuilder`，创建 stride 为 `sizeof(FVector4f)`、数量为 9 的 Structured Buffer。
4. 从 Graph 分配 `FMassSpringTestCS::FParameters`；填 `ValueCount=9`，给 `OutValues` 绑定该 Buffer 的 UAV。
5. 获取当前 ShaderMap 中的 `FMassSpringTestCS`，调用 `FComputeShaderUtils::AddPass`；GroupCount=(1,1,1)。
6. 添加 Buffer 的异步读回 Copy Pass，再 Execute Graph。读回也是输出消费者，避免生成结果无人使用的 Pass 被裁剪。
7. 将 Readback 对象保留到后续渲染帧，在渲染线程检查 `IsReady()`；未就绪就下次再查，不能忙等。
8. 就绪后 Lock，按长度复制 9 个 float4 到独立 CPU 数组，Unlock；通过线程安全传递把数据交回游戏线程再用于 Actor 显示。
9. 输出第 0、8 个元素，应分别为 `(0,2,3,1)` 和 `(8,2,3,1)`；保留 Readback 和请求状态直到结束或安全取消。

这一节提供的是**明确的实现步骤，不是完整可粘贴的异步读回代码**；对象生命周期和结束 Play 的清理必须一起实现。可以在 Rider 查阅现有 [CubeFlockActor.cpp](E:/UEProjects/Playground02/Plugins/CubeFlock/Source/CubeFlock/Private/CubeFlockActor.cpp)，搜索 `ENQUEUE_RENDER_COMMAND`、`FRDGBuilder`、`FComputeShaderUtils::AddPass` 和 Readback 相关代码，理解每段职责后缩减为测试版。

先认识几个术语：Buffer 是 GPU 数组；SRV 是供 Shader 读取的视图；UAV 是可写视图；Pass 是一次图中操作；Readback 是把 GPU 结果拷回 CPU。创建 Buffer 不等于 Dispatch，Dispatch 结束提交也不等于 CPU 已能读取结果。

- [ ] 一次性测试得到预期 9 个元素。
- [ ] 连续 Play/Stop 五次，无崩溃、悬空读回或不停增加的请求。

**阶段 1 汇总检查（保留原来的进度项）：**

- [ ] 参考 [CubeFlock 插件模块](E:/UEProjects/Playground02/Plugins/CubeFlock/Source/CubeFlock/Private/CubeFlockModule.cpp)，创建独立 Runtime 模块。
- [ ] 将 Shader 模块加载阶段设为 `PostConfigInit`，在模块启动时注册 `/Plugin/MassSpring` 的 Shader 路径映射。
- [ ] 在 `.uproject` 启用 MassSpring 插件；`.uplugin` 需要存放资产时设 `CanContainContent=true`，若资产都放 `/Game` 则无需因此启用。
- [ ] Build.cs 添加实际用到的 Core、CoreUObject、Engine、RenderCore、RHI；使用插件管理器时添加 Projects。只有代码依赖 Renderer 私有/公开接口时再添加相应依赖，避免无理由堆模块。
- [ ] 定义一个最小 `FGlobalShader`，使用 Shader 参数结构、`IMPLEMENT_GLOBAL_SHADER` 与 `FComputeShaderUtils::AddPass`。
- [ ] 最小 Shader 向小型 Buffer 写已知数值；只在调试时用异步 GPU Readback 检查。
- [ ] 修改模块结构、UCLASS 或 Shader 注册后关闭编辑器，完整编译并重新启动；不要依赖 Live Coding 验证首次注册。

本机已有文档记录的编译入口：

```powershell
& 'D:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat' Playground02Editor Win64 Development '-Project=E:\UEProjects\Playground02\Playground02.uproject' -WaitMutex
```

**验收：** 新插件加载、Shader 编译成功，读回固定数据正确；重复 Play/Stop 不崩溃。到此尚不需要 Niagara。

## 阶段 2：确定 GPU 数据布局和所有权

**在哪里做：Rider，主要是 `MassSpringSimulation.h/.cpp`、`MassSpringCommon.ush`。前置条件：阶段 1E 的测试已经实际执行并读回。**

把一整块 Buffer 当作 Unity 的 `NativeArray<Body>` 理解，但它在 GPU 上，CPU 不能直接用下标实时读取。先把“一份数据两侧如何解释”定下来，再写算法。

推荐先写一个独立的内部头文件 `Private/MassSpringTypes.h`，放 CPU 上传布局。以下 Body 布局是新方案示例，不能与老师旧 60 字节布局混用：

```cpp
// 放在头文件中，前面包括 CoreMinimal.h。
struct alignas(16) FMassSpringBodyGPU
{
    FVector4f PositionInvMass;    // xyz: 米；w: 逆质量
    FVector4f Rotation;           // xyzw 单位四元数
    FVector4f LinearVelocity;     // xyz: 米/秒；w: 保留
    FVector4f AngularInvInertia;  // xyz: 弧度/秒；w: 立方体标量逆惯量
    uint32 ParticleStart;
    uint32 ParticleCount;
    uint32 Padding0;
    uint32 Padding1;
};
static_assert(sizeof(FMassSpringBodyGPU) == 80, "GPU body stride mismatch");
```

在 `MassSpringCommon.ush` 中写完全同序的 HLSL 结构：

```hlsl
struct FMassSpringBody
{
    float4 PositionInvMass;
    float4 Rotation;
    float4 LinearVelocity;
    float4 AngularInvInertia;
    uint ParticleStart;
    uint ParticleCount;
    uint Padding0;
    uint Padding1;
};
```

执行顺序：

1. 创建一个 `TArray<FMassSpringBodyGPU>`，先只含一个元素；所有字段都初始化，Rotation 设 `(0,0,0,1)`，不要全零。
2. 设置 PositionInvMass=(0,0,2,1)，质量 1 kg、边长 1 m 对应逆惯量 6，AngularInvInertia=(0,0,0,6)。
3. Shader 先做 Body 输入→输出逐元素复制，不做重力。异步读回，验证字段未错位。
4. 再定义粒子静态结构及动态结构。每新增字段都同步修改 C++ / HLSL 和 stride 检查。
5. 最后才增加 Body A/B 状态切换：A 是本步输入，B 是本步结果。变量交换应在提交顺序和资源生命周期允许的位置进行。

CPU Actor 负责参数；Simulation 渲染状态负责 GPU 资源。不要把所有 RHI 类型暴露为 UPROPERTY，GPU 缓冲也不是可在 Inspector 手动赋值的资产。

- [ ] 定义 Body 数据：位置、四元数、线速度、角速度、质量或逆质量、惯量或逆惯量、粒子起始索引与数量。
- [ ] 定义粒子静态数据：局部坐标、BodyID；定义运动快照：世界位置、世界速度、相对中心偏移。
- [ ] 建立独立 ParticleForceBuffer，接触 Pass 只读取运动快照、只写自己的力槽。
- [ ] 使用 `FVector3f` / `FVector4f` 或显式 float 字段匹配 HLSL；不要直接上传可能为 double 的 `FVector` / `FQuat`。
- [ ] 显式规划 padding，检查 `sizeof` 与 `offsetof`。可以用 float4 分组简化布局，但不把“16 字节对齐”误认为所有 StructuredBuffer 结构的硬性统一步长。
- [ ] 不照抄老师的 60 字节布局后继续增加字段而忘记更新 stride。
- [ ] Body 输入输出双缓冲；资源只在渲染线程创建和替换。参数从游戏线程复制过去，避免渲染命令捕获 Actor 裸指针后长期使用。
- [ ] 跨帧使用池化 Buffer / 外部资源引用；每次 RDG 图重新注册，结果按需提取。不要跨帧保存临时 `FRDGBufferRef`。
- [ ] 所有 Kernel 增加 `if (Index >= Count) return;`；实际数量独立于 Dispatch GroupCount。
- [ ] 处理零刚体数量：不 Dispatch、不分配非法零长度资源，显示为空。

**验收：** 上传一个位置和一个已知四元数后，调试读回各字段相符；数量设为 1、7、9、100 时无越界或 GPU 验证错误。

## 阶段 3：先证明坐标和旋转正确

**在哪里做：Rider 写初始化与 BuildParticles Shader；UE 用阶段 1C 的 Actor 展示调试结果。**

具体操作顺序：

1. 给 Actor 增加 `BodyCount`（初始 1）、`ParticlesPerEdge`（初始 2）、`MassKg`（初始 1）的可编辑属性。新增 UPROPERTY 后冷构建。
2. 在初始化函数里用 x/y/z 三层循环生成局部粒子。n=2、L=1 时，每个坐标分量只能取 -0.25 或 +0.25，共 8 点。
3. 建一个 `BuildParticlesCS` 入口，第一版一线程处理一个粒子；通过 BodyID 读取 Body，计算世界位置、旋转偏移和速度。
4. C++ 为该入口增加独立 Global Shader 类和参数结构。一个 `.usf` 可以注册多个入口，不需要每个入口建一个插件。
5. 一次性运行 BuildParticles 并读回结果；游戏线程将结果转 cm，用 DrawDebugSphere 显示，半径为 `L/n/2*100`。
6. DrawDebugBox 用 Body 的中心和朝向，而不是永远用最早的 CenterMeters。更新后的 Debug 数据应来自同一步快照。

先固定单位四元数，预期 8 个小球围绕中心对称；再换已知旋转。**不要同时开启随机初始旋转、重力和碰撞**，否则难以判断错误来源。

如果只是想先练初始化，可临时在 CPU 算同一组点并画出来，然后与 GPU 读回对比。这是参考实现，最终模拟仍使用 GPU 数据。

- [ ] 初始化 1 个边长 1 m 的方块，质量 1 kg，中心位于 `(0,0,2)`，速度和角速度为零，四元数为单位旋转。
- [ ] 每边先放 2 个粒子，即 8 粒子；计算 d=L/n 与局部位置。
- [ ] 实现 BuildParticles：`offset=Rotate(q,local)`，`position=center+offset`，`velocity=v+cross(ω,offset)`。
- [ ] 小规模调试阶段允许异步读回并用 DrawDebug 显示方块、小球和中心。标注这是延迟调试视图，不用它衡量生产路径性能。
- [ ] 暂不施加力，检查局部坐标和小球尺寸。
- [ ] 设一个已知绕 Z 的 90° 旋转，验证局部 +X 点到达 +Y。
- [ ] 再设世界角速度 `(0,0,1)`，检查 +X 侧采样点的旋转线速度朝 +Y。
- [ ] 增加到每边 4 个粒子，确认 64 粒子，方块外观尺寸不变。

**验收：** 所有粒子与中心的距离保持正确；旋转位置和旋转速度方向一致。若失败，先修旋转约定，不开始碰撞。

## 阶段 4：固定步长与自由落体

**在哪里做：Actor Tick 计划子步；Simulation 在渲染线程按计划 Dispatch；IntegrateBodiesCS 更新 GPU 状态。**

这里没有一个勾选项叫“让 UE 自动按你的 h 调用 Shader”。需要自己维护累加器。固定子步的 CPU 逻辑可按下面的伪代码实现：

```text
若暂停：返回（不累积 DeltaSeconds）
Accumulator += DeltaSeconds
若 Accumulator > h * MaxSubsteps：
    记录超出部分；Accumulator = h * MaxSubsteps
Steps = 0
当 Accumulator >= h 且 Steps < MaxSubsteps：
    Steps += 1
    Accumulator -= h
复制 h、Steps 及参数，提交一次渲染线程请求
渲染线程在请求中顺序执行 Steps 次子步
```

这段只用于说明控制流，`Accumulator`、`Steps` 需要在你的 C++ 中声明和初始化。输入中的 h 必须大于零，MaxSubsteps 至少为 1。

1. 先只改 IntegrateBodies，完全不调用 Contacts。给方块 2 m 高度，验证它沿 -Z 落下。
2. 暂停/单步可以先做简单编辑器按钮：声明 `UFUNCTION(CallInEditor, Category="MassSpring|Debug")` 的无参函数，在 `.cpp` 实现。按钮仅修改暂停标记/请求计数，真正 GPU 更新仍走统一调度。
3. 这些按钮名字如 ResetSimulation、StepOnce 是你自己添加的，不是 UE 自动提供的 MassSpring 功能。运行时应选中正在模拟的 Actor 实例；不在运行世界时可以直接拒绝并写日志。
4. 重置时增加状态版本号，清掉旧读回/显示结果，避免上一轮异步结果晚到后覆盖新状态。
5. 固定执行 120 步（1 秒），初始 z=2、v=0 时，半隐式欧拉应有 vz=-9.81、z≈-2.945875 m。此时没开地面，落到地面以下正常。
6. 用 Output Log 的模拟步数和位置做判断，别只凭屏幕中的降落速度。

若想限帧比较，可在 UE 控制台输入 `t.MaxFPS 30`、`t.MaxFPS 60` 等，结束后用 `t.MaxFPS 0` 取消上限；实际帧率未达到目标时，以记录到的模拟步数为准。

- [ ] 在 CPU 累加渲染帧时间，每当 accumulator≥h，安排一次固定模拟子步并减去 h。
- [ ] 起点设 h=1/120 s、每渲染帧最多 8 子步；明确卡顿时的积压处理策略，例如限制 accumulator 并记录丢弃的模拟时间，避免无限追帧。
- [ ] 在每个子步 Dispatch 前绑定 h。暂停不积累时间；单步按钮只推进一次 h。
- [ ] Implement IntegrateBodies：无接触时 `v += g*h`、`x += v*h`，g=(0,0,-9.81)。
- [ ] 对静态刚体使用 invMass=0，并保持位姿、速度不变；动态刚体质量必须为正。
- [ ] 实现重置、暂停、单步；重置同时清空累加器、速度、历史显示状态。
- [ ] 记录模拟时间，而不是用墙钟时间评估自由落体。
- [ ] 对同一模拟时间比较 30、60、120 FPS 下结果。
- [ ] 每边粒子数从 2 改为 4，验证落体加速度不变。

可用离散结果直接验算：由静止开始、常量重力、执行 N 步时，半隐式欧拉应得到 `v=Ngh`、`x=x0+g h² N(N+1)/2`。它与连续解析解存在离散误差，不能强求两者完全一致。

**验收：** GPU 结果与上述离散公式在浮点容差内一致；帧率变化不改变同一子步数的状态。超过子步上限时，报告时间丢弃而非宣称仍保持实时一致。

## 阶段 5：单个方块与地面接触

**在哪里做：新增 ComputeContactsCS、ReduceBodyForcesCS；扩展 IntegrateBodiesCS。UE 地面网格只负责让你看见 Z=0。**

按下面顺序连接，不要只在 HLSL 里新增函数却忘了 C++ 调度：

1. BuildParticles 输出本步位置、速度和旋转偏移。
2. Contacts 给每个粒子算地面力，写 ForceBuffer；没接触的粒子也写零。
3. Reduce 每个刚体遍历自己的粒子，输出合力 F 和合力矩 τ。
4. Integrate 读取这些结果更新速度和位姿，最后交换 Body A/B。

先把阻尼和切向力都设为零，暂停单步检查**粒子压入地面时法向力 Z 是否大于零**。确认力方向后加阻尼，再看是否衰减。把诊断信息限制为少量粒子，避免每帧输出几千行日志。

地面受力示意（法线为向上的 N，点速度为 vp）：

```text
depth = radius - dot(p - planePoint, N)
F = (0,0,0)
若 depth > 0：
    magnitude = max(0, k * depth - normalDamping * dot(vp, N))
    F = magnitude * N
```

初期不要给可见方块启用 Simulate Physics 来“帮它碰撞”，否则会引入第二套运动来源。你的 GPU Buffer 中的姿态不会自动与 Chaos 相互同步。

- [ ] 增加解析平面：N=(0,0,1)、p0=(0,0,0)，每粒子穿透量为 `r-dot(p-p0,N)`。
- [ ] 实现非负法向弹簧阻尼力；先把切向力关闭。
- [ ] 地面计算独立函数，不使用 `ParticleBuffer[-1]`。
- [ ] 每子步重写 ParticleForceBuffer，避免累加上一子步的残留力。
- [ ] 按刚体汇总 `ΣF` 和 `Σ(r×F)`；重力只在刚体阶段添加一次。
- [ ] 使用立方体惯量 `I=ML²/6` 更新角速度，移除凭经验代替惯量的 AngularForceScalar。
- [ ] 采用四元数积分并归一化，处理无效输入与零长度四元数初始化。
- [ ] 先从低高度、轴对齐、零初速落下，再测试倾斜落下。
- [ ] 暴露 k、法向阻尼 c_n、h；每次只改一个，记录最大穿透、反弹高度和能量变化趋势。

参数实验可从 M=1 kg、L=1 m、每边 2 粒子、每接触 k=1000 N/m、c_n=10 N·s/m、h=1/120 s 开始。**这组数值未实测，若不稳定先减小 h 或 k。** 接触刚度是每采样点的，多个采样点同时接触会叠加，不能把单接触参数当作整个方块参数。

**验收：** 轴对齐方块产生向上支撑，倾斜方块出现合理转动；持续运行 30 秒无 NaN、无限加速或穿过地面。允许有限罚力穿透，不把视觉上完全不穿透当作唯一标准。

## 阶段 6：两个刚体相互碰撞

**在哪里做：扩展 Contacts Shader 和初始化场景，其他显示方式保持不变。**

推荐第一组明确输入：每个方块 1 m、1 kg；中心分别 `(-0.6,0,2)`、`(0.6,0,2)`；速度分别 `(1,0,0)`、`(-1,0,0)`；朝向为单位四元数，关闭重力和地面，两者先逐渐靠近。

1. Contacts 在线性循环中遍历其他粒子，过滤自己和同 BodyID。
2. 两粒子都从本步不变的快照读位置和速度，每个线程仅写自己的 ForceBuffer 元素。
3. 用对称初始状态检查 X 向排斥；记录两个刚体总线动量。
4. 再把第二个方块 Y 增加 0.2 m，观察偏心接触是否产生转动。
5. 最后再加法向阻尼、切向耗散和重力，各开关分别验证。

这些是输入实验，不要求碰撞后速度精确符合理想弹性立方体公式，因为你使用的是球采样和罚力法。重点是方向正确、数值有限和无外力时的动量行为。

- [ ] 初始化 2 个方块，位置分离；关闭重力、地面及阻尼，先做对称正面碰撞。
- [ ] 实现每粒子扫描全部粒子的 O(P²) 基线，只用于小规模。
- [ ] 排除自身与相同 BodyID 的粒子。
- [ ] 先检查距离平方；无接触时尽早退出。对接触才求长度和法线。
- [ ] 对完全重合的不同粒子提供稳定、反对称的备用法线，避免除零。
- [ ] 每线程仅累加当前粒子的力。i→j 和 j→i 各算一次属于此方案设计，不能把一端的力再额外写给另一端而重复计数。
- [ ] 合力归约先用一线程遍历一个刚体全部采样点；先验证再考虑线程组归约。
- [ ] 测试正面、偏心、不同质量、一个静态刚体、完全重合输入。
- [ ] 无外力时监测总线动量：误差不应持续系统性增长。数值积分不精确守恒能量，不把每步能量完全相等作为硬性标准。
- [ ] 打开法向阻尼，再加入有上限的切向阻尼摩擦；检查滑动减弱且不会持续增能。

**验收：** 对称碰撞无明显凭空侧移/旋转，偏心碰撞有角运动；静态刚体不动；所有状态有限。此时你已复现本章的核心算法，尚未达到大规模运行目标。

## 阶段 7：整理 RDG 顺序与生命周期

**这是回头整理阶段，不是到现在才处理线程安全。阶段 1E 开始就应满足基本生命周期要求。**

先画出自己的资源流，用这一份作为代码核对清单：

```text
BodyA + ParticleLocal → BuildParticles → ParticleSnapshot
ParticleSnapshot     → Contacts       → ParticleForces
Snapshot + Forces    → Reduce         → BodyForceTorque
BodyA + ForceTorque  → Integrate      → BodyB
BodyB 成为下一个子步的 BodyA
```

在 Rider 查看每个 Pass 的 Parameters，逐项对照上面输入输出。若一个 Shader 内使用了 Buffer，但 C++ 参数结构没有声明对应读取/写入依赖，就停下来修正。

持久化的意思是“下一帧仍有结果可用”，不是把局部 Graph 对象存进 Actor。参考现有 CubeFlock 中 `RegisterExternalBuffer` 与 `QueueBufferExtraction` 的成对使用；异步命令里保留资源状态的引用，不依赖 Actor 必然活到命令执行。

测试生命周期时使用：Play→Stop，Play→暂停→Stop，两个 Actor 同时运行，修改数量后重置，切换关卡。每种操作都可能暴露普通连续运行时看不到的问题。

- [ ] 将一个子步清楚拆为 BuildParticles → ComputeContacts → ReduceBodyForces → IntegrateBodies。
- [ ] 给各 Pass 命名，例如 `MassSpring.BuildParticles`、`MassSpring.Contacts`，便于 GPU 捕获。
- [ ] 在参数结构中声明真实 SRV/UAV 依赖，避免只在 Lambda 内隐式访问资源。
- [ ] BodyStateA 读、BodyStateB 写，在子步结束交换；同一渲染帧多个子步时，下一步读取上一子步输出。
- [ ] 先只使用普通 Compute 队列；依赖正确且测量有收益后再考虑 Async Compute。
- [ ] 最后一个子步后刷新 Debug 粒子位置，使它们与显示方块一致。
- [ ] 为重置、改变刚体数、改变粒子密度建立安全重建路径，在资源切换完成前保留旧资源所需引用。
- [ ] 重复 Play/Stop、删除 Actor、切关卡、放两个模拟 Actor，检查资源隔离与释放。
- [ ] 若以后从视图回调调度，按世界/帧去重，避免编辑器多视口或立体视图让同一世界一帧模拟多次。

**验收：** RDG 验证无资源依赖问题，多子步结果正确；停止模拟后没有悬空引用或持续新增 GPU 资源。

## 阶段 8：用空间网格替换全体遍历

**这是性能进阶。可以在阶段 6 正确后先跳到阶段 9 的小规模显示练习，再回头完成；不要因此把大规模碰撞留在 O(P²)。**

在代码里保留 `UseGrid` 开关，并且两种路径消费同一份 ParticleSnapshot。这样可以暂停后对同一状态执行两种接触求解，比较力，而不是拿两次已经分岔的运动轨迹比较。

先写固定容量版本，按以下新 Pass 顺序实现：

```text
BuildParticles
→ ClearGrid（计数和溢出归零）
→ FillGrid（粒子索引填格子）
→ ContactsGrid（遍历 27 格中的候选粒子）
→ Reduce → Integrate
```

格子坐标可先用 `floor((position-gridMin)/cellSize)`；三维坐标转线性索引前检查每轴范围。n=4、L=1 时 cellSize=0.25 m。模拟范围扩大后格子数增长很快，先在小的有界实验区域计算所需内存，不要给每格都盲目分配极大容量。

调试时至少读回“网格外粒子数”和“容量溢出数”；若不是零，屏幕上接触似乎正常也不算通过。固定容量版本不够用时，再学习紧凑列表/排序，不需要第一天同时实现两个数据结构。

不要直接把 64000 粒子投入 O(P²) 求解器。先保留暴力版本作为小规模正确性参考。

- [ ] 初版使用有边界的均匀网格，cellSize=d。粒子坐标转换到 cell 坐标时使用 floor，正确处理负坐标。
- [ ] 每子步清空单元计数，将粒子索引填入所属单元，再开始接触查询。
- [ ] 明确网格外粒子策略：报计数并扩容/重建，或进入明确的边界处理；不允许无声丢失碰撞。
- [ ] 若使用“每格固定容量”的原子计数结构，增加 OverflowCounter；查询上限取实际已存数量，不读取未写槽位。
- [ ] 每粒子访问本格及 26 邻格，再执行精确距离检测和同 BodyID 过滤。
- [ ] 在同一份小规模输入上比较网格与暴力版每粒子受力，允许浮点累加顺序导致的小误差。
- [ ] 人为制造密集堆积，验证溢出计数；溢出时该次结果判为不合格，不能仅因为画面还在动就算通过。
- [ ] 需要避免固定容量时，再实现排序 cell key + cell range，或计数/前缀和/散布的紧凑列表。
- [ ] 保留最坏情况意识：高密度单元仍会产生很多候选配对，均匀网格不保证所有场景严格 O(P)。

**验收：** 小场景接触结果与基线一致，目标规模无容量溢出；分别记录 BuildGrid 和 Contacts 的 GPU 时间，确认优化实际有收益。

## 阶段 9：接入 Niagara 显示桥梁

### 9A. 先认识 Niagara 编辑器，不接求解器

**在哪里做：UE 编辑器。先建一个独立显示练习，防止把 Niagara 操作问题与 GPU Buffer 问题混在一起。**

1. Edit → Plugins 搜索 Niagara，确认可用。插件启用若要求重启，先保存再重启。
2. 在 Content Browser 的 MassSpringDemo 文件夹右键，寻找 FX → Niagara System。新建向导和模板名称会随版本变化，选择普通可编辑 Emitter / Empty Emitter，不选 Stateless / Lightweight Emitter 来做本教程。
3. 建立 `NS_MassSpringBodies`，双击打开 Niagara 编辑器，添加一个 Emitter。Emitter 是粒子生成和更新规则，System 可以包含多个 Emitter。
4. 在 Emitter Properties 中找到 Sim Target，设 GPUCompute Sim；在 Emitter Update 添加 Spawn Burst Instantaneous，数量先设为 1。
5. Particle Spawn 中初始化位置为原点、Scale 为 1。避免初始测试粒子立即死亡；若模板有生命周期管理，明确调长生命周期或禁用相应死亡逻辑，并关闭无意的循环重生。
6. 在 Render 区域添加 Mesh Renderer，选择引擎 Cube。选择器看不到引擎网格时启用 Show Engine Content。
7. 编译 Niagara 并保存，将 System 资产拖进关卡，放到容易看到的位置。现在应有一个静止 Cube；没有求解器参与。

Niagara 的 Spawn Burst、Emitter Properties 等是系统内置概念；下面的 `ReadBody` 是我们之后写的函数。找不到 `ReadBody` 不是你操作错，而是自定义 DI 尚未实现。

- [ ] 单独 Niagara System 能显示一个 Cube，能调整 Scale，并知道在哪查看编译错误。

### 9B. 再写自定义 DI，不能直接拖 Buffer

**在哪里做：Rider 写 C++/HLSL；UE 创建用户参数与 Scratch Pad 读取模块。**

建议新建 `Public/NiagaraDataInterfaceMassSpring.h`、`Private/NiagaraDataInterfaceMassSpring.cpp`，需要的 Shader 辅助函数放插件 Shaders 下。这部分是高级扩展接口，本文不给出未经验证的整套可粘贴实现。

按最小功能递增：

1. 在 `.uplugin` 声明 Niagara 依赖，在 `.Build.cs` 添加 Niagara（公开头文件继承其类型时为 Public 依赖），冷构建。
2. 新 DI 最初只实现 `GetBodyCount` 和返回一个固定位置的 GPU 函数，验证它能被 Niagara 使用。
3. 在 Niagara System 创建类型为自定义 DI 的 User Parameter，例如 `User.MassSpringData`。类型尚未出现时先查模块加载/UHT 和 DI 的类型注册方式，不用反复新建 System。
4. 建一个 Scratch Pad 模块，用 DI 函数返回固定位置并写入粒子 Position，确认固定数据可见。
5. 然后把固定数据替换为 Proxy 绑定的已完成 Body Buffer；逐步增加位置、四元数、速度输出。
6. 由管理 Actor 将自己的模拟实例关联到该 Niagara Component 的 DI 实例，处理实例复制与初始化生命周期。关卡里放两个管理 Actor 必须读各自数据。
7. 最后再处理多 BodyID、稳定索引和变更数量后的重置。

DI 代码请在 Rider 中跳转到本机内置实现，观察 GPU 参数绑定、函数注册和 Proxy，而不仅阅读基类声明。不同版本函数签名可能改变；编译通过也不代表 GPU 同步已正确。

### 9C. 切换到真正的 Body 显示

当 9B 的单体读取通过，再把 Burst 数量改为 BodyCount，Spawn 时保存 BodyID；Update 中读取外部状态。没有自定义 DI 前，不要假装执行了这些步骤。

位置首先按 **System Local Space** 设置统一约定：本方案建议使用世界空间显示（Local Space 关闭），输出“模拟原点厘米坐标 + 模拟位置×100”；若选择 Local Space，需重新统一原点与旋转转换。不要让 Actor 平移被加两次。

Niagara 管理对象可作为 MassSpringActor 的 NiagaraComponent；也可以先独立 Niagara Actor 调试，但必须有代码把它关联到指定模拟实例。纯粹把两个 Actor 放到同一个位置不会让它们自动共享 Buffer。

**接下来是最终集成检查项：**

这是单独的 UE 集成任务，通常比写接触公式更费时间。需要按本机 5.8 的 Data Interface 实现核对 API，不能把下列函数名当作可直接复制的完整插件。

- [ ] 为 MassSpring 插件添加 Niagara 插件与模块依赖；当前项目 CubeFlock 已声明 Niagara 依赖，但新插件应声明自己的依赖。
- [ ] 新建 `UNiagaraDataInterfaceMassSpring` 及渲染线程 Proxy，按模拟实例保存资源和 BodyCount。
- [ ] 定义 GPU 读取接口，例如 `ReadBody(BodyID)`，返回位置、四元数、线速度及有效性。这里是拟定接口，需要自己实现。
- [ ] 参考本机 `D:/UE/UE_5.8/Engine/Plugins/FX/Niagara/Source/Niagara/Classes/NiagaraDataInterface.h` 与具体内置 DI，完成函数签名、HLSL 生成、Shader 参数布局和 `SetShaderParameters`。
- [ ] DI 明确只用于 GPU，或为 CPU 调用实现合法的回退，不留下未定义行为。
- [ ] 先发布一个已完成、只读的 Body 快照；允许固定一帧显示延迟。设计环形/双缓冲和资源转换，保证被 Niagara 读取的 Buffer 不被提前覆盖。
- [ ] 用帧编号或模拟步编号检查生产/消费关系；不以游戏线程 Tick 顺序推断 GPU 顺序。跨 RDG 图时明确外部访问与队列同步。
- [ ] 新建 `/Game/MassSpringDemo/NS_MassSpringBodies` 和 GPU Emitter，Burst 一次生成 BodyCount 个显示粒子，不按帧重复生成。
- [ ] Spawn 时保存稳定的自定义 BodyID。不能每帧把当前 Execution Index 当永久 ID；禁用意外死亡、重生和数量变化，数量变更时明确重置系统。
- [ ] 在 Update 中通过 DI 覆盖 Position、Velocity、MeshOrientation；位置和速度由 m 转 cm，朝向不做单位缩放。
- [ ] 去掉额外重力、随机位移、Collision 和第二次 Solve Forces/Velocity，保证求解器是运动状态唯一来源。
- [ ] 添加 Mesh Renderer，绑定 Cube、材质、MeshOrientation 和 Scale。测量 Cube 源网格边长，按 `目标厘米边长/源网格厘米边长` 算 Scale，不默认源网格是 1 cm。
- [ ] 设置 Bounds 覆盖整个模拟区域；移动相机验证不会过早消失。
- [ ] 需要 Debug 时另建 Emitter，每显示粒子对应一个采样点；只在调试开启。
- [ ] 验证位置以外的旋转运动矢量、重置后的历史数据和 TSR 残影；只写 Velocity 并不自动保证所有历史变换正确。

**验收：** 小规模 Niagara 方块与调试快照一致；运行主路径没有逐帧 GPU→CPU→GPU 的全部位置往返；暂停、重置和双实例场景正确，延迟行为已记录。

可选过渡：若 DI 阶段暂时太难，可参考现有 CubeFlock 把位姿写浮点纹理供显示端读取。必须同时存旋转、使用精确 texel 读取并处理读写同步；它是另一种 GPU 数据桥，不是免同步捷径。不要因此改写已验证的求解器。

## 阶段 10：规模、稳定性和性能验收

**在哪里做：UE 运行与性能工具；Rider/日志记录数值。先确定测试场景，再开始调参。**

每次实验只改一个因素，例如保持每边粒子数为 4，仅将刚体数量从 16 增至 100；下一次再保持数量不变，改变时间步。否则你无法知道性能和稳定性变化来自哪里。

UE 控制台的 `stat unit` 可先观察 Frame/Game/Draw/GPU 概况，`stat gpu` 用于初步查看 GPU 项目；不同运行配置与硬件的可用信息会不同。需要拆分自己的 Pass 时，用已加的 RDG 事件名进行 GPU 捕获或 Insights 分析，不把总 GPU 时间直接当 MassSpring 时间。

建议在实验记录中每次写一行普通文字，例如：`B=100, n=4, P=6400, h=1/120, maxSubsteps=8, contacts=待测 ms, overflow=0`。不要先填预期值冒充测量值。

- [ ] 建立固定测试组合：1、2、16、100 个刚体；通过后再挑战 1000 个。
- [ ] 每次同时记录 BodyCount、ParticlesPerEdge、TotalParticles。每边数量翻倍，总粒子数增加 8 倍。
- [ ] 测试低速落地、偏心相撞、多层堆叠、密集初始化及高速穿越。
- [ ] 记录 h、单帧子步数、最大穿透、NaN 计数、网格溢出、模拟 GPU ms、显示 GPU ms、分辨率和 GPU 型号。
- [ ] 关闭调试读回后测性能；不要把总 FPS 变化直接归因于碰撞 Shader。
- [ ] 用 Unreal Insights / GPU 捕获工具识别瓶颈，再选择归约优化、内存布局优化或调度优化。
- [ ] 高速场景若出现隧穿，先限制实验速度或减小 h；需要可靠高速碰撞时另做 CCD，不能把增加 k 当作 CCD。
- [ ] 长时间堆叠若抖动，检查时间步、阻尼、法向力符号、采样密度和摩擦模型；记录罚力法局限，不无限增大刚度。
- [ ] 先保存当前与上一显示状态；需要更平滑时按 accumulator/h 插值显示位置，朝向用正确的四元数插值，并记录额外显示延迟。

**验收：** 形成一个可重复运行的演示关卡和测试记录；只对实际测过的规模报告性能，不预先承诺“十万刚体实时”。

## 阶段 11：本章完成后的进阶选择

- [ ] **接触求解方向**：增加接触缓存、休眠、静摩擦、更稳定的接触求解；每项单独与当前结果比较。
- [ ] **约束方向**：研究 PBD/XPBD，先做两个质点的一根距离约束，再扩展绳子/布料；刚体接触 XPBD 还需要旋转自由度与有效质量推导。
- [ ] **生产物理方向**：用 Chaos 做相同小场景，比较交互需求和开发成本，不要求内部算法与课程一致。
- [ ] **渲染底层方向**：有明确需求后才考虑自定义 SceneProxy / Vertex Factory / 间接绘制，承担材质、阴影、剔除和运动矢量集成。
- [ ] **特效工具方向**：在 Niagara 暴露颜色、调试模式及表现参数，保持物理状态唯一所有者。

## 常见症状的排查顺序

### 先处理 UE / Rider 开发环境问题

- **打开工程提示找不到 NewPlugin**：检查 `.uproject` 插件记录与实际插件位置，按阶段 1A 处理。
- **MassSpring 类一直不出现**：检查 Editor 构建是否成功、插件是否启用，是否已重新启动 UE；Content Browser 的 C++ Classes 显示选项只是可见性，不替代编译。
- **`MassSpringActor.generated.h` 找不到**：不要手写这个文件。先看 UHT 日志，检查 `UCLASS`、`GENERATED_BODY` 和 include 顺序。
- **编译提示 DLL 无法写入 / Live Coding 正在运行**：保存并退出 UE，再使用冷构建流程。不要在编辑器占用 DLL 时连续反复点 Build。
- **`IMPLEMENT_MODULE` 重复符号**：保留现有 MassSpring.cpp 中的一份模块实现，不要又照旧目录图新建第二份模块入口。
- **Cannot open include file**：检查文件名、模块依赖与头文件路径；不要把整个 Engine/Source 加到 include path 试图掩盖依赖问题。
- **Shader 虚拟路径找不到**：检查 Shaders 物理目录、StartupModule 日志、映射前缀和 `.usf` 后缀，确认不是 `.usf.txt`。
- **C++ 构建成功，但启动 UE 时 Shader 编译失败**：这是另一编译阶段；阅读 Shader 文件和入口对应的错误，成功编译 C++ 不表示 HLSL 正确。
- **Rider 里红线很多，但构建通过**：等待索引/重新加载工程模型；先不随意改正确代码来消红线。
- **Actor 能拖入，但 Play 什么也看不到**：查 BeginPlay 日志；确认 Actor Tick 已开启、相机朝向调试区域、使用的是正确关卡。阶段 1C 不是可见 Mesh，线框只在运行时画。
- **编辑器中参数与 C++ 默认值不一致**：实例或 Blueprint 已保存覆盖值，可在 Details 将该属性重置到默认；修改 C++ 初始值不总会覆盖已有实例配置。
- **Build.cs 是 C#，是否要运行这个 C# 项目**：不用，UBT 自动读取它来构建 C++ 模块。

### 再处理模拟问题

- **一运行就炸开**：先查初始穿透、零距离法线、力的符号、h、质量与惯量；不要先怪 Niagara。
- **增加采样点后落得更快**：查重力是否按粒子重复且没分配质量。
- **旋转方向不对**：查 `ω×r` 顺序、四元数乘法顺序和坐标基。
- **重置后某些方块瞬移**：查 BodyID、上一帧显示状态、资源重建和 Niagara 生命周期。
- **相机一转方块全没了**：查 Bounds，再查可见性和粒子寿命。
- **少量正常，大量穿透**：查网格溢出、网格边界和候选邻域，再查时间步。
- **GPU 慢但画面正常**：看总粒子数和 Contacts 时间，确认不是仍在全体配对。

## 最终交付清单

- [ ] 独立 MassSpring 插件和演示关卡。
- [ ] 参数、暂停、单步、重置入口。
- [ ] GPU 接触与刚体积分求解器，含范围检查及数值保护。
- [ ] 小规模暴力参考版和经过对照的网格版本。
- [ ] Niagara 显示系统及明确同步的数据桥。
- [ ] Debug 粒子显示、性能记录和已知局限说明。
- [ ] 能独立解释四阶段数据流，及其与老师源码的每一处主动差异。

## 参考依据

RDG 负责 Pass 与资源依赖；具体资源持久化方式请结合本文阶段 2、7 和当前引擎实现检查。[Epic RDG](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)

Niagara 集成以当前 Data Interface 与 Renderer API 为准；此清单的 Body 读取函数是自定义设计。[Data Interface API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraDataInterface)、[Renderer 文档](https://dev.epicgames.com/documentation/unreal-engine/render-module-reference-for-niagara-effects-in-unreal-engine?lang=en-US)

碰撞采样方法可对照粒子刚体的经典实现；XPBD 属于后续不同的求解路线。[GPU Gems 3 第 29 章](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-29-real-time-rigid-body-simulation-gpus)、[XPBD 原论文](https://matthias-research.github.io/pages/publications/XPBD.pdf)
