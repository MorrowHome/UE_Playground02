# UE C++ / Rider 入门 02：游戏内按钮、运动控制与状态同步

> 接续 [第 1 节：浮动方块](E:/UEProjects/Playground02/Docs/UE_Cpp_Rider入门_01_浮动方块.md)。本文根据你当前的 LearningFloatActor 源码编写。只新增教学文档，不替你修改源码、创建资产或启动编译。下面的代码需要你在项目中构建并运行验证。

## 1. 本节结果与学习重点

运行游戏后，屏幕左上角出现一个控制面板：

- 显示“状态：运动中”或“状态：已暂停”。
- 点击“暂停运动”，方块停在当前位置，按钮改成“继续运动”。
- 再点一次，方块从暂停时的相位继续浮动。
- 点击“重置位置”，方块回到 BeginPlay 时的初始位置，但不改变暂停开关。

这是屏幕上的可交互 UI，不是 Details 面板里的编辑器按钮，也不是场景中可按压的三维按钮。三维按钮需要额外的碰撞/交互或 Widget Component，留到后面。

本节深入四个 UE 开发概念：

1. 用 UFUNCTION 为对象暴露行为接口。
2. 用 UUserWidget + Widget Blueprint 组合 C++ 行为与可视布局。
3. 用动态多播委托把状态变化通知给 UI。
4. 明确谁持有目标引用、谁创建 UI，以及鼠标输入交给谁。

先完成按钮这条交互链，不同时引入自由落体。下一节再用这个面板控制重力模拟。

## 2. 先理解对象关系

```text
Level Blueprint：知道本关卡要控制哪个方块
  ├─ 创建 WBP_MotionPanel（UI 实例）
  ├─ 把关卡中的 LearningFloatActor 引用交给面板
  └─ 设置鼠标与输入模式

WBP_MotionPanel：布局由 UMG 制作，逻辑来自 C++ UMotionControlWidget
  ├─ 点击按钮 → 调用 Actor.ToggleMotion()
  └─ 收到 Actor.OnMotionEnabledChanged → 刷新文字

LearningFloatActor：持有运动状态，执行 Tick
  └─ 状态改变后广播通知，不知道具体是哪个 UI 在控制它
```

你可以把它类比为 Unity：Canvas 上的 Button 调用一个 MonoBehaviour 方法，该对象再通过 C# event 通知 UI。这里不逐帧查找 Actor，也不在 UI 中复制一份独立的 Enable Motion 布尔值。

**真实状态只在 Actor 中保存。** UI 文字反映这个状态；按钮文字表示“下一次点击要执行的动作”。运动中显示“暂停运动”，暂停时显示“继续运动”。

## 3. 文件和资产清单

修改现有两个文件：

- `E:/UEProjects/Playground02/Source/Playground02/LearningFloatActor.h`
- `E:/UEProjects/Playground02/Source/Playground02/LearningFloatActor.cpp`

新增两个 C++ 文件，仍放在现有 Playground02 模块：

- `E:/UEProjects/Playground02/Source/Playground02/MotionControlWidget.h`
- `E:/UEProjects/Playground02/Source/Playground02/MotionControlWidget.cpp`

在 UE 中新增资产：

- `/Game/LearningBasics/UI/WBP_MotionPanel`：界面布局。
- `/Game/LearningBasics/BP_LearningUIGameMode`：可选但推荐，用于隔离现有游戏控制器逻辑。
- `/Game/LearningBasics/Lvl_MotionUI`：从上一节关卡另存的新实验关卡。

不要把 C++ 文件建在插件中，不要创建第二个 LearningFloatActor 类。

## 4. 第一步：为 Actor 增加行为接口与事件

### 4.1 为什么不直接让按钮改 bEnableFloating

直接暴露为 `BlueprintReadWrite` 也能实现开关，但外部可以绕过状态变更逻辑，不容易统一发送通知。我们保留已有字段名，通过方法修改：

```cpp
SetMotionEnabled(false); // 设置指定状态，可安全重复调用
ToggleMotion();          // 切换当前状态，适合 Button
IsMotionEnabled();       // 读取状态
ResetMotion();           // 重置位置和相位，不改变开关
```

你现有字段叫 `bEnableFloating`。为保留已保存的实例值，继续使用这个名字，只改变它在 Details 中的显示名为 Enable Motion。UI 不需要知道字段的实现名字。

### 4.2 完整头文件

关闭 UE，将 `LearningFloatActor.h` 替换为：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LearningFloatActor.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnLearningMotionEnabledChanged, bool, bEnabled);

UCLASS()
class PLAYGROUND02_API ALearningFloatActor : public AActor
{
    GENERATED_BODY()

public:
    ALearningFloatActor();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Learning|Motion")
    void SetMotionEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Learning|Motion")
    void ToggleMotion();

    UFUNCTION(BlueprintPure, Category = "Learning|Motion")
    bool IsMotionEnabled() const;

    UFUNCTION(BlueprintCallable, Category = "Learning|Motion")
    void ResetMotion();

    UPROPERTY(BlueprintAssignable, Category = "Learning|Motion")
    FOnLearningMotionEnabledChanged OnMotionEnabledChanged;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Learning")
    TObjectPtr<UStaticMeshComponent> CubeMesh;

    UPROPERTY(EditAnywhere, Category = "Learning|Motion",
        meta = (DisplayName = "Enable Motion"))
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

### 4.3 完整实现文件

将 `LearningFloatActor.cpp` 替换为：

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
}

void ALearningFloatActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bEnableFloating)
    {
        return;
    }

    ElapsedSeconds += DeltaSeconds;
    const float Phase = 2.0f * PI * FMath::Max(FrequencyHz, 0.0f) * ElapsedSeconds;
    const float OffsetZ = FMath::Max(AmplitudeCm, 0.0f) * FMath::Sin(Phase);
    FVector NewLocation = StartLocation;
    NewLocation.Z += OffsetZ;
    SetActorLocation(NewLocation);
}

void ALearningFloatActor::SetMotionEnabled(bool bEnabled)
{
    if (bEnableFloating == bEnabled)
    {
        return;
    }

    bEnableFloating = bEnabled;
    UE_LOG(LogTemp, Display, TEXT("%s: motion %s"), *GetName(),
        bEnabled ? TEXT("enabled") : TEXT("disabled"));
    OnMotionEnabledChanged.Broadcast(bEnabled);
}

void ALearningFloatActor::ToggleMotion()
{
    SetMotionEnabled(!bEnableFloating);
}

bool ALearningFloatActor::IsMotionEnabled() const
{
    return bEnableFloating;
}

void ALearningFloatActor::ResetMotion()
{
    if (!HasActorBegunPlay())
    {
        return;
    }
    ElapsedSeconds = 0.0f;
    SetActorLocation(StartLocation);
}
```

你原来的 `.cpp` 没显式 include `Components/StaticMeshComponent.h`；即使此前通过预编译头间接编译成功，这里也补上直接依赖，避免依赖偶然的 include 顺序。

### 4.4 理解这次新增的 UE 机制

- `BlueprintCallable`：方法可以作为蓝图执行节点调用。
- `BlueprintPure`：读取状态的纯查询节点，这里用 const 确保不会修改对象。
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam`：声明一个可绑定多个监听者、携带一个 bool 参数的动态委托类型。
- `BlueprintAssignable`：蓝图可以订阅这个事件。
- `Broadcast`：通知当前所有监听者；没有监听者也能调用。

动态委托可以绑定反射函数，所以后面的 C++ 回调需要 UFUNCTION。并不是所有 C++ 回调都必须用动态委托；此处选择它是为了和 UE UI/蓝图体系衔接。[Epic 委托说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-and-lambda-functions-in-unreal-engine)、[UPROPERTY 说明](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-uproperties)

`SetMotionEnabled` 在值相同时直接返回，避免重复通知。暂停只停止该方块的时间累计，不暂停整个游戏世界，也不阻止 UI 点击。

**先验收：**

- [ ] 冷构建通过，上一节方块仍可浮动。
- [ ] Details 中的 Enable Motion 能设置运行初始状态。
- [ ] 此时还没有屏幕按钮，这是预期结果。

## 5. 第二步：创建 C++ Widget 基类

### 5.1 检查模块依赖

你的 [Playground02.Build.cs](E:/UEProjects/Playground02/Source/Playground02/Playground02.Build.cs) 已经包含 UMG 和 Slate，**不用重复添加**。本节直接使用 UUserWidget、UButton 和 UTextBlock，不需要为教程盲目添加其他模块。

新建 `MotionControlWidget.h` 和 `MotionControlWidget.cpp`。Widget 是 UI 对象，不继承 AActor；它不能像方块一样拖到世界大纲里。

### 5.2 完整 Widget 头文件

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MotionControlWidget.generated.h"

class ALearningFloatActor;
class UButton;
class UTextBlock;

UCLASS()
class PLAYGROUND02_API UMotionControlWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Learning|UI")
    void SetTargetActor(ALearningFloatActor* NewTarget);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ToggleMotionButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ResetMotionButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ToggleMotionText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MotionStateText;

private:
    UPROPERTY(Transient)
    TObjectPtr<ALearningFloatActor> TargetActor;

    void BindTargetEvents();
    void UnbindTargetEvents();
    void RefreshUI();

    UFUNCTION()
    void HandleToggleClicked();

    UFUNCTION()
    void HandleResetClicked();

    UFUNCTION()
    void HandleMotionEnabledChanged(bool bEnabled);

    UFUNCTION()
    void HandleTargetDestroyed(AActor* DestroyedActor);
};
```

`BindWidget` 要求派生 Widget Blueprint 中存在**同名、同类型**控件。它只是按名字把控件引用接到成员，不会凭空创建一个 Button。

`TargetActor` 是运行时引用，因此标记 Transient，不作为持久资产数据保存。UPROPERTY 使引用可被 UE 跟踪，但它不能阻止关卡 Actor 被 Destroy，所以后面仍要检查 IsValid。

### 5.3 完整 Widget 实现文件

```cpp
#include "MotionControlWidget.h"
#include "LearningFloatActor.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UMotionControlWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ToggleMotionButton->OnClicked.AddUniqueDynamic(
        this, &UMotionControlWidget::HandleToggleClicked);
    ResetMotionButton->OnClicked.AddUniqueDynamic(
        this, &UMotionControlWidget::HandleResetClicked);

    BindTargetEvents();
    RefreshUI();
}

void UMotionControlWidget::NativeDestruct()
{
    UnbindTargetEvents();
    ToggleMotionButton->OnClicked.RemoveDynamic(
        this, &UMotionControlWidget::HandleToggleClicked);
    ResetMotionButton->OnClicked.RemoveDynamic(
        this, &UMotionControlWidget::HandleResetClicked);

    Super::NativeDestruct();
}

void UMotionControlWidget::SetTargetActor(ALearningFloatActor* NewTarget)
{
    UnbindTargetEvents();
    TargetActor = NewTarget;
    BindTargetEvents();
    RefreshUI();
}

void UMotionControlWidget::BindTargetEvents()
{
    if (IsValid(TargetActor))
    {
        TargetActor->OnMotionEnabledChanged.AddUniqueDynamic(
            this, &UMotionControlWidget::HandleMotionEnabledChanged);
        TargetActor->OnDestroyed.AddUniqueDynamic(
            this, &UMotionControlWidget::HandleTargetDestroyed);
    }
}

void UMotionControlWidget::UnbindTargetEvents()
{
    if (IsValid(TargetActor))
    {
        TargetActor->OnMotionEnabledChanged.RemoveDynamic(
            this, &UMotionControlWidget::HandleMotionEnabledChanged);
        TargetActor->OnDestroyed.RemoveDynamic(
            this, &UMotionControlWidget::HandleTargetDestroyed);
    }
}

void UMotionControlWidget::HandleToggleClicked()
{
    if (IsValid(TargetActor))
    {
        TargetActor->ToggleMotion();
    }
}

void UMotionControlWidget::HandleResetClicked()
{
    if (IsValid(TargetActor))
    {
        TargetActor->ResetMotion();
    }
}

void UMotionControlWidget::HandleMotionEnabledChanged(bool bEnabled)
{
    // Re-query the actor: it remains the source of truth.
    RefreshUI();
}

void UMotionControlWidget::HandleTargetDestroyed(AActor* DestroyedActor)
{
    if (DestroyedActor == TargetActor.Get())
    {
        UnbindTargetEvents();
        TargetActor = nullptr;
        RefreshUI();
    }
}

void UMotionControlWidget::RefreshUI()
{
    // Also allows SetTargetActor to be called before adding this widget to a viewport.
    if (!ToggleMotionButton || !ResetMotionButton || !ToggleMotionText || !MotionStateText)
    {
        return;
    }

    const bool bHasTarget = IsValid(TargetActor);
    ToggleMotionButton->SetIsEnabled(bHasTarget);
    ResetMotionButton->SetIsEnabled(bHasTarget);

    if (!bHasTarget)
    {
        MotionStateText->SetText(NSLOCTEXT("LearningMotionUI", "NoTarget", "未连接方块"));
        ToggleMotionText->SetText(NSLOCTEXT("LearningMotionUI", "Unavailable", "不可操作"));
        return;
    }

    const bool bEnabled = TargetActor->IsMotionEnabled();
    MotionStateText->SetText(bEnabled
        ? NSLOCTEXT("LearningMotionUI", "Running", "状态：运动中")
        : NSLOCTEXT("LearningMotionUI", "Paused", "状态：已暂停"));
    ToggleMotionText->SetText(bEnabled
        ? NSLOCTEXT("LearningMotionUI", "PauseAction", "暂停运动")
        : NSLOCTEXT("LearningMotionUI", "ResumeAction", "继续运动"));
}
```

保存文件为 UTF-8，避免中文字符串编码问题。`NSLOCTEXT` 创建带命名空间/键的 FText，适合 UI 文本；本节无需配置完整本地化流程。

### 5.4 为什么比直接 OnClicked 多了一些代码

核心交互只有 `HandleToggleClicked → TargetActor->ToggleMotion()`。其他代码解决 UI 集成中的常见问题：

- `RefreshUI`：刚打开界面时就读取真实状态，不等下一次点击。
- `AddUniqueDynamic`：同一对象和回调不重复订阅。
- `NativeDestruct` 解绑：UI 移除时停止监听；NativeConstruct 可能在重新加入界面时再次调用。
- `SetTargetActor` 先解绑旧对象：以后切换控制另一个方块时，不被旧对象事件干扰。
- `OnDestroyed`：目标被销毁后按钮禁用。
- `IsValid`：空引用或已进入销毁状态时不继续调用。

这是单机关卡练习，不包含网络复制、跨关卡持久 UI 或流式关卡卸载的完整管理。Actor 被 Destroy 的处理不等于覆盖所有 EndPlay 场景；跨关卡系统需要另行管理生命周期。

**验收：**

- [ ] 冷构建成功，UMotionControlWidget 类已生成。
- [ ] 暂时还没创建 Widget Blueprint，游戏中没有 UI 是正常的。

## 6. 第三步：在 UMG 中设计面板

UMG 是 UE 的 UI 制作系统。Widget Blueprint 的 Designer 排布局，Graph 可写蓝图逻辑；我们把点击逻辑写在 C++，因此这次 Graph 不需要再接一套 OnClicked。[Epic UMG 入门](https://dev.epicgames.com/documentation/en-us/unreal-engine/umg-ui-designer-quick-start-guide-in-unreal-engine)

### 6.1 创建正确父类的 Widget Blueprint

1. 打开 UE，在 Content Browser 的 `Content/LearningBasics` 下建 `UI` 文件夹。
2. 右键 → User Interface → Widget Blueprint。
3. 在父类选择中找到 `MotionControlWidget`；可能需要展开 All Classes 搜索。
4. 命名为 `WBP_MotionPanel`。
5. 如果创建时只能先选 User Widget，打开资产，在 File → Reparent Blueprint 或 Class Settings 的父类设置中改为 MotionControlWidget。
6. 确认 Parent Class 是 MotionControlWidget，不是普通 UserWidget。

Widget Blueprint 创建后，在补齐 BindWidget 要求的控件之前，出现缺控件编译错误是正常的；按下面补齐后再编译。

### 6.2 建立控件树

在 Designer 左侧 Palette 中搜索控件，拖到 Hierarchy，最终结构如下：

```text
Canvas Panel
└─ Border（面板背景，可命名 PanelBackground）
   └─ Vertical Box
      ├─ Text Block：MotionStateText
      ├─ Button：ToggleMotionButton
      │  └─ Text Block：ToggleMotionText
      └─ Button：ResetMotionButton
         └─ Text Block：ResetLabel
```

若初始没有 Canvas Panel，先放一个作为根。Border 和 Button 都只有一个内容槽位：多个元素应放进 Vertical Box，再把 Vertical Box 放到 Border 中。

**必须严格匹配的四个名字：**

- ToggleMotionButton：Button。
- ResetMotionButton：Button。
- ToggleMotionText：Text Block。
- MotionStateText：Text Block。

对这四个控件勾选 Is Variable（如果面板提供该选项）。ResetLabel 不参与 C++ 绑定，名字可以自定，文本填“重置位置”。

### 6.3 让它固定在屏幕左上角

1. 选 Border，在 Canvas Slot 中将 Anchors 设左上角，Alignment=(0,0)，Position=(30,30)，Size=(300,180)。
2. Border Padding 设 12，背景选较深颜色，文字选浅色。
3. Vertical Box 内每个子项留适当 Padding，例如 6；按钮文本居中。
4. MotionStateText 初始文本可填“等待连接”；ToggleMotionText 填“暂停运动”。运行时 C++ 会覆盖它们。
5. 保持按钮 Is Enabled=true、Visibility=Visible，先不添加其他覆盖整个面板的控件。
6. 编译并保存 Widget Blueprint，确保无 BindWidget 缺失/类型不匹配错误。

如果中文显示方框，换成含中文字形的字体，或先用 Running / Paused / Pause / Resume 验证逻辑。

**不要再在 Widget Graph 中为 ToggleMotionButton 添加 ToggleMotion 调用。** C++ 已订阅 OnClicked；两边同时切换可能一次点击执行两次，最终看起来完全没变。

## 7. 第四步：关卡创建 UI，并指定要控制谁

### 7.1 准备独立实验关卡

1. 将上一节关卡另存为 `Content/LearningBasics/Lvl_MotionUI`。
2. 先只留一个要控制的 LearningFloatActor，确认 CubeMesh 已选 Cube。
3. 将它放在容易观察的位置，例如 (0,0,200)。
4. World Outliner 中把这个实例重命名为 `Cube_Controlled`。这是实例标签，C++ 类名不变。

### 7.2 隔离已有 PlayerController 的输入逻辑

你的项目已有其他示例控制器，可能每帧改鼠标捕获或输入模式。为了让本节稳定，推荐只为新关卡使用一个简单 GameMode：

1. 在 LearningBasics 右键 → Blueprint Class → All Classes，选择 GameModeBase，命名 `BP_LearningUIGameMode`。
2. 打开 Class Defaults，Player Controller Class 设为引擎默认 PlayerController；Default Pawn Class 设为 None（若可选）。本节用固定 CameraActor，不需要玩家角色移动。
3. 保存，在 `Lvl_MotionUI` 的 World Settings 中将 GameMode Override 设为 BP_LearningUIGameMode。
4. 只改当前关卡 Override，不必改整个项目的默认 GameMode。

### 7.3 放一个能看到方块的游戏相机

1. Place Actors 搜索 Camera Actor，拖入关卡，命名 `Camera_UI`。
2. 调整位置和旋转，使其朝向方块。可以在编辑器中 Pilot 该相机调整构图，完成后退出 Pilot。
3. 不确定方向时，先把相机放在方块的 -X 方向，例如 (-600,0,200)，Rotation=(0,0,0)，UE 相机局部 +X 为前方，便于从侧面看到上下浮动。
4. 后面 Level Blueprint 用 Set View Target with Blend 明确设置游戏视角。

### 7.4 取得准确的关卡实例引用

1. 在 World Outliner 选中 Cube_Controlled。
2. 从关卡工具栏的 Blueprints 菜单打开 Level Blueprint。
3. 在图中右键，选择 Create a Reference to Cube_Controlled。也可把 Outliner 实例拖到已打开的图中，具体操作取决于编辑器布局。
4. 同样为 Camera_UI 创建关卡实例引用。

这些节点引用的是关卡实例，不是 C++ Class 或默认对象。不要在 Widget 内 Get All Actors Of Class 后随便取第 0 个；场景放第二个 Cube 时这种写法就容易控制错对象。

### 7.5 连接 BeginPlay 创建面板

在 Level Blueprint 里建立以下执行顺序。下面是节点连接说明，不是可以粘贴的蓝图代码：

```text
Event BeginPlay
→ Set View Target with Blend
→ Create Widget（Class = WBP_MotionPanel）
→ 保存返回值到 MotionPanel 变量
→ Set Target Actor
→ Add to Viewport
→ Set Show Mouse Cursor = true
→ Set Input Mode Game And UI
```

每个节点具体引脚：

1. **Get Player Controller**：Player Index=0。将输出接到后续需要 PlayerController 的节点。这是单机/单本地玩家练习。
2. **Set View Target with Blend**：Target 接 PlayerController，New View Target 接 Camera_UI，Blend Time=0。
3. **Create Widget**：Class 选 WBP_MotionPanel，Owning Player 接 PlayerController。
4. 对 Create Widget 的 Return Value 右键 → Promote to Variable，命名 MotionPanel。之后反复使用这个实例，不每次点按钮都重新创建 UI。
5. 从 MotionPanel 引用拖线，搜索 **Set Target Actor**（来自我们写的 UFUNCTION）。Target 接 MotionPanel，New Target 接 Cube_Controlled。
6. **Add to Viewport**：Target 接 MotionPanel，ZOrder=0 即可。
7. 从 PlayerController 引用拖线，创建 **Set Show Mouse Cursor**，值勾选 true。
8. **Set Input Mode Game And UI**：Player Controller 接同一个 PlayerController；In Widget to Focus 可先留空；Mouse Lock Mode=Do Not Lock；Hide Cursor During Capture=false。

有执行引脚的节点要用白线顺序连接；对象输出引脚只是传数据，不能代替执行线。Get Player Controller 等纯节点没有白色执行引脚是正常的。

本教程没有依赖方块与 Level Blueprint 的 BeginPlay 谁先发生：设置目标后会主动读状态；重置行为只在用户点击、正常进入游戏后发生。

**Set Input Mode Game And UI** 让 UI 有机会处理输入，未处理的输入可交给游戏。若将来做纯菜单，可考虑 UI Only；这两者都不同于 Set Game Paused。鼠标光标可见也不等于输入已经正确交给 UI。

### 7.6 用 Play 验证，不用 Simulate

上一节可以用 Simulate 观察方块，本节需要本地玩家、Viewport UI 和鼠标输入，使用 **Play → Selected Viewport** 或 **New Editor Window**。

1. 保存 Level Blueprint、Widget Blueprint 和关卡。
2. Play，应看到固定相机中的方块和左上角面板。
3. 鼠标点击“暂停运动”：方块停住，状态和按钮文字变化。
4. 再点击“继续运动”：从原相位继续。
5. 暂停时点“重置位置”：回到初始位置，仍然暂停。
6. 恢复后再点“重置位置”：回到初始位置后继续浮动。

不需要为 UMG Button 设置 PlayerController 的 Enable Click Events。那个选项通常用于世界对象的点击事件，不能替代屏幕 UI 的输入模式和命中测试。

## 8. 第五步：用实验验证架构，而不仅是按钮能点

### 实验 A：初始状态为关闭

- [ ] 停止 Play，在方块 Details 取消 Enable Motion，再运行。

预期：首次显示就写“状态：已暂停”和“继续运动”。这证明 UI 初始化会读 Actor，而不是写死“运行中”。

### 实验 B：由按钮以外的逻辑改变状态

- [ ] 临时在 Level Blueprint 的初始化链末尾接 Delay 3 秒，再调用 Cube_Controlled 的 `Set Motion Enabled(false)`。

预期：没有点击按钮，3 秒后方块停止，UI 也同步改变。实验完成后移除这段 Delay 测试逻辑。

这验证的是“Actor 状态变化→事件→UI”，不是“按钮自己换一段文字”。

### 实验 C：两个面板观察同一个 Actor

- [ ] 可选：再创建一个面板，改它的屏幕位置，两个面板都 SetTargetActor 到同一方块。

预期：点击任一面板，两份文字一起更新。动态多播允许多个监听者。完成实验后移除额外面板，避免重叠误判点击。

### 实验 D：两个方块，明确控制对象

- [ ] 放第二个 LearningFloatActor，保持 UI 只引用 Cube_Controlled。

预期：按钮只控制指定实例，另一个继续运动。想切换对象，调用 MotionPanel.SetTargetActor 指向第二个实例即可。

### 实验 E：目标被销毁

- [ ] 可选：Level Blueprint 初始化链末尾临时 Delay 5 秒，再 Destroy Actor(Cube_Controlled)。

预期：面板显示“未连接方块”，按钮禁用，不发生空引用访问。实验后移除测试节点。

### 实验 F：重复打开关闭面板

- [ ] 可选：对同一 MotionPanel 调用 Remove from Parent，再 Add to Viewport。

预期：按钮一次点击只执行一次切换。NativeDestruct/NativeConstruct 和 AddUniqueDynamic 避免重复订阅。此面板示例是常驻操作面板；如果做可关闭菜单，还需要在关闭时恢复游戏输入模式与鼠标状态。

## 9. 常见问题与定位顺序

### Widget Blueprint 报 BindWidget 缺失

检查 Parent Class、四个控件名字、控件类型和 Is Variable。不要给 ToggleMotionText 放一个 Rich Text Block，它与 UTextBlock 不同。

### 面板完全不出现

先确认运行的是 Lvl_MotionUI，使用 Play 而不是 Simulate；检查 BeginPlay 白色执行线是否执行、Create Widget 是否选 WBP_MotionPanel、Return Value 是否 Add to Viewport。可以在蓝图节点间临时加 Print String 检查执行是否到达。

### 面板出现，但写“未连接方块”

检查 Set Target Actor 的两个引用：Target 是 Widget 实例，New Target 是关卡中的 Cube_Controlled。只有创建 Widget，不给它目标引用，面板不会自动猜测要控制谁。

### 按钮能悬停，但点击后状态不变

检查 Widget Graph 是否又绑定了一次 ToggleMotion。一次点击切换两次会回到原状态。用 Actor 的日志检查一次点击打印了几条 enabled/disabled。

### 鼠标不显示或点击被游戏视角抢走

检查 Show Mouse Cursor 和 Set Input Mode Game And UI，确认是在 Add to Viewport 后设置。检查本关卡是否真的用了简单 PlayerController，而不是现有示例每帧重置输入的控制器。

### 按钮连悬停效果都没有

检查按钮和父容器 Is Enabled、Visibility，以及是否有另一个可命中控件覆盖它。父容器若设为 Not Hit-Testable (Self & All Children)，子按钮也无法接收鼠标；Self Only 与 Self & All Children 含义不同。

### 方块暂停但文字不变

正常运行应通过 SetMotionEnabled 改状态。直接在运行时 Details 修改 bEnableFloating，会绕过 Setter，不广播委托；本节不实现 PostEditChangeProperty 编辑器事件桥接。测试事件同步时用按钮或 UFUNCTION 调用。

### Reset 后方块立刻又动了

这是定义的行为：Reset 只重置位置和时间，不改开关。想“重置并暂停”，让相应按钮先调用 SetMotionEnabled(false)，再 ResetMotion；不要悄悄让同名接口承担不同含义。

### C++ 编译错误提到 AddDynamic / 签名不匹配

检查回调有 UFUNCTION，OnClicked 回调无参数，OnMotionEnabledChanged 回调为 bool，OnDestroyed 回调为 AActor*。成员函数名与声明一致。新增反射成员后冷构建，不手写 generated.h。

## 10. 进一步理解：这次刻意没做的抽象

**为什么用 Level Blueprint 创建 UI？** 它最容易持有关卡中明确的 Actor 引用，便于本节追踪数据流。大型项目通常将 UI 创建移到本地 PlayerController、HUD 或专门 UI 管理对象中，并通过选择系统指定目标；不用一开始就建整套框架。

**为什么不用 Widget 的 Text 属性绑定每帧查询？** 这节状态只在开关变化时更新，事件通知已足够，而且能清楚学习谁驱动谁。位置这类连续变化的信息，则可能需要按合适频率刷新。

**为什么不用 SetActorTickEnabled(false)？** 当前只想关闭运动。如果未来同一 Actor 的 Tick 还有其他逻辑，整个关闭 Tick 会一起停掉。若 Actor 只有运动，未来可以做更细的 Tick 管理，但外部 API 仍可保持 SetMotionEnabled 不变。

**按钮与 CheckBox 如何选择？** Button 适合“暂停/继续”动作；CheckBox 更直接表达“启用运动”状态。若后面换 CheckBox，OnCheckStateChanged 应调用 `SetMotionEnabled(bChecked)`，而不是 ToggleMotion；UI 程序化同步勾选值时也要防止不必要的事件回流。

## 11. 完成标准

- [ ] 游戏内按钮可点，鼠标输入正常。
- [ ] 暂停不跳回起点，继续不重新开始周期。
- [ ] 重置位置与暂停开关是独立操作。
- [ ] UI 首次打开的状态正确。
- [ ] 非 UI 调用 Setter 时，UI 仍然同步。
- [ ] 未指定目标时禁用按钮，不崩溃。
- [ ] 能解释 UFUNCTION、BindWidget、AddUniqueDynamic 和 Broadcast 的作用。
- [ ] 能分清类、关卡 Actor 实例、Widget Blueprint 资产和运行时 Widget 实例。

完成后，下一节可以把正弦动画换成 `v += a*dt; x += v*dt` 的自由落体，用同一个面板控制启停与重置，再逐步学习数值积分。
