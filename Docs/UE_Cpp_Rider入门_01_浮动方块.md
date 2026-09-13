# UE C++ / Rider 入门 01：让一个方块上下浮动

> 面向熟悉 Unity、刚开始学习 UE 和 C++ 的你。项目：Playground02，UE 5.8，Windows + Rider。本文只提供教程，没有替你创建 C++ 文件，也没有启动编译、编辑器或调试器。示例需要按下面步骤在你的项目中构建和验收。

## 1. 这一次只学什么

最终效果：关卡中有一个 Cube，点击运行后，它围绕初始位置上下浮动。你可以在 Details 中调整幅度、频率和开关。

本次直接使用现有 `Playground02` 游戏模块。保留现有 MassSpring、CubeFlock 插件，不修改它们。新练习不需要自己创建插件，不需要 Niagara、Compute Shader、RDG 或 GPU Buffer。

分三次完成，每次都有可以看到的结果：

1. 创建一个空 Actor，放进关卡，运行时打印一条日志。
2. 给 Actor 添加 Static Mesh Component，在编辑器中给它选择 Cube。
3. 使用 Tick 和正弦函数，让 Cube 上下浮动。

**每个阶段提供该阶段完整的两个文件。进入下一阶段时，用新版本替换同名文件内容，不是把两份类定义接在一起。**

## 2. 先对应你熟悉的 Unity

- Unity 的 Scene，在这里对应 Level / Map（关卡）。
- Hierarchy 对应 World Outliner（世界大纲）。
- Inspector 对应 Details（细节）。
- Project 窗口对应 Content Browser（内容浏览器）。
- 场景里的 GameObject，在这里大致对应 Actor。
- MeshFilter 和 MeshRenderer 的常见静态模型用途，在这里由 Static Mesh Component 承担。
- `Start()` 的本次对应入口是 `BeginPlay()`。
- `Update()` 的本次对应入口是 `Tick(float DeltaSeconds)`。
- `Debug.Log()` 对应本例的 `UE_LOG()`。

这些只是帮助入门的近似对应。尤其注意：**Actor 本身不是要拖到另一个 Actor 上的 MonoBehaviour 脚本。** 本例创建的 Actor 类直接拖进关卡，成为一个场景对象。

UE C++ 常把一个类拆成两个文件：

```text
LearningFloatActor.h    声明：这个类有哪些函数和成员
LearningFloatActor.cpp  实现：这些函数具体做什么
```

这两个文件共同定义一个类 `ALearningFloatActor`，不是两个独立组件。

## 3. 先熟悉一次构建和运行流程

### 3.1 用 Rider 打开现有项目

1. 在 UE 保存关卡，停止 Play，然后关闭编辑器。
2. 在 Rider 中选择 File → Open，打开 [Playground02.uproject](E:/UEProjects/Playground02/Playground02.uproject)。如果当前已经打开正确项目，不用重复打开。
3. 等待工程加载和索引结束。
4. 找到 `Source/Playground02`，确认能打开 `Playground02.Build.cs`。

Rider 可直接打开 `.uproject`，无需为了本教程新建 `.sln`。Rider 是编辑工具，实际构建由 UE 的 Unreal Build Tool 调用 C++ 工具链完成。[JetBrains 项目打开说明](https://www.jetbrains.com/help/rider/Unreal_Engine__Before_You_Start.html)

当前项目的 Build.cs 已包含 Core、CoreUObject、Engine，本练习不用改它。

### 3.2 本教程统一使用冷构建

每次改完一个阶段的代码，都按这个顺序：

```text
UE 保存 → 关闭 UE → Rider 保存代码
→ 构建 Playground02Editor / Development / Win64
→ 确认成功 → 重新打开 UE → 打开练习关卡 → 运行
```

初期先不使用 Live Coding / Hot Reload，减少“代码改了，但编辑器仍运行旧版本”的干扰。

在 Rider 顶部目标/配置中确认项目是 Playground02 的 Editor 目标，配置为 Development，平台 Win64。界面有时组合显示为 Development Editor。不要选 Shipping，也不用编译整个引擎。

然后执行 Build 菜单中针对当前项目的构建。菜单位置因 Rider 版本/Keymap 不同会变化，以目标名称和 Build 输出为准。

如果你已经习惯 Rider Terminal，也可以自己在 PowerShell 运行下面这一条，**两种方式选一种，不要同时启动**：

```powershell
& 'D:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat' Playground02Editor Win64 Development '-Project=E:\UEProjects\Playground02\Playground02.uproject' -WaitMutex
```

这是供你手动选择的命令，不是阅读文档后自动执行的操作。

### 3.3 如何知道构建成功

最后应有 `Result: Succeeded`。第一次加入本例时，通常还会看到反射代码生成、C++ 编译及 `UnrealEditor-Playground02.dll` 链接记录；增量构建的文件分组名称可能不同。

`Warning` 不等于失败。出现 `Error` 时先看第一条具体错误及附近上下文，最后的退出码通常只是失败结果。

`Build.cs` 是用 C# 写的构建规则，不是需要你在 Rider 中另行运行的 C# 游戏程序。普通构建也不需要你手动附加 VS 调试器。

## 4. 阶段一：一个空 Actor，先让它存在

### 4.1 创建两个文件

关闭 UE。在 Rider 的 `Source/Playground02` 目录上右键，新建下面两个文件：

- `E:/UEProjects/Playground02/Source/Playground02/LearningFloatActor.h`
- `E:/UEProjects/Playground02/Source/Playground02/LearningFloatActor.cpp`

文件名请完全一致，不要建到 `Plugins/MassSpring`，也不要给文件再加 `.txt` 后缀。若使用 Rider 的 Unreal Class 向导，父类选 Actor，模块选 Playground02；向导生成同名文件后编辑它们即可，不要重复建类。

这次放在已有模块根目录，先不额外引入 Public/Private 文件夹组织规则。

### 4.2 头文件完整内容

将 `LearningFloatActor.h` 写成：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LearningFloatActor.generated.h"

UCLASS()
class PLAYGROUND02_API ALearningFloatActor : public AActor
{
    GENERATED_BODY()

public:
    ALearningFloatActor();

protected:
    virtual void BeginPlay() override;
};
```

先理解这些：

- `#pragma once`：避免这个头文件被重复展开。
- `#include`：引入其他声明，类似“我需要使用哪些类型”，不等于 C# 的命名空间导入机制。
- `: public AActor`：继承 UE 的 Actor 类。
- `A` 前缀：UE 对 Actor 类型的命名约定。
- `PLAYGROUND02_API`：当前模块的导出宏，先保留原样。
- `UCLASS()` 和 `GENERATED_BODY()`：让 UE 的反射工具认识这个类。
- `ALearningFloatActor()`：构造函数，没有返回值类型，名字与类相同。
- `BeginPlay()`：声明一个开始运行时执行的函数。
- `override`：明确表示它覆盖基类的虚函数，编译器可以检查签名是否正确。
- 类定义最后的 `;` 不能漏掉。

**`LearningFloatActor.generated.h` 不需要自己创建。** Unreal Header Tool（UHT）在构建时生成它。它应是这个头文件中最后一条 include。

### 4.3 实现文件完整内容

将 `LearningFloatActor.cpp` 写成：

```cpp
#include "LearningFloatActor.h"

ALearningFloatActor::ALearningFloatActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ALearningFloatActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Display, TEXT("LearningFloatActor started: %s"), *GetName());
}
```

`ALearningFloatActor::BeginPlay` 中的 `::` 表示“这是 ALearningFloatActor 类的 BeginPlay 实现”。

`Super::BeginPlay()` 调用父类的实现，类似 C# 中的 `base` 调用。`TEXT()` 构造 UE 常用的文本字面量；`GetName()` 返回对象名字；这里的 `*GetName()` 是 FString 提供的取字符数据操作，不是让你手动管理一块字符串内存。

目前不需要逐帧工作，所以关闭 Tick。

### 4.4 编译、放进关卡、看日志

1. 保存两个文件，按照第 3 节构建，确认成功。
2. 重新打开 UE。在 Content Browser 的项目 Content 下新建文件夹 `LearningBasics`。
3. File → New Level 选择 Basic 一类的基础关卡模板，保存为 `Content/LearningBasics/Lvl_FloatingCube`。如果模板名称不同，可以用一个已有基础关卡另存为这个名字。
4. Content Browser 设置中启用 Show C++ Classes（显示 C++ 类），在 C++ Classes → Playground02 查找 `LearningFloatActor`。也可以用 Place Actors 搜索这个类，显示名可能被分词成 Learning Float Actor。
5. 把类拖进关卡，确认 World Outliner 中出现该 Actor。
6. 这是空 Actor，**没有可见方块是正常的**。这个阶段不需要设置它的位置。
7. 在 Window 菜单找到 Output Log（输出日志；有的布局位于 Developer Tools 子菜单）。
8. 点击 Play 或 Play 下拉里的 Simulate，搜索 `LearningFloatActor started`。
9. 停止运行并保存关卡。

构造函数不等于 Unity 的 Start。UE 在创建类默认对象、加载类和生成实例时都可能运行构造函数；涉及当前游戏世界的初始化，本例放在 BeginPlay。

**阶段一验收：**

- [x] 我创建的是 `.h/.cpp`，没有手写 `.generated.h`。
- [x] Rider 构建成功，UE 能找到这个类。
- [x] 关卡中有一个 LearningFloatActor。
- [x] 点击运行能看到日志，停止后再次运行会再次输出。

通过后再进入下一节。你已经完成 UE C++ 最基本的一次完整循环。

## 5. 阶段二：让 Actor 有一个可见的 Cube

### 5.1 我们添加的是什么

空 Actor 只有行为，没有模型。现在给它创建 `UStaticMeshComponent`，由这个组件承载 Static Mesh 资产和渲染。

我们会在编辑器中手动选择 Cube，先不在 C++ 构造函数里查找或加载资产。这能让“创建组件”和“给组件设置模型”两个步骤看得更清楚。

### 5.2 替换头文件

关闭 UE，将 `LearningFloatActor.h` 完整替换为：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LearningFloatActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class PLAYGROUND02_API ALearningFloatActor : public AActor
{
    GENERATED_BODY()

public:
    ALearningFloatActor();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Learning")
    TObjectPtr<UStaticMeshComponent> CubeMesh;
};
```

`class UStaticMeshComponent;` 是前置声明：告诉编译器这个类型存在，完整定义在 `.cpp` include。

`TObjectPtr` 是 UE 的对象引用类型，用它保存组件引用；这里配合 UPROPERTY，让 UE 可以跟踪这个成员。此处不需要自己 `new`、`delete` 组件。

`VisibleAnywhere` 表示这个组件引用可见，但不是让你在 Details 中替换指针。组件自己的 Static Mesh 等可编辑属性仍能在选中组件后设置。

### 5.3 替换实现文件

将 `LearningFloatActor.cpp` 完整替换为：

```cpp
#include "LearningFloatActor.h"
#include "Components/StaticMeshComponent.h"

ALearningFloatActor::ALearningFloatActor()
{
    PrimaryActorTick.bCanEverTick = false;

    CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
    SetRootComponent(CubeMesh);
    CubeMesh->SetMobility(EComponentMobility::Movable);
    CubeMesh->SetSimulatePhysics(false);
    CubeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALearningFloatActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Display, TEXT("LearningFloatActor started: %s"), *GetName());
}
```

`CreateDefaultSubobject<类型>` 在构造函数中创建默认组件；尖括号中的类型告诉函数创建什么。`SetRootComponent` 把它设成根组件，这样移动 Actor 就会移动这个组件。

`Movable` 表示允许运行时移动。我们关闭物理模拟和碰撞，因为本练习用代码直接决定位置；先不让重力、碰撞反应一起干扰结果。

### 5.4 在 UE 给组件选择模型

1. 保存、冷构建、重新打开练习关卡。
2. 选中 LearningFloatActor，在 Details 的组件列表中选中 `CubeMesh`。若列表折叠，先展开组件区域。
3. 找到 Static Mesh 属性，点击资产选择器。
4. 在选择器设置中启用 Show Engine Content（显示引擎内容），搜索 Cube，选择引擎 BasicShapes 下的 Cube：`/Engine/BasicShapes/Cube`。
5. 模型出现后，选 Actor 自身，设置 Location=(0,0,200)、Rotation=(0,0,0)、Scale=(1,1,1)。Location 单位是厘米，也就是离原点上方 2 m。
6. 确认没有在组件 Details 中重新勾上 Simulate Physics。
7. 保存关卡，点击运行，方块应保持静止，日志仍然正常输出。

如果旧的阶段一实例显示不正常，可以删除**这个练习 Actor 实例**，重新拖一个该类进关卡，再设置 Cube；不需要删除源文件或其他关卡内容。

如果看不清：选中 Actor 按 F 聚焦；右键按住配合 WASD 移动镜头。可暂时把视口的 Lit 改为 Unlit，区分“模型没设置”和“灯光太暗”。

**阶段二验收：**

- [x] 我知道 Actor 与 Static Mesh Component 的区别。
- [x] 我通过 Details 给 CubeMesh 选择了 Cube 资产。
- [x] 方块在编辑器和运行时可见，运行时保持静止。
- [x] 保存并重开关卡后，Cube 资产选择仍然保留。

## 6. 阶段三：让 Cube 上下浮动

### 6.1 先看运动公式

这次只改变 Z，高度来自：

```text
当前高度 = 初始高度 + 幅度 × sin(2π × 频率 × 已运行秒数)
```

幅度单位为厘米；频率单位为 Hz，也就是每秒完整往返几次。幅度 50 表示上下各 50 cm，总活动范围 100 cm；频率 0.5 表示每 2 秒完成一次往返。

我们每帧从初始位置算绝对位置，**不是每帧把正弦偏移重复加到当前位置**，否则会产生累积错误。

### 6.2 最终完整头文件

关闭 UE，将 `LearningFloatActor.h` 完整替换为：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LearningFloatActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class PLAYGROUND02_API ALearningFloatActor : public AActor
{
    GENERATED_BODY()

public:
    ALearningFloatActor();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Learning")
    TObjectPtr<UStaticMeshComponent> CubeMesh;

    UPROPERTY(EditAnywhere, Category = "Learning|Motion")
    bool bEnableFloating = true;

    UPROPERTY(EditAnywhere, Category = "Learning|Motion",
        meta = (ClampMin = "0.0", Units = "cm"))
    float AmplitudeCm = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Learning|Motion",
        meta = (ClampMin = "0.0"))
    float FrequencyHz = 0.5f;

private:
    FVector StartLocation = FVector::ZeroVector;
    float ElapsedSeconds = 0.0f;
};
```

新增内容解释：

- `Tick(float DeltaSeconds)`：每次运行收到距离上一帧经过的秒数。
- `EditAnywhere`：允许在类默认配置或场景实例中编辑这个属性。
- `Category`：Details 中的分类；竖线表示分组层次。
- `ClampMin`：编辑器输入约束，不是完整的运行时数据校验。
- `bEnableFloating`：UE 常用 b 前缀标识布尔变量。
- `50.0f` 的 f 表示 float 字面量。
- `StartLocation`、`ElapsedSeconds` 只供内部运行使用，不需要在 Details 编辑，因此没有加 UPROPERTY。

这些浮动参数放在 protected 并不影响 UPROPERTY 在编辑器中暴露；C++ 访问控制与 UE 编辑器可编辑性是两个层面。

### 6.3 最终完整实现文件

将 `LearningFloatActor.cpp` 完整替换为：

```cpp
#include "LearningFloatActor.h"
#include "Components/StaticMeshComponent.h"

ALearningFloatActor::ALearningFloatActor()
{
    PrimaryActorTick.bCanEverTick = true;

    CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
    SetRootComponent(CubeMesh);
    CubeMesh->SetMobility(EComponentMobility::Movable);
    CubeMesh->SetSimulatePhysics(false);
    CubeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALearningFloatActor::BeginPlay()
{
    Super::BeginPlay();

    StartLocation = GetActorLocation();
    ElapsedSeconds = 0.0f;

    UE_LOG(LogTemp, Display,
        TEXT("Floating cube started. Amplitude=%.1f cm, Frequency=%.2f Hz"),
        AmplitudeCm, FrequencyHz);
}

void ALearningFloatActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bEnableFloating)
    {
        return;
    }

    ElapsedSeconds += DeltaSeconds;

    const float SafeAmplitude = FMath::Max(AmplitudeCm, 0.0f);
    const float SafeFrequency = FMath::Max(FrequencyHz, 0.0f);
    const float Phase = 2.0f * PI * SafeFrequency * ElapsedSeconds;
    const float OffsetZ = SafeAmplitude * FMath::Sin(Phase);

    FVector NewLocation = StartLocation;
    NewLocation.Z += OffsetZ;
    SetActorLocation(NewLocation);
}
```

### 6.4 沿着一次运行理解代码

1. 构造函数创建 CubeMesh，允许 Tick，配置可移动和无物理碰撞。
2. 点击运行时，BeginPlay 记录你在关卡摆放的初始位置，例如 Z=200。
3. 每帧 Tick 把 DeltaSeconds 加到 ElapsedSeconds，得到累计运行时间。
4. 正弦函数生成 -1 到 +1 之间的数，乘以幅度得到高度偏移。
5. 复制 StartLocation，只修改 Z，再调用 SetActorLocation。

这里移动的是 Actor，其根组件 CubeMesh 跟随它移动。没有使用物理力，也没有调用 AddForce；这是一个数学动画。

为什么不写 `ElapsedSeconds += 0.016f`？因为帧率不恒定。DeltaSeconds 表示实际经过时间，让运动周期不依赖“刚好每秒 60 帧”的假设。

为什么 OffsetZ 不再乘 DeltaSeconds？因为公式已经通过累计秒数算出了**位置**，它不是“每秒移动量”。之后学速度积分时，才会出现 `位置 += 速度 × DeltaSeconds`。

`const` 表示本次计算中变量不再修改；`FMath` 是 UE 常用数学函数集合；`FVector` 表示三个分量。

本例没有附着父对象，所以 StartLocation 是世界坐标。在运行中拖动 Actor 也不会更新 StartLocation，下次 Tick 会按原位置继续运动；需要换摆放位置时，先停止，再移动，再运行。

### 6.5 编译和观察

1. 保存、冷构建、重新打开关卡。
2. 选中 LearningFloatActor，确认 CubeMesh 仍然选了 Cube。
3. Actor Location 设 (0,0,200)，Scale 保持 (1,1,1)。
4. 在 Details 的 Learning / Motion 分类中设置 Enable Floating=true、Amplitude Cm=50、Frequency Hz=0.5。显示名称可能被 UE 自动加空格。
5. 建议从 Play 下拉选择 **Simulate**，便于用编辑器镜头观察，不必处理现有项目玩家角色和输入。
6. 方块应在 Z=150 到 250 cm 之间往返，大约 2 秒一个周期。
7. 停止运行，回到编辑状态，再改参数并保存。

**阶段三验收：**

- [x] BeginPlay 日志里的参数与我设置的一致。
- [x] Cube 上下浮动，X/Y 不变，不会越飘越远。
- [x] 停止后改幅度，下一次运行范围改变。
- [x] 停止后改频率，下一次运行周期改变。
- [x] 停止并重新运行，运动从当前编辑位置重新开始。

## 7. 做四个小实验，确认真的理解了

每次都先停止运行，在编辑状态改值，再开始。这样不会混淆运行实例和关卡保存值。

### 实验 A：只改幅度

- [x] 频率保持 0.5，把幅度从 50 改成 100。

预期：初始 Z=200 时在 100～300 cm 之间活动，周期仍约 2 秒。

### 实验 B：只改频率

- [x] 幅度保持 50，把频率改为 1。

预期：上下范围不变，每秒完成一次往返。

### 实验 C：关闭开关

- [x] 运行前取消 Enable Floating。

预期：Cube 静止。若你在运行中关闭，它停在关闭时的位置；再次开启会从之前累计的相位继续，因为关闭时我们既没更新位置，也没累计时间。

### 实验 D：放两个实例

- [x] 停止运行，复制一个 LearningFloatActor，放到 X=200、Z=200。
- [x] 第一个频率设 0.5，第二个设 1，再运行。

预期：两个 Cube 独立运动。每个实例有自己的 StartLocation、ElapsedSeconds 和可编辑参数，不需要复制 C++ 类。

**边界现象：** 幅度为零会让位置回到 StartLocation；频率为零也产生零偏移。运行中修改幅度或频率可能使位置跳变，这是当前简单公式的结果，不是编译错误。长时间运行的 float 累计精度不是本节目标。

## 8. 可选：做一次 Rider 断点调试

这不是完成浮动效果的必要步骤。先让正常运行成功，再练习。

1. 关闭当前 UE 编辑器，避免同时启动两个实例。
2. Rider 选择 Playground02 的 Editor 运行配置，使用 **Debug** 启动。这里指以调试器运行，构建配置仍可保持 Development Editor。
3. 在 BeginPlay 的 `StartLocation = GetActorLocation();` 左侧行号栏点击，放一个断点。
4. UE 打开后运行关卡，应停在断点处。
5. 使用 Step Over 单步，看 StartLocation 和 ElapsedSeconds 的值，然后 Resume 继续。
6. 用完移除断点。不要初次就在 Tick 无条件断点，否则每帧都会停住。

C++ 调试器暂停游戏线程时，UE 看起来会卡住，这是正常暂停。此步骤由你主动在 Rider 执行，不需要同时打开 VS 调试器。[JetBrains UE 调试说明](https://www.jetbrains.com.cn/en-us/help/rider/Unreal_Engine__Debugger.html)

## 9. 常见问题：从最容易查的地方开始

### Rider 报 generated.h 不存在

先构建，让 UHT 生成它。若构建失败，检查 `.generated.h` 文件名是否与头文件一致、是否是最后一个 include、类末尾是否有分号。不要自行创建 generated.h。

### 编译成功，但 UE 中找不到类

确认打开的是 Playground02，插件模块与游戏模块没有混淆；检查 C++ Classes 显示选项，重新启动 UE。UE Content Browser 主要显示反射类和资产，不会把所有 `.cpp` 当作 Unity 脚本资产列出来。

### 报错 PLAYGROUND02_API 或类重复定义

确认文件在 `Source/Playground02`，使用的是 `PLAYGROUND02_API`；每个阶段应替换旧内容，不能把三个版本粘在一个文件里。

### 有 Actor，但看不到 Cube

阶段一没有模型是预期结果。阶段二之后检查 CubeMesh 的 Static Mesh 是否为空，再查位置、缩放和镜头；可以用 Unlit 视图排除光照问题。

### Details 中没有运动参数

确认已完成阶段三的头文件替换和冷构建，并选中 Actor 自身而不是只选 CubeMesh 的组件属性。Rider 保存代码不等于 UE 已加载新 DLL。

### Cube 可见但不动

确认正在 Play / Simulate、Enable Floating 勾选、幅度和频率非零、构造函数中 `bCanEverTick=true`、组件为 Movable。不要只在编辑状态等待它自动动起来。

### 方块掉下去、被碰撞弹开

检查是否在组件 Details 中启用了 Simulate Physics。当前教程的位置由 Tick 决定，不需要引擎物理来推动它。

### 改了 C++ 默认参数，关卡里还是原值

已保存的实例属性可能覆盖 C++ 默认值。直接在 Details 设置，或使用该属性的重置默认值按钮。修改构造/成员默认值不一定覆盖已有对象配置。

### 运行中改参数，停止后丢了

Play 世界通常是用于测试的副本。需要保存的参数，在停止后对编辑状态下的实例设置，再保存关卡。

### Rider 构建提示 Live Coding 或 DLL 被占用

保存并退出 UE，再按冷构建流程重试。没有必要连续重复点击 Build，也不要先删除整个 Binaries/Intermediate。

### 再次出现 Niagara 模块崩溃

本练习没有加载 Niagara 资源。如果堆栈仍指向 CubeFlock 的 NiagaraFlockActor，先处理那个既有模块问题，再继续本教程；不要因此改浮动公式或给这个 Actor 加 Niagara 依赖。

## 10. 完成后你应该能讲清楚的内容

- [x] `.h` 声明与 `.cpp` 实现各负责什么。
- [x] 构造函数、BeginPlay、Tick 在本例中分别做什么。
- [x] Actor 和 Static Mesh Component 如何组合。
- [x] UPROPERTY 如何让变量出现在 Details。
- [x] 为什么记录初始位置、为什么使用 DeltaSeconds。
- [x] 幅度与频率分别控制运动的哪一部分。
- [x] 从 Rider 改代码到 UE 运行的完整流程。

这节做到这里就结束。下一节再学习“速度、加速度和自由落体”，先用 CPU 模拟一个方块；等这些概念熟悉后，再讨论如何把计算搬到 GPU。
